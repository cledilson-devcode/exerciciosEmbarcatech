/**
 * Projeto: Servidor HTTP com controle de Alerta (Buzzer/Display/LED) via AP - Pico W
 */

#include "pico/stdlib.h"
#include "hardware/i2c.h"      // Necessário para o tipo i2c_inst_t usado pelo display
#include "hardware/gpio.h"     // Para controle de GPIO geral
#include <string.h>            // Funções de manipulação de strings (memset, strncmp, etc.)

// Módulos da aplicação de Display OLED
#include "inc/display/display_app.h"   // Funções principais da aplicação do display
#include "inc/display/ssd1306_i2c.h"   // Definições do driver SSD1306, como tamanho do buffer
#include "inc/display/oled_messages.h" // Estrutura e funções para mensagens de alerta no display

// Módulos de Rede e Wi-Fi
#include "pico/cyw43_arch.h" // Arquitetura específica do Pico para o chip Wi-Fi CYW43
#include "lwip/pbuf.h"       // Gerenciamento de buffers de pacotes LwIP
#include "lwip/tcp.h"        // API TCP do LwIP

// Servidores DHCP e DNS
#include "dhcpserver.h"      // Servidor DHCP para atribuir IPs aos clientes
#include "dnsserver.h"       // Servidor DNS para resolver nomes (opcional, mas útil para AP)

// Módulo do Buzzer
#include "inc/buzzer.h"        // Funções de controle do buzzer

// --- Definições do Servidor HTTP e Aplicação ---
#define TCP_PORT 80                                        // Porta padrão para HTTP
#define DEBUG_printf printf                                // Macro para mensagens de depuração (pode ser desabilitada)
#define POLL_TIME_S 5                                      // Tempo (em segundos) para o callback de poll do TCP LwIP
#define HTTP_GET "GET "                                    // Prefixo para requisições HTTP GET (com espaço)
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n" // Cabeçalhos HTTP padrão para resposta OK
#define LED_TEST_BODY "<html><head><title>PicoW Control</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"></head><body><h1>Controle de Alerta PicoW</h1><p>Estado do Alerta (Buzzer/Display/LED): <strong>%s</strong></p><p><a href=\"/?led=1\" role=\"button\">ATIVAR ALERTA</a></p><p><a href=\"/?led=0\" role=\"button\">DESATIVAR ALERTA</a></p></body></html>" // Corpo HTML da página de controle
#define LED_PARAM "led=%d"                                 // Formato do parâmetro URL para controlar o alerta
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s/\n\n" // Cabeçalhos para redirecionamento HTTP

// --- Pinos GPIO ---
// #define LED_GPIO_VERMELHO 13 // Pino do LED vermelho físico (controlado por display_app agora)
#define BUZZER_PIN 21             // Pino GPIO conectado ao buzzer

// --- Variáveis Globais Estáticas para o Display OLED ---
// Buffer para armazenar os dados dos pixels do display OLED
static uint8_t g_oled_buffer[ssd1306_buffer_length];
// Estrutura que define a área de renderização no display
static struct render_area g_oled_render_area;
// Estrutura que armazena o estado do alerta piscante no display
static BlinkingAlertState g_oled_alert_state;
// Ponteiro para a instância I2C usada pelo display (i2c1)
static i2c_inst_t* g_i2c_port_display = i2c1;

// --- Estruturas para o Servidor TCP ---
// Estado geral do servidor TCP (inclui o PCB do servidor e estado de conclusão)
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; // Protocol Control Block (PCB) do LwIP para o servidor
    bool complete;              // Flag para indicar se o servidor deve ser finalizado
    ip_addr_t gw;               // Endereço IP do gateway (Pico W no modo AP)
    char ap_name[32];           // Nome do Access Point (SSID)
} TCP_SERVER_T;

// Estado de uma conexão TCP individual com um cliente
typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;        // PCB desta conexão específica com o cliente
    int sent_len;               // Número de bytes já enviados nesta conexão
    char headers[128];          // Buffer para os cabeçalhos da resposta HTTP
    char result[512];           // Buffer para o corpo da resposta HTTP (página HTML)
    int header_len;             // Comprimento dos cabeçalhos HTTP
    int result_len;             // Comprimento do corpo da resposta HTTP
    ip_addr_t *gw;              // Ponteiro para o endereço IP do gateway
    char *ap_name_ptr;          // Ponteiro para o nome do AP (para redirecionamentos)
} TCP_CONNECT_STATE_T;


// --- Callbacks e Funções do Servidor TCP ---

// Fecha a conexão com um cliente TCP e libera os recursos associados.
static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        // Garante que o estado da conexão corresponde ao PCB que está sendo fechado
        if (con_state && con_state->pcb != client_pcb) {
            // Não faz nada com con_state se não pertencer a este pcb
        } else if (con_state) {
             assert(con_state->pcb == client_pcb); // Verifica consistência em modo debug
        }

        // Remove callbacks e argumentos do PCB antes de fechar
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);

        // Tenta fechar a conexão TCP
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            // Se o fechamento normal falhar, força o aborto da conexão
            DEBUG_printf("tcp_close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT; // Indica que a conexão foi abortada
        }
        // Libera a memória alocada para o estado da conexão
        if (con_state) {
            free(con_state);
        }
    }
    return close_err; // Retorna o status do fechamento
}

// Fecha o PCB do servidor TCP principal, parando de aceitar novas conexões.
static void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL); // Remove argumento do PCB do servidor
        tcp_close(state->server_pcb);     // Fecha o PCB do servidor
        state->server_pcb = NULL;         // Marca como nulo para evitar uso posterior
    }
}

// Callback chamado pelo LwIP quando dados foram enviados e confirmados pelo cliente.
static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (!con_state) return ERR_OK; // Estado da conexão pode já ter sido liberado

    con_state->sent_len += len; // Atualiza o contador de bytes enviados
    // Se todos os dados (cabeçalhos + corpo) foram enviados, fecha a conexão
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK; // Indica sucesso no envio
}

// Processa a requisição HTTP, controla hardware e gera a resposta HTML.
static int server_handle_request(const char *params, char *result, size_t max_result_len) {
    int http_param_led_value = -1; // Valor padrão, indica que nenhum parâmetro 'led' foi passado

    // Se existem parâmetros na URL, tenta extrair o valor de 'led'
    if (params) {
        sscanf(params, LED_PARAM, &http_param_led_value);
    }

    // Controla o estado do alerta (buzzer, display, LED físico) com base no parâmetro 'led'
    if (http_param_led_value == 1) { // Se ?led=1 (ATIVAR ALERTA)
        DEBUG_printf("HTTP Request: ATIVAR Alerta\n");
        g_play_music_flag = true;                    // Ativa flag do buzzer
        display_application_activate_alert();        // Ativa alerta "EVACUAR" no display e LED associado
        // gpio_put(LED_GPIO_VERMELHO, 1); // O LED vermelho agora é controlado por display_app
    } else if (http_param_led_value == 0) { // Se ?led=0 (DESATIVAR ALERTA)
        DEBUG_printf("HTTP Request: DESATIVAR Alerta\n");
        g_play_music_flag = false;                   // Desativa flag do buzzer
        display_application_deactivate_alert();      // Ativa estado "SEGURO" no display e apaga LED associado
        // gpio_put(LED_GPIO_VERMELHO, 0);
    }
    // Se nenhum parâmetro 'led' válido foi passado, o estado do alerta não é alterado.

    // Gera o corpo da página HTML dinamicamente com base no estado atual do alerta
    if (g_play_music_flag) { // Verifica o estado do buzzer para determinar a mensagem
        return snprintf(result, max_result_len, LED_TEST_BODY, "ATIVO (EVACUAR)");
    } else {
        return snprintf(result, max_result_len, LED_TEST_BODY, "INATIVO (SEGURO)");
    }
}

// Callback chamado pelo LwIP quando dados são recebidos de um cliente.
err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err_in) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;

    // Se p for NULL, o cliente fechou a conexão
    if (!p) {
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    // Garante que o estado da conexão corresponde ao PCB
    assert(con_state && con_state->pcb == pcb);

    // Se dados foram recebidos
    if (p->tot_len > 0) {
        char request_buffer[128]; // Buffer para armazenar a requisição HTTP
        memset(request_buffer, 0, sizeof(request_buffer));
        // Copia os dados recebidos para o buffer local, limitando ao tamanho do buffer
        pbuf_copy_partial(p, request_buffer, p->tot_len > sizeof(request_buffer) - 1 ? sizeof(request_buffer) - 1 : p->tot_len, 0);

        // Processa apenas requisições GET
        if (strncmp(HTTP_GET, request_buffer, strlen(HTTP_GET)) == 0) {
            char *request_path = request_buffer + strlen(HTTP_GET); // Aponta para o caminho após "GET "
            char *params_start = strchr(request_path, '?');         // Encontra o início dos parâmetros
            char *http_version_start = strstr(request_path, " HTTP/"); // Encontra o início da versão HTTP

            // Termina a string do caminho/parâmetros antes da versão HTTP
            if (http_version_start) {
                *http_version_start = 0;
            }

            char *actual_params = NULL;
            // Se existem parâmetros, isola o caminho dos parâmetros
            if (params_start) {
                *params_start = 0; // Termina a string do caminho
                actual_params = params_start + 1; // Aponta para os parâmetros
            }

            // Gera o conteúdo da resposta (página HTML)
            con_state->result_len = server_handle_request(actual_params, con_state->result, sizeof(con_state->result));

            // Prepara os cabeçalhos HTTP
            if (con_state->result_len > 0) { // Se há conteúdo para enviar (página HTML)
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                                                 200, con_state->result_len); // Resposta 200 OK
            } else { // Se não há conteúdo (ex: erro ao gerar), envia um redirecionamento
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT,
                                                 con_state->ap_name_ptr ? con_state->ap_name_ptr : ipaddr_ntoa(con_state->gw));
            }

            // Verifica se os cabeçalhos excederam o buffer
            if (con_state->header_len > sizeof(con_state->headers) - 1) {
                DEBUG_printf("Header buffer overflow %d\n", con_state->header_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Envia os cabeçalhos HTTP para o cliente
            con_state->sent_len = 0; // Reseta contador de bytes enviados para esta resposta
            err_t err_write = tcp_write(pcb, con_state->headers, con_state->header_len, TCP_WRITE_FLAG_COPY);
            if (err_write != ERR_OK) {
                DEBUG_printf("tcp_write failed for headers: %d\n", err_write);
                return tcp_close_client_connection(con_state, pcb, err_write);
            }

            // Se houver corpo da resposta, envia também
            if (con_state->result_len > 0) {
                err_write = tcp_write(pcb, con_state->result, con_state->result_len, TCP_WRITE_FLAG_COPY);
                if (err_write != ERR_OK) {
                    DEBUG_printf("tcp_write failed for body: %d\n", err_write);
                    return tcp_close_client_connection(con_state, pcb, err_write);
                }
            }
        }
        // Informa ao LwIP que os dados recebidos foram processados
        tcp_recved(pcb, p->tot_len);
    }
    // Libera o buffer de pacotes pbuf
    pbuf_free(p);
    return ERR_OK; // Indica sucesso no recebimento
}

// Callback chamado periodicamente pelo LwIP para conexões ativas (se configurado).
// Usado aqui para fechar conexões inativas após um tempo.
static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    // DEBUG_printf("tcp_server_poll: pcb=%p\n", (void*)pcb);
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Simplesmente fecha a conexão
}

// Callback chamado pelo LwIP quando ocorre um erro na conexão TCP.
static void tcp_server_err(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) { // ERR_ABRT significa que nós mesmos abortamos a conexão
        DEBUG_printf("tcp_server_err: %d on pcb=%p con_state=%p\n", err, con_state ? (void*)con_state->pcb : NULL, (void*)con_state);
        if (con_state) { // Só tenta fechar se con_state for válido (não foi liberado ainda)
             tcp_close_client_connection(con_state, con_state->pcb, err);
        }
    }
}

// Callback chamado pelo LwIP quando uma nova conexão TCP é aceita no servidor.
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg; // Estado geral do servidor
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("Failure in accept\n");
        return ERR_VAL; // Indica erro na aceitação
    }
    // DEBUG_printf("Client connected, pcb=%p\n", (void*)client_pcb);

    // Aloca memória para o estado desta nova conexão
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("Failed to allocate connect state\n");
        tcp_close(client_pcb); // Fecha o PCB do cliente se não puder alocar estado
        return ERR_MEM; // Erro de memória
    }
    con_state->pcb = client_pcb;        // Armazena o PCB do cliente
    con_state->gw = &state->gw;         // Ponteiro para o gateway do AP
    con_state->ap_name_ptr = state->ap_name; // Ponteiro para o nome do AP

    // Configura os callbacks para esta conexão específica
    tcp_arg(client_pcb, con_state);         // Argumento a ser passado para os callbacks
    tcp_sent(client_pcb, tcp_server_sent);  // Callback para dados enviados
    tcp_recv(client_pcb, tcp_server_recv);  // Callback para dados recebidos
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2); // Callback de poll (ex: a cada 10s)
    tcp_err(client_pcb, tcp_server_err);    // Callback para erros

    return ERR_OK; // Indica sucesso na aceitação da conexão
}

// Inicializa e abre o servidor TCP para escutar na porta definida.
static bool tcp_server_open(void *arg, const char *ap_name_param) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    // Armazena o nome do AP no estado do servidor
    strncpy(state->ap_name, ap_name_param, sizeof(state->ap_name) -1);
    state->ap_name[sizeof(state->ap_name) -1] = '\0'; // Garante terminação nula

    DEBUG_printf("Starting server on port %d\n", TCP_PORT);

    // Cria um novo PCB TCP
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY); // Aceita conexões IPv4 ou IPv6
    if (!pcb) {
        DEBUG_printf("Failed to create pcb\n");
        return false;
    }

    // Associa (bind) o PCB a qualquer endereço IP local e à porta TCP_PORT
    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        DEBUG_printf("Failed to bind to port %d, error %d\n", TCP_PORT, err);
        tcp_close(pcb); // Libera o pcb se o bind falhar
        return false;
    }

    // Coloca o servidor em modo de escuta (listen) para novas conexões
    // O '1' é o backlog (número de conexões pendentes permitidas)
    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("Failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    // Configura o argumento e o callback de aceitação para o PCB do servidor
    tcp_arg(state->server_pcb, state); // Passa o estado do servidor para o callback de accept
    tcp_accept(state->server_pcb, tcp_server_accept); // Define a função a ser chamada para novas conexões

    printf("Servidor HTTP iniciado. Conecte-se a Wi-Fi '%s' e acesse http://%s\n",
           ap_name_param, ip4addr_ntoa(&state->gw)); // Exibe informações de conexão
    printf("(Pressione 'd' no serial para desabilitar o Access Point)\n");
    return true; // Servidor aberto com sucesso
}

// Função de callback chamada quando há caracteres disponíveis na entrada serial.
// Usada para detectar a tecla 'd' e desabilitar o modo Access Point.
void key_pressed_func(void *param) {
    if (!param) return;
    TCP_SERVER_T *state = (TCP_SERVER_T*)param;
    int key = getchar_timeout_us(0); // Lê tecla sem bloquear
    if (key == 'd' || key == 'D') {
        DEBUG_printf("Tecla 'd' pressionada, desabilitando AP...\n");
        // Entra em modo crítico para operações de rede
        cyw43_arch_lwip_begin();
        cyw43_arch_disable_ap_mode(); // Desabilita o modo Access Point
        cyw43_arch_lwip_end();        // Sai do modo crítico
        state->complete = true;       // Sinaliza para o loop principal terminar
    }
}

// Função principal da aplicação.
int main() {
    // Inicializa stdio para comunicação serial (USB ou UART)
    stdio_init_all();
    sleep_ms(1000); // Pequena pausa para garantir que o serial esteja pronto
    DEBUG_printf("Inicializando...\n");

    // Inicializa o módulo do buzzer e define o estado inicial da flag de música
    buzzer_init(BUZZER_PIN);
    g_play_music_flag = false; // Garante que o buzzer começa desligado

    // Inicializa a aplicação do Display OLED
    // As variáveis g_oled_buffer, g_oled_render_area, g_oled_alert_state e g_i2c_port_display
    // são passadas para serem gerenciadas pelo módulo display_app.
    if (!display_application_init(g_oled_buffer, &g_oled_render_area, &g_oled_alert_state, g_i2c_port_display)) {
        DEBUG_printf("ERRO: Falha ao inicializar a aplicacao do display!\n");
        // Decide se deve continuar ou parar em caso de falha
    } else {
        DEBUG_printf("Aplicacao do display OLED inicializada.\n");
    }

    // Aloca memória para o estado do servidor TCP
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("ERRO: Falha ao alocar estado do servidor TCP!\n");
        return 1; // Falha crítica
    }

    // Inicializa o chip Wi-Fi (CYW43xxx)
    if (cyw43_arch_init()) {
        DEBUG_printf("ERRO: Falha ao inicializar cyw43_arch!\n");
        free(state); // Libera memória alocada antes de sair
        return 1; // Falha crítica
    }
    DEBUG_printf("cyw43_arch inicializado.\n");

    // Configura callback para detectar teclas pressionadas no serial
    stdio_set_chars_available_callback(key_pressed_func, state);

    // Define o nome (SSID) e a senha da rede Wi-Fi do Access Point
    const char *ap_name = "PicoW_Alerta";
    const char *password = "12345678";

    // Habilita o modo Access Point no chip Wi-Fi
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
    DEBUG_printf("Modo Access Point '%s' habilitado.\n", ap_name);

    // Configura o endereço IP e máscara de sub-rede para o Access Point
    ip4_addr_t mask;
    ip4_addr_set_u32(&state->gw, PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS)); // IP padrão do AP (ex: 192.168.4.1)
    ip4_addr_set_u32(&mask, PP_HTONL(CYW43_DEFAULT_IP_MASK));      // Máscara padrão (ex: 255.255.255.0)

    // Inicia o servidor DHCP para atribuir IPs aos clientes que se conectarem
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &state->gw, &mask);
    DEBUG_printf("Servidor DHCP iniciado.\n");

    // Inicia o servidor DNS (útil para resolver o nome do AP para seu IP)
    dns_server_t dns_server;
    dns_server_init(&dns_server, &state->gw);
    DEBUG_printf("Servidor DNS iniciado.\n");

    // Abre o servidor TCP para escutar por conexões HTTP
    if (!tcp_server_open(state, ap_name)) {
        DEBUG_printf("ERRO: Falha ao abrir o servidor TCP!\n");
        // Limpa recursos de rede antes de sair em caso de falha
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_deinit();
        free(state);
        return 1; // Falha crítica
    }

    state->complete = false; // Flag para controlar o loop principal
    DEBUG_printf("Loop principal iniciado.\n");

    // Loop principal da aplicação
    while(!state->complete) {
        // Se a flag de música estiver ativa, toca o buzzer
        // buzzer_play deve ser não-bloqueante ou tocar uma nota/sequência curta
        if (g_play_music_flag) {
            buzzer_play(BUZZER_PIN);
        }

        // Processa a lógica contínua do display OLED (ex: piscar "EVACUAR")
        display_application_process();

        // Gerenciamento do driver Wi-Fi e da pilha LwIP
#if PICO_CYW43_ARCH_POLL
        // Modo polling: chama cyw43_arch_poll() e espera por trabalho ou timeout
        cyw43_arch_poll();
        cyw43_arch_wait_for_work_until(make_timeout_time_ms(20)); // Timeout para o poll (ex: 20ms)
#else
        // Modo não-polling (interrupção): apenas uma pausa para outras tarefas
        sleep_ms(20); // Pausa no loop principal (ex: 20ms)
#endif
    }

    // --- Finalização da Aplicação ---
    DEBUG_printf("Finalizando aplicacao...\n");
    // Fecha o servidor TCP
    tcp_server_close(state);
    // Desinicializa servidores DNS e DHCP
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    // Desinicializa o chip Wi-Fi
    // cyw43_arch_disable_ap_mode(); // Já chamado em key_pressed_func
    cyw43_arch_deinit();
    // Libera a memória do estado do servidor
    free(state);
    printf("Aplicacao finalizada.\n");
    return 0; // Fim do programa
}
/**
 * Projeto: Servidor HTTP com controle de LED e Leitura de Temperatura via Access Point - Raspberry Pi Pico W
 * HTML com atualizacao dinamica da temperatura via JavaScript (Fetch API) a cada 1 segundo.
 * Lista de temperaturas capturadas ao ligar o LED, persistida com localStorage.
 */

// === INCLUDES ===
#include <string.h> // Para funcoes de string como strcmp, strlen, strcpy, strcat, strchr, sscanf
#include <stdio.h>  // Para printf (saida de debug) e snprintf (formatacao de strings)
#include <stdlib.h> // Para calloc (alocacao de memoria) e free (liberacao de memoria)
#include <assert.h> // Para a macro assert() usada para checagens de sanidade durante o desenvolvimento

// Includes especificos do Pico SDK
#include "pico/cyw43_arch.h" // Para controle do chip Wi-Fi CYW43 (inicializacao, modo AP)
#include "pico/stdlib.h"     // Funcoes basicas do SDK (gpio_init, sleep_ms, stdio_init_all)

// Includes da pilha LwIP (Lightweight IP) para networking
#include "lwip/pbuf.h"    // Gerenciamento de buffers de pacotes
#include "lwip/tcp.h"     // API raw para TCP (conexoes, callbacks)
#include "lwip/ip4_addr.h" // Para manipulacao de enderecos IPv4 (ip4addr_aton, ip_addr_set_ip4_u32)

// Includes dos modulos locais para servidores DHCP e DNS
#include "dhcpserver.h" // Para o servidor DHCP que atribui IPs aos clientes
#include "dnsserver.h"  // Para o servidor DNS que resolve nomes para o IP do Pico

// Include para o hardware ADC (Analog-to-Digital Converter)
#include "hardware/adc.h" // Para ler o sensor de temperatura interno

// === DEFINES GLOBAIS ===
#define TCP_PORT 80         // Porta padrao para o servidor HTTP
#define DEBUG_printf printf // Define DEBUG_printf para usar printf (facilita desabilitar todos os debugs se necessario)
#define POLL_TIME_S 5       // Tempo (em segundos) para o LwIP chamar o callback de poll de uma conexao TCP individual
                            // O LwIP chama o poll a cada POLL_TIME_S * 0.5 segundos.

#define HTTP_GET "GET"      // String para identificar requisicoes HTTP GET

// String HTML principal que sera servida. Contem placeholders (%s, %d, %.2f)
// que serao preenchidos pelo snprintf no codigo C.
// Inclui JavaScript embutido para atualizacao dinamica da temperatura e lista.
#define MAIN_PAGE_HTML \
    "<html><head><title>Pico W Controle</title></head><body>" \
    "<h1>Pico W Controle</h1>" \
    "<p>LED esta: <span id=\"estadoLedHtml\">%s</span></p>" \
    "<p><a href=\"/?led=%d\">Mudar LED para %s</a></p><hr>" \
    "<p>Temperatura Interna Atual: <span id=\"temperaturaAtualHtml\">%.2f</span> °C</p>" \
    "<hr><h3>Temperaturas Capturadas (quando LED foi ligado):</h3>" \
    "<div id=\"listaTemperaturasHtml\"><p>Carregando lista...</p></div>" \
    /* Botao para limpar o localStorage adicionado abaixo */ \
    "<p><button onclick=\"limparListaTemperaturas()\">Limpar Lista de Temperaturas</button></p>" \
    "<script>" \
    "const servidorLedLigado = %s; /* Injetado pelo C: true ou false */\n" \
    "const servidorTemperaturaInicial = %.2f; /* Injetado pelo C */\n" \
    "let capturasSalvas = JSON.parse(localStorage.getItem('picoTemperaturas')) || [];\n" \
    "\n" \
    "const spanTempAtual = document.getElementById('temperaturaAtualHtml');\n" \
    "const divListaTemp = document.getElementById('listaTemperaturasHtml');\n" \
    "\n" \
    "function atualizarSpanTemperatura(tempValor) {" \
    "  if (spanTempAtual) spanTempAtual.innerText = parseFloat(tempValor).toFixed(2);" \
    "}\n" \
    "\n" \
    "function mostrarListaTemperaturas() {" \
    "  if (!divListaTemp) return;\n" \
    "  divListaTemp.innerHTML = '';\n" \
    "  if (capturasSalvas.length === 0) {" \
    "    divListaTemp.innerHTML = '<p>Nenhuma temperatura capturada.</p>';" \
    "    return;" \
    "  }\n" \
    "  const ul = document.createElement('ul');\n" \
    "  capturasSalvas.forEach(function(item) {" \
    "    const li = document.createElement('li');" \
    "    li.textContent = parseFloat(item.temp).toFixed(2) + ' C (as ' + new Date(item.time).toLocaleTimeString() + ')';" \
    "    ul.appendChild(li);" \
    "  });\n" \
    "  divListaTemp.appendChild(ul);" \
    "}\n" \
    "\n" \
    /* Funcao para limpar o localStorage e atualizar a exibicao da lista */ \
    "function limparListaTemperaturas() {" \
    "  if (confirm('Tem certeza que deseja limpar todas as temperaturas capturadas?')) {" \
    "    localStorage.removeItem('picoTemperaturas');\n" \
    "    capturasSalvas = [];\n" \
    "    mostrarListaTemperaturas();\n" \
    "    console.log('Lista de temperaturas limpa.');\n" \
    "  }" \
    "}\n" \
    "\n" \
    "function fetchTemperaturaPeriodica() {" \
    "  fetch('/api/temperatura')" \
    "    .then(response => response.json())" \
    "    .then(data => { atualizarSpanTemperatura(data.temperatura); })" \
    "    .catch(error => {" \
    "      console.error('Erro ao buscar temperatura periodicamente:', error);" \
    "      if(spanTempAtual) spanTempAtual.innerText = 'Erro!';" \
    "    });" \
    "}\n" \
    "\n" \
    "document.addEventListener('DOMContentLoaded', function() {" \
    "  console.log('DOM Carregado. LED Ligado (do servidor):', servidorLedLigado, 'Temp Inicial (do servidor):', servidorTemperaturaInicial);\n" \
    "  atualizarSpanTemperatura(servidorTemperaturaInicial);\n" \
    "  mostrarListaTemperaturas();\n" \
    "\n" \
    "  const paramsUrl = new URLSearchParams(window.location.search);\n" \
    "  const acaoLed = paramsUrl.get('led');\n" \
    "\n" \
    "  if (acaoLed !== null && servidorLedLigado) {\n" \
    "    console.log('LED foi LIGADO nesta acao, capturando temperatura:', servidorTemperaturaInicial);\n" \
    "    const novaCaptura = { temp: servidorTemperaturaInicial, time: Date.now() };\n" \
    "    capturasSalvas.push(novaCaptura);\n" \
    "    localStorage.setItem('picoTemperaturas', JSON.stringify(capturasSalvas));\n" \
    "    mostrarListaTemperaturas(); \n" \
    "  }\n" \
    "\n" \
    "  setInterval(fetchTemperaturaPeriodica, 1000); /* ATUALIZADO PARA 1000ms (1 SEGUNDO) */ \n" \
    "});\n" \
    "</script>" \
    "</body></html>"

#define LED_PARAM "led=%d"                  // Formato do parametro GET para controlar o LED (ex: /?led=1)
#define PATH_MAIN_PAGE "/"                  // Caminho (path) para a pagina HTML principal
#define PATH_API_TEMPERATURE "/api/temperatura" // Caminho para o endpoint JSON da temperatura
#define LED_GPIO 13                         // GPIO do RP2040 conectado ao LED (conforme PDF)
#define HTTP_REDIRECT_HEADER_FORMAT "HTTP/1.1 302 Found\r\nLocation: http://%s/\r\nConnection: close\r\n\r\n" // Formato do cabecalho de redirecionamento HTTP

// === ESTRUTURAS DE DADOS ===

// Estrutura para armazenar o estado do servidor TCP principal
typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb; // Protocol Control Block (PCB) do LwIP para o socket de escuta do servidor
    bool complete;              // Flag para indicar se o servidor deve encerrar (controlado pela tecla 'd')
    ip_addr_t gw;               // Endereco IP do gateway (que sera o proprio Pico W no modo AP)
} TCP_SERVER_T;

// Estrutura para armazenar o estado de cada conexao TCP individual de cliente
typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;        // PCB desta conexao especifica com um cliente
    int sent_len;               // Numero de bytes ja enviados nesta conexao
    char response_header[256];  // Buffer para armazenar o cabecalho da resposta HTTP
    char response_body[3072];   // Buffer para armazenar o corpo da resposta HTTP (HTML ou JSON)
                                // Aumentado para acomodar o JavaScript extenso.
    int header_len;             // Comprimento do cabecalho da resposta
    int body_len;               // Comprimento do corpo da resposta
    ip_addr_t *gw;              // Ponteiro para o IP do gateway (para redirecionamentos)
} TCP_CONNECT_STATE_T;

// === FUNCOES ===

// Converte o valor bruto lido do ADC do sensor de temperatura para graus Celsius.
float convert_to_celsius(uint16_t raw) {
    const float conversion_factor = 3.3f / (1 << 12); // VRef / (2^resolucao_ADC)
    float voltage = raw * conversion_factor;
    // Formula do datasheet do RP2040: Temp (°C) = 27 - (ADC_voltage - 0.706) / 0.001721
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

// Fecha uma conexao TCP com um cliente de forma segura e libera recursos.
static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        // Desregistra callbacks para evitar chamadas futuras em um estado invalido
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);

        err_t err = tcp_close(client_pcb); // Tenta fechar a conexao TCP
        if (err != ERR_OK) {
            DEBUG_printf("tcp_close falhou %d, chamando abort\n", err);
            tcp_abort(client_pcb); // Forca o fechamento se tcp_close falhar
            close_err = ERR_ABRT;  // Indica que a conexao foi abortada
        }
        if (con_state) {
            free(con_state); // Libera a memoria da estrutura de estado da conexao
        }
    }
    return close_err;
}

// Fecha o socket de escuta do servidor TCP principal.
static void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL); // Desregistra o argumento do callback de accept
        tcp_close(state->server_pcb);    // Fecha o PCB de escuta
        state->server_pcb = NULL;
    }
}

// Callback do LwIP chamado quando dados enviados anteriormente foram reconhecidos (ACKed) pelo cliente.
static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg; // Estado da conexao especifica
    if (!con_state) return ERR_OK; 

    con_state->sent_len += len; // Incrementa o contador de bytes enviados e reconhecidos

    // Se todos os bytes do cabecalho e do corpo foram enviados e reconhecidos
    if (con_state->sent_len >= con_state->header_len + con_state->body_len) {
        // DEBUG_printf("envio completo: %d bytes (cab: %d, corpo: %d)\n", con_state->sent_len, con_state->header_len, con_state->body_len);
        return tcp_close_client_connection(con_state, pcb, ERR_OK); // Fecha a conexao
    }
    return ERR_OK;
}

// Gera o corpo e o cabecalho da resposta HTTP com base no caminho requisitado e parametros.
static void generate_server_response_content(TCP_CONNECT_STATE_T *con_state, const char *request_path, const char *params) {
    // Reinicializa comprimentos para cada nova resposta
    con_state->body_len = 0;    // Comprimento do corpo da resposta (HTML ou JSON)
    con_state->header_len = 0;  // Comprimento do cabecalho HTTP
    // Variaveis para construir a resposta HTTP
    int http_status_code = 200;         // Codigo de status HTTP (padrao: 200 OK)
    const char *http_status_text = "OK";  // Texto do status HTTP (padrao: "OK")
    char content_type_str[64] = "text/html; charset=utf-8"; // Tipo de conteudo (padrao: HTML)

    // Roteamento baseado no caminho da requisicao
    if (strcmp(request_path, PATH_MAIN_PAGE) == 0) { // Requisicao para a pagina principal
        bool led_state_final = gpio_get(LED_GPIO); // Le o estado atual do LED

        // Processa parametros GET (ex: para ligar/desligar LED)
        if (params) { 
            int led_val_from_param;
            if (sscanf(params, LED_PARAM, &led_val_from_param) == 1) { // Tenta parsear "led=X"
                gpio_put(LED_GPIO, led_val_from_param ? 1 : 0); // Altera o estado do GPIO
                led_state_final = led_val_from_param ? true : false; // Atualiza o estado para a resposta HTML
                DEBUG_printf("LED %s por requisicao\n", led_state_final ? "LIGADO" : "DESLIGADO");
            }
        }
        // Le a temperatura atual do sensor interno
        adc_select_input(4); // Seleciona o canal ADC do sensor de temperatura
        uint16_t raw_temp = adc_read(); // Le o valor bruto do ADC
        float temp_c_atual = convert_to_celsius(raw_temp); // Converte para Celsius

        // Formata o corpo HTML usando a macro MAIN_PAGE_HTML e os dados atuais
        con_state->body_len = snprintf(con_state->response_body, sizeof(con_state->response_body),
                                       MAIN_PAGE_HTML,
                                       led_state_final ? "LIGADO" : "DESLIGADO",     // %s para o estado do LED no HTML
                                       led_state_final ? 0 : 1,                      // %d para o proximo estado no link do LED
                                       led_state_final ? "DESLIGADO" : "LIGADO",     // %s para o texto do link do LED
                                       temp_c_atual,                                 // %.2f para a temperatura inicial no HTML
                                       led_state_final ? "true" : "false",           // %s para a variavel JS servidorLedLigado
                                       temp_c_atual);                                // %.2f para a variavel JS servidorTemperaturaInicial
        
        // Checa se o corpo HTML foi truncado devido ao buffer pequeno
        if (con_state->body_len >= sizeof(con_state->response_body) -1 ) {
            DEBUG_printf("AVISO: Corpo HTML truncado! Necessario: %d, Buffer: %zu. Ajustando body_len.\n", con_state->body_len, sizeof(con_state->response_body));
            con_state->body_len = strlen(con_state->response_body); // Usa o comprimento real da string truncada
        }

    } else if (strcmp(request_path, PATH_API_TEMPERATURE) == 0) { // Requisicao para o endpoint da API de temperatura
        adc_select_input(4);
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        // Formata o corpo da resposta como JSON
        con_state->body_len = snprintf(con_state->response_body, sizeof(con_state->response_body),
                                       "{\"temperatura\": %.2f}", temp_c);
        if (con_state->body_len >= sizeof(con_state->response_body) -1) {
             DEBUG_printf("AVISO: Corpo JSON truncado! Necessario: %d, Buffer: %zu. Ajustando body_len.\n", con_state->body_len, sizeof(con_state->response_body));
            con_state->body_len = strlen(con_state->response_body);
        }
        strcpy(content_type_str, "application/json; charset=utf-8"); // Define o Content-Type para JSON

    } else { // Caminho nao reconhecido
        http_status_code = 302; // Prepara para redirecionar para a pagina principal
        http_status_text = "Found"; // Texto padrao para status 302
    }

    // Montagem do Cabecalho HTTP
    if (http_status_code == 302) { // Se for um redirecionamento
         con_state->header_len = snprintf(con_state->response_header, sizeof(con_state->response_header),
                                     HTTP_REDIRECT_HEADER_FORMAT, ipaddr_ntoa(con_state->gw)); // gw é o IP do AP
        con_state->body_len = 0; // Redirecionamentos geralmente nao tem corpo
    } else { // Para respostas normais (200 OK, etc.)
        con_state->header_len = snprintf(con_state->response_header, sizeof(con_state->response_header),
                                   "HTTP/1.1 %d %s\r\n" 
                                   "Content-Type: %s\r\n"  
                                   "Content-Length: %d\r\n" 
                                   "Connection: close\r\n\r\n", 
                                   http_status_code, http_status_text,
                                   content_type_str,
                                   con_state->body_len); 

        // Checa se o cabecalho foi truncado
        if (con_state->header_len >= sizeof(con_state->response_header) -1) {
            DEBUG_printf("ERRO FATAL: Cabecalho HTTP truncado! Necessario: %d, Buffer: %zu\n", con_state->header_len, sizeof(con_state->response_header));
            con_state->header_len = 0; // Nao envia um cabecalho invalido
            con_state->body_len = 0;   // E, consequentemente, nao envia o corpo
        }
    }
}

// Callback do LwIP chamado quando dados sao recebidos de um cliente.
err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err_in) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg; // Estado da conexao especifica
    char client_request_buffer[128]; // Buffer para armazenar a linha de requisicao do cliente

    // Verifica se houve erro na recepcao
    if (err_in != ERR_OK) {
        DEBUG_printf("tcp_server_recv erro %d\n", err_in);
        if (p) pbuf_free(p); // Libera o pbuf se existir
        return tcp_close_client_connection(con_state, pcb, err_in);
    }

    // Se p for NULL, o cliente fechou a conexao
    if (!p) {
        DEBUG_printf("conexao fechada pelo cliente (p eh NULL)\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    
    // Checagem de sanidade para con_state
    if (!con_state) { 
        DEBUG_printf("ERRO: con_state eh NULL em tcp_server_recv\n");
        pbuf_free(p);
        tcp_abort(pcb); // Aborta o pcb se nao temos estado para ele
        return ERR_ABRT;
    }
    assert(con_state->pcb == pcb); // Garante que o estado corresponde ao PCB

    // Se dados foram recebidos
    if (p->tot_len > 0) {
        // Copia a parte inicial da requisicao para um buffer local
        int req_len = pbuf_copy_partial(p, client_request_buffer, sizeof(client_request_buffer) - 1, 0);
        client_request_buffer[req_len] = '\0'; // Adiciona terminador nulo

        // Processa apenas requisicoes GET
        if (strncmp(HTTP_GET, client_request_buffer, strlen(HTTP_GET)) == 0) {
            // Extrai o caminho e os parametros da linha de requisicao GET
            char *request_uri_start = client_request_buffer + strlen(HTTP_GET);
            while(*request_uri_start == ' ') request_uri_start++; 

            char *request_uri_end = strchr(request_uri_start, ' '); 
            if (request_uri_end) { *request_uri_end = '\0'; }
            else {
                request_uri_end = strchr(request_uri_start, '\r'); if (request_uri_end) *request_uri_end = '\0';
                else { request_uri_end = strchr(request_uri_start, '\n'); if (request_uri_end) *request_uri_end = '\0';}
            }

            char *request_path = request_uri_start;
            char *params = strchr(request_path, '?'); 
            if (params) { *params++ = '\0'; }

            // Gera o cabecalho e o corpo da resposta HTTP
            generate_server_response_content(con_state, request_path, params);

            // Envia a resposta se o cabecalho foi gerado corretamente
            if (con_state->header_len > 0) { 
                err_t write_err = tcp_write(pcb, con_state->response_header, con_state->header_len, TCP_WRITE_FLAG_COPY);
                if (write_err != ERR_OK) {
                    DEBUG_printf("falha ao escrever dados do cabecalho %d\n", write_err);
                    pbuf_free(p); 
                    return tcp_close_client_connection(con_state, pcb, write_err);
                }

                // Envia o corpo da resposta, se houver
                if (con_state->body_len > 0) {
                    write_err = tcp_write(pcb, con_state->response_body, con_state->body_len, TCP_WRITE_FLAG_COPY);
                    if (write_err != ERR_OK) {
                        DEBUG_printf("falha ao escrever dados do corpo %d\n", write_err);
                        pbuf_free(p);
                        return tcp_close_client_connection(con_state, pcb, write_err);
                    }
                }
            } else { // Se o cabecalho nao foi gerado (erro critico)
                DEBUG_printf("ERRO: Cabecalho nao foi gerado, fechando conexao.\n");
                pbuf_free(p);
                return tcp_close_client_connection(con_state, pcb, ERR_VAL); 
            }
        }
        tcp_recved(pcb, p->tot_len); // Informa ao LwIP que processamos os dados recebidos
    }
    pbuf_free(p); // Libera o pbuf de entrada apos o processamento
    return ERR_OK;
}

// Callback do LwIP chamado periodicamente para uma conexao TCP ativa.
static err_t tcp_server_poll_cb(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg; // Estado da conexao
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Fecha conexao ociosa
}

// Callback do LwIP chamado quando ocorre um erro nao fatal em uma conexao TCP
static void tcp_server_err_cb(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg; // Estado da conexao
    if (err != ERR_ABRT) { 
        DEBUG_printf("tcp_server_err_cb: erro %d\n", err);
        if (con_state) {
            free(con_state); // Libera o estado da aplicacao associado a conexao
        }
    } else {
        DEBUG_printf("tcp_server_err_cb: conexao abortada\n");
         if (con_state) { 
            free(con_state); 
        }
    }
}

// Callback do LwIP chamado quando uma nova conexao de cliente e aceita no socket de escuta.
static err_t tcp_server_accept_cb(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *server_state = (TCP_SERVER_T*)arg; // Estado do servidor principal

    // Verifica se o accept foi bem-sucedido
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("falha no callback de accept: err %d\n", err);
        return ERR_VAL; // Retorna erro se accept falhou
    }
    DEBUG_printf("cliente conectado de %s porta %u\n", ipaddr_ntoa(&client_pcb->remote_ip), client_pcb->remote_port);

    // Aloca memoria para o estado desta nova conexao
    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("falha ao alocar estado da conexao\n");
        tcp_close(client_pcb); // Fecha o PCB do cliente se nao puder alocar estado
        return ERR_MEM;        // Retorna erro de memoria
    }
    con_state->pcb = client_pcb;        // Armazena o PCB do cliente no estado
    con_state->gw = &server_state->gw; // Passa o ponteiro para o IP do gateway do servidor

    // Registra os callbacks para esta conexao de cliente
    tcp_arg(client_pcb, con_state);                 // Associa con_state a este PCB
    tcp_sent(client_pcb, tcp_server_sent);          // Callback para dados enviados e ACKed
    tcp_recv(client_pcb, tcp_server_recv);          // Callback para dados recebidos
    tcp_poll(client_pcb, tcp_server_poll_cb, POLL_TIME_S * 2); // Callback de poll periodico
    tcp_err(client_pcb, tcp_server_err_cb);           // Callback para erros

    return ERR_OK; // Indica que a conexao foi aceita com sucesso
}

// Inicializa e abre o servidor TCP na porta definida por TCP_PORT.
static bool tcp_server_open(TCP_SERVER_T *server_state) { 
    DEBUG_printf("iniciando servidor na porta %d\n", TCP_PORT);

    // Cria um novo PCB para o tipo de IP "qualquer" (IPv4 ou IPv6 se habilitado)
    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("falha ao criar pcb\n");
        return false;
    }

    // Faz o bind do PCB a qualquer endereco IP local e a porta TCP_PORT
    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err != ERR_OK) { 
        DEBUG_printf("falha ao fazer bind na porta %d, erro %d\n", TCP_PORT, err);
        tcp_close(pcb); // Libera o PCB se o bind falhar
        return false;
    }

    // Coloca o PCB no estado de escuta (LISTEN), aguardando conexoes de entrada
    server_state->server_pcb = tcp_listen_with_backlog(pcb, 1); // '1' e o backlog
    if (!server_state->server_pcb) { 
        DEBUG_printf("falha ao escutar (listen)\n");
        // Nao precisa fechar 'pcb' aqui, pois tcp_listen_with_backlog ja o fez se falhou.
        return false;
    }

    // Registra o argumento (estado do servidor) e o callback para quando uma conexao for aceita
    tcp_arg(server_state->server_pcb, server_state); 
    tcp_accept(server_state->server_pcb, tcp_server_accept_cb); 

    return true; // Servidor aberto com sucesso
}

// Callback chamado quando ha caracteres disponiveis na entrada serial (USB).
void key_pressed_callback(void *param) {
    TCP_SERVER_T *server_state = (TCP_SERVER_T*)param; // Estado do servidor
    int key = getchar_timeout_us(0); // Le o caractere sem bloquear
    if (key == 'd' || key == 'D') { // Se a tecla 'd' ou 'D' for pressionada
        DEBUG_printf("Desabilitando modo AP por pressao de tecla...\n");
        cyw43_arch_disable_ap_mode(); // Desabilita o modo Access Point do chip Wi-Fi
        server_state->complete = true; // Sinaliza o loop principal para encerrar
    }
}

// Funcao principal do programa.
int main() {
    // Inicializa a E/S padrao (necessaria para printf e getchar_timeout_us)
    stdio_init_all();
    DEBUG_printf("Pico W Ponto de Acesso - JS Fetch 1s, Lista localStorage\n");

    // Inicializa o GPIO conectado ao LED
    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT); // Define o pino como saida
    gpio_put(LED_GPIO, 0);            // Comeca com o LED desligado

    // Inicializa o ADC para o sensor de temperatura interno
    adc_init();
    adc_set_temp_sensor_enabled(true); // Habilita o sensor de temperatura (usa canal ADC 4)

    // Aloca memoria para o estado do servidor TCP
    TCP_SERVER_T *server_state = calloc(1, sizeof(TCP_SERVER_T));
    if (!server_state) {
        DEBUG_printf("falha ao alocar server_state\n");
        return 1; // Erro critico
    }

    // Inicializa o chip Wi-Fi CYW43 (necessario antes de qualquer operacao Wi-Fi)
    if (cyw43_arch_init()) { 
        DEBUG_printf("falha ao inicializar CYW43\n");
        free(server_state);
        return 1; // Erro critico
    }

    // Configura um callback para ser chamado quando houver caracteres na entrada serial
    stdio_set_chars_available_callback(key_pressed_callback, server_state);

    // Define o nome da rede (SSID) e a senha para o Ponto de Acesso
    const char *ap_name = "picow_test"; 
    const char *password = "password"; 

    // Habilita o modo Access Point no chip Wi-Fi
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
    DEBUG_printf("Modo AP habilitado (requisicao enviada): SSID='%s'\n", ap_name);
    
    // Define o IP e a mascara de rede para o Ponto de Acesso
    const char *ap_ip_str = "192.168.4.1";
    const char *ap_mask_str = "255.255.255.0";
    ip4_addr_t ap_ip4, ap_mask4; // Estruturas IPv4 do LwIP

    // Converte as strings de IP e mascara para as estruturas do LwIP
    if (!ip4addr_aton(ap_ip_str, &ap_ip4)) { 
        DEBUG_printf("Falha ao parsear IP do AP: %s\n", ap_ip_str);
        free(server_state); cyw43_arch_deinit(); return 1;
    }
    if (!ip4addr_aton(ap_mask_str, &ap_mask4)) {
        DEBUG_printf("Falha ao parsear Mascara do AP: %s\n", ap_mask_str);
        free(server_state); cyw43_arch_deinit(); return 1;
    }

    // Converte as estruturas ip4_addr_t para o tipo mais geral ip_addr_t (usado pelo LwIP)
    ip_addr_set_ip4_u32(&server_state->gw, ip4_addr_get_u32(&ap_ip4)); // gw = gateway (IP do AP)
    ip_addr_t mask_addr; 
    ip_addr_set_ip4_u32(&mask_addr, ip4_addr_get_u32(&ap_mask4));      // mascara de rede

    // Inicializa o servidor DHCP
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &server_state->gw, &mask_addr); 
    DEBUG_printf("Servidor DHCP inicializado em %s\n", ipaddr_ntoa(&server_state->gw));

    // Inicializa o servidor DNS
    dns_server_t dns_server;
    dns_server_init(&dns_server, &server_state->gw); 
    DEBUG_printf("Servidor DNS inicializado, redirecionara para %s\n", ipaddr_ntoa(&server_state->gw));

    // Abre o servidor TCP para escutar por conexoes HTTP
    if (!tcp_server_open(server_state)) { 
        DEBUG_printf("falha ao abrir servidor TCP\n");
        // Limpeza em caso de falha
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_deinit();
        free(server_state);
        return 1;
    }
    DEBUG_printf("Servidor TCP aberto na porta %d\n", TCP_PORT);

    // Mensagens de instrucao para o usuario
    printf("Conecte-se ao Wi-Fi: %s, Senha: %s\n", ap_name, password);
    printf("Depois abra http://%s no seu navegador.\n", ipaddr_ntoa(&server_state->gw));
    printf("Pressione 'd' no terminal para desabilitar o AP e sair.\n");

    server_state->complete = false; // Flag para controlar o loop principal
    // Loop principal do programa
    while(!server_state->complete) {
        // Le e imprime a temperatura no terminal USB a cada segundo
        adc_select_input(4); 
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        printf("Temperatura interna atual (Terminal): %.2f C\n", temp_c); 
        
        sleep_ms(1000); // Pausa de 1 segundo
    }

    // Secao de limpeza ao encerrar o programa
    DEBUG_printf("Encerrando...\n");
    tcp_server_close(server_state);    // Fecha o servidor TCP
    dns_server_deinit(&dns_server);    // Desinicializa o servidor DNS
    dhcp_server_deinit(&dhcp_server);  // Desinicializa o servidor DHCP
    cyw43_arch_deinit();               // Desinicializa o chip Wi-Fi
    free(server_state);                // Libera a memoria do estado do servidor
    printf("Teste do Ponto de Acesso Completo.\n");
    return 0; // Encerra o programa
}
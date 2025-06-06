/**
 * Projeto: Servidor HTTP com controle de LED e Leitura de Temperatura via Access Point - Raspberry Pi Pico W
 * HTML com atualizacao dinamica da temperatura via JavaScript (Fetch API) a cada 1 segundo.
 * Lista de temperaturas capturadas ao ligar o LED, persistida com localStorage.
 */

#include <string.h>
#include <stdio.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/ip4_addr.h"

#include "dhcpserver.h"
#include "dnsserver.h"

#include "hardware/adc.h"

#define TCP_PORT 80
#define DEBUG_printf printf
#define POLL_TIME_S 5

#define HTTP_GET "GET"

// HTML principal - Intervalo do JavaScript atualizado para 1000ms (1 segundo)
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
    "  setInterval(fetchTemperaturaPeriodica, 1000); \n" \
    "});\n" \
    "</script>" \
    "</body></html>"

#define LED_PARAM "led=%d"
#define PATH_MAIN_PAGE "/"
#define PATH_API_TEMPERATURE "/api/temperatura"
#define LED_GPIO 13
#define HTTP_REDIRECT_HEADER_FORMAT "HTTP/1.1 302 Found\r\nLocation: http://%s/\r\nConnection: close\r\n\r\n"


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char response_header[256]; 
    char response_body[3072];   // Buffer para HTML + JS
    int header_len;
    int body_len;
    ip_addr_t *gw;
} TCP_CONNECT_STATE_T;


float convert_to_celsius(uint16_t raw) {
    const float conversion_factor = 3.3f / (1 << 12);
    float voltage = raw * conversion_factor;
    return 27.0f - (voltage - 0.706f) / 0.001721f;
}

static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("tcp_close falhou %d, chamando abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state) {
            free(con_state);
        }
    }
    return close_err;
}

static void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (!con_state) return ERR_OK; 

    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->body_len) {
        // DEBUG_printf("envio completo: %d bytes (cab: %d, corpo: %d)\n", con_state->sent_len, con_state->header_len, con_state->body_len);
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static void generate_server_response_content(TCP_CONNECT_STATE_T *con_state, const char *request_path, const char *params) {
    con_state->body_len = 0;
    con_state->header_len = 0;
    int http_status_code = 200;
    const char *http_status_text = "OK";
    char content_type_str[64] = "text/html; charset=utf-8";

    if (strcmp(request_path, PATH_MAIN_PAGE) == 0) {
        bool led_state_final = gpio_get(LED_GPIO); 

        if (params) { 
            int led_val_from_param;
            if (sscanf(params, LED_PARAM, &led_val_from_param) == 1) {
                gpio_put(LED_GPIO, led_val_from_param ? 1 : 0);
                led_state_final = led_val_from_param ? true : false; 
                DEBUG_printf("LED %s por requisicao\n", led_state_final ? "LIGADO" : "DESLIGADO");
            }
        }
        adc_select_input(4);
        uint16_t raw_temp = adc_read();
        float temp_c_atual = convert_to_celsius(raw_temp); 

        con_state->body_len = snprintf(con_state->response_body, sizeof(con_state->response_body),
                                       MAIN_PAGE_HTML,
                                       led_state_final ? "LIGADO" : "DESLIGADO",     
                                       led_state_final ? 0 : 1,                      
                                       led_state_final ? "DESLIGADO" : "LIGADO",     
                                       temp_c_atual,                                     
                                       led_state_final ? "true" : "false",        
                                       temp_c_atual);                                    
        
        if (con_state->body_len >= sizeof(con_state->response_body) -1 ) {
            DEBUG_printf("AVISO: Corpo HTML truncado! Necessario: %d, Buffer: %zu. Ajustando body_len.\n", con_state->body_len, sizeof(con_state->response_body));
            con_state->body_len = strlen(con_state->response_body); 
        }

    } else if (strcmp(request_path, PATH_API_TEMPERATURE) == 0) {
        adc_select_input(4);
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        // DEBUG_printf("API: Servindo temperatura: %.2f C\n", temp_c);
        con_state->body_len = snprintf(con_state->response_body, sizeof(con_state->response_body),
                                       "{\"temperatura\": %.2f}", temp_c);
        if (con_state->body_len >= sizeof(con_state->response_body) -1) {
             DEBUG_printf("AVISO: Corpo JSON truncado! Necessario: %d, Buffer: %zu. Ajustando body_len.\n", con_state->body_len, sizeof(con_state->response_body));
            con_state->body_len = strlen(con_state->response_body);
        }
        strcpy(content_type_str, "application/json; charset=utf-8");

    } else {
        http_status_code = 302; 
        http_status_text = "Found";
    }

    if (http_status_code == 302) {
         con_state->header_len = snprintf(con_state->response_header, sizeof(con_state->response_header),
                                     HTTP_REDIRECT_HEADER_FORMAT, ipaddr_ntoa(con_state->gw));
        con_state->body_len = 0; 
    } else {
        con_state->header_len = snprintf(con_state->response_header, sizeof(con_state->response_header),
                                   "HTTP/1.1 %d %s\r\n"
                                   "Content-Type: %s\r\n"
                                   "Content-Length: %d\r\n"
                                   "Connection: close\r\n\r\n",
                                   http_status_code, http_status_text,
                                   content_type_str,
                                   con_state->body_len); 

        if (con_state->header_len >= sizeof(con_state->response_header) -1) {
            DEBUG_printf("ERRO FATAL: Cabecalho HTTP truncado! Necessario: %d, Buffer: %zu\n", con_state->header_len, sizeof(con_state->response_header));
            con_state->header_len = 0; 
            con_state->body_len = 0;   
        }
    }
    // DEBUG_printf("Preparando resposta: Status %d, CT: %s, CL: %d, HL: %d\n",
    //              http_status_code, content_type_str, con_state->body_len, con_state->header_len);
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err_in) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    char client_request_buffer[128]; 

    if (err_in != ERR_OK) {
        DEBUG_printf("tcp_server_recv erro %d\n", err_in);
        if (p) pbuf_free(p);
        return tcp_close_client_connection(con_state, pcb, err_in);
    }

    if (!p) {
        DEBUG_printf("conexao fechada pelo cliente (p eh NULL)\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    
    if (!con_state) { 
        DEBUG_printf("ERRO: con_state eh NULL em tcp_server_recv\n");
        pbuf_free(p);
        tcp_abort(pcb); // Aborta o pcb se nao temos estado para ele
        return ERR_ABRT;
    }
    assert(con_state->pcb == pcb);

    if (p->tot_len > 0) {
        int req_len = pbuf_copy_partial(p, client_request_buffer, sizeof(client_request_buffer) - 1, 0);
        client_request_buffer[req_len] = '\0';

        if (strncmp(HTTP_GET, client_request_buffer, strlen(HTTP_GET)) == 0) {
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

            generate_server_response_content(con_state, request_path, params);

            if (con_state->header_len > 0) { 
                err_t write_err = tcp_write(pcb, con_state->response_header, con_state->header_len, TCP_WRITE_FLAG_COPY);
                if (write_err != ERR_OK) {
                    DEBUG_printf("falha ao escrever dados do cabecalho %d\n", write_err);
                    pbuf_free(p);
                    return tcp_close_client_connection(con_state, pcb, write_err);
                }

                if (con_state->body_len > 0) {
                    write_err = tcp_write(pcb, con_state->response_body, con_state->body_len, TCP_WRITE_FLAG_COPY);
                    if (write_err != ERR_OK) {
                        DEBUG_printf("falha ao escrever dados do corpo %d\n", write_err);
                        pbuf_free(p);
                        return tcp_close_client_connection(con_state, pcb, write_err);
                    }
                }
            } else {
                DEBUG_printf("ERRO: Cabecalho nao foi gerado, fechando conexao.\n");
                pbuf_free(p);
                return tcp_close_client_connection(con_state, pcb, ERR_VAL); 
            }
        }
        tcp_recved(pcb, p->tot_len); 
    }
    pbuf_free(p); 
    return ERR_OK;
}

static err_t tcp_server_poll_cb(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    return tcp_close_client_connection(con_state, pcb, ERR_OK); 
}

static void tcp_server_err_cb(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) { 
        DEBUG_printf("tcp_server_err_cb: erro %d\n", err);
        if (con_state) {
            free(con_state); 
        }
    } else {
        DEBUG_printf("tcp_server_err_cb: conexao abortada\n");
         if (con_state) {
            free(con_state); 
        }
    }
}

static err_t tcp_server_accept_cb(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *server_state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("falha no callback de accept: err %d\n", err);
        return ERR_VAL;
    }
    DEBUG_printf("cliente conectado de %s porta %u\n", ipaddr_ntoa(&client_pcb->remote_ip), client_pcb->remote_port);

    TCP_CONNECT_STATE_T *con_state = calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("falha ao alocar estado da conexao\n");
        tcp_close(client_pcb); 
        return ERR_MEM;
    }
    con_state->pcb = client_pcb;
    con_state->gw = &server_state->gw; 

    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll_cb, POLL_TIME_S * 2); 
    tcp_err(client_pcb, tcp_server_err_cb);

    return ERR_OK;
}

static bool tcp_server_open(TCP_SERVER_T *server_state) { 
    DEBUG_printf("iniciando servidor na porta %d\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("falha ao criar pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err != ERR_OK) { 
        DEBUG_printf("falha ao fazer bind na porta %d, erro %d\n", TCP_PORT, err);
        tcp_close(pcb); 
        return false;
    }

    server_state->server_pcb = tcp_listen_with_backlog(pcb, 1); 
    if (!server_state->server_pcb) {
        DEBUG_printf("falha ao escutar (listen)\n");
        if (pcb) { 
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(server_state->server_pcb, server_state); 
    tcp_accept(server_state->server_pcb, tcp_server_accept_cb); 

    return true;
}

void key_pressed_callback(void *param) {
    TCP_SERVER_T *server_state = (TCP_SERVER_T*)param;
    int key = getchar_timeout_us(0);
    if (key == 'd' || key == 'D') {
        DEBUG_printf("Desabilitando modo AP por pressao de tecla...\n");
        cyw43_arch_disable_ap_mode();
        server_state->complete = true;
    }
}

int main() {
    stdio_init_all();
    DEBUG_printf("Pico W Ponto de Acesso - JS Fetch 1s, Lista localStorage\n");

    gpio_init(LED_GPIO);
    gpio_set_dir(LED_GPIO, GPIO_OUT);
    gpio_put(LED_GPIO, 0); 

    adc_init();
    adc_set_temp_sensor_enabled(true);

    TCP_SERVER_T *server_state = calloc(1, sizeof(TCP_SERVER_T));
    if (!server_state) {
        DEBUG_printf("falha ao alocar server_state\n");
        return 1;
    }

    if (cyw43_arch_init()) { 
        DEBUG_printf("falha ao inicializar CYW43\n");
        free(server_state);
        return 1;
    }

    stdio_set_chars_available_callback(key_pressed_callback, server_state);

    const char *ap_name = "picow_test"; 
    const char *password = "password"; 

    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
    DEBUG_printf("Modo AP habilitado (requisicao enviada): SSID='%s'\n", ap_name);
    
    const char *ap_ip_str = "192.168.4.1";
    const char *ap_mask_str = "255.255.255.0";
    ip4_addr_t ap_ip4, ap_mask4;

    if (!ip4addr_aton(ap_ip_str, &ap_ip4)) {
        DEBUG_printf("Falha ao parsear IP do AP: %s\n", ap_ip_str);
        free(server_state); cyw43_arch_deinit(); return 1;
    }
    if (!ip4addr_aton(ap_mask_str, &ap_mask4)) {
        DEBUG_printf("Falha ao parsear Mascara do AP: %s\n", ap_mask_str);
        free(server_state); cyw43_arch_deinit(); return 1;
    }

    ip_addr_set_ip4_u32(&server_state->gw, ip4_addr_get_u32(&ap_ip4));
    ip_addr_t mask_addr; 
    ip_addr_set_ip4_u32(&mask_addr, ip4_addr_get_u32(&ap_mask4));

    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &server_state->gw, &mask_addr); 
    DEBUG_printf("Servidor DHCP inicializado em %s\n", ipaddr_ntoa(&server_state->gw));

    dns_server_t dns_server;
    dns_server_init(&dns_server, &server_state->gw); 
    DEBUG_printf("Servidor DNS inicializado, redirecionara para %s\n", ipaddr_ntoa(&server_state->gw));

    if (!tcp_server_open(server_state)) { 
        DEBUG_printf("falha ao abrir servidor TCP\n");
        dns_server_deinit(&dns_server);
        dhcp_server_deinit(&dhcp_server);
        cyw43_arch_deinit();
        free(server_state);
        return 1;
    }
    DEBUG_printf("Servidor TCP aberto na porta %d\n", TCP_PORT);
    printf("Conecte-se ao Wi-Fi: %s, Senha: %s\n", ap_name, password);
    printf("Depois abra http://%s no seu navegador.\n", ipaddr_ntoa(&server_state->gw));
    printf("Pressione 'd' no terminal para desabilitar o AP e sair.\n");

    server_state->complete = false;
    while(!server_state->complete) {
        adc_select_input(4); 
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        printf("Temperatura interna atual (Terminal): %.2f C\n", temp_c); 
        
        sleep_ms(1000); 
    }

    DEBUG_printf("Encerrando...\n");
    tcp_server_close(server_state);
    dns_server_deinit(&dns_server);
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_deinit();
    free(server_state);
    printf("Teste do Ponto de Acesso Completo.\n");
    return 0;
}
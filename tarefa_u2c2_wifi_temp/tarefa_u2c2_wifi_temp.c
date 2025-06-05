/**
 * Projeto: Servidor HTTP com controle de LED e Leitura de Temperatura via Access Point - Raspberry Pi Pico W
 * Baseado no exemplo picow_access_point_poll e adaptado para tarefa.
 *
 * Objetivos:
 * - Configurar o Raspberry Pi Pico W como um ponto de acesso (Access Point) Wi-Fi.
 * - Iniciar servidores DHCP e DNS locais.
 * - Criar um servidor HTTP embarcado que disponibiliza uma pagina HTML.
 * - Permitir o controle remoto de um LED (GPIO13) via comandos HTTP.
 * - Exibir a temperatura interna do RP2040 na pagina HTML e no terminal.
 * - Finalizacao controlada do modo Access Point via tecla 'd'.
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
#define DEBUG_printf printf // Todas as mensagens de debug usarao printf
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"

#define LED_AND_TEMP_BODY "<html><head><title>Pico W Controle</title></head><body><h1>Pico W Controle</h1><p>LED esta %s</p><p><a href=\"?led=%d\">Mudar LED para %s</a></p><hr><p>Temperatura Interna: %.2f °C</p></body></html>"

#define LED_PARAM "led=%d"
#define CONTROL_PAGE_PATH "/"
#define LED_GPIO 13
#define HTTP_RESPONSE_REDIRECT_ROOT "HTTP/1.1 302 Redirect\nLocation: http://%s/\n\n"


typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
} TCP_SERVER_T;

typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[128];
    char result[512];
    int header_len;
    int result_len;
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
    DEBUG_printf("tcp_server_sent %u bytes\n", len);
    if (!con_state) return ERR_OK;

    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        DEBUG_printf("envio completo: %d bytes no total (cabecalho %d, resultado %d)\n", con_state->sent_len, con_state->header_len, con_state->result_len);
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static int server_content_handler(const char *request_path, const char *params, char *result, size_t max_result_len) {
    int len = 0;
    if (strcmp(request_path, CONTROL_PAGE_PATH) == 0) {
        bool current_led_state = gpio_get(LED_GPIO);
        if (params) {
            int led_val_from_param;
            if (sscanf(params, LED_PARAM, &led_val_from_param) == 1) {
                gpio_put(LED_GPIO, led_val_from_param ? 1 : 0);
                current_led_state = led_val_from_param ? true : false;
                DEBUG_printf("LED %s por requisicao\n", current_led_state ? "LIGADO" : "DESLIGADO");
            }
        }

        adc_select_input(4);
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        DEBUG_printf("Estado atual LED: %s, Raw temp: %d, Temp C: %.2f\n", current_led_state ? "LIGADO" : "DESLIGADO", raw_temp, temp_c);

        if (current_led_state) {
            len = snprintf(result, max_result_len, LED_AND_TEMP_BODY, "LIGADO", 0, "DESLIGADO", temp_c);
        } else {
            len = snprintf(result, max_result_len, LED_AND_TEMP_BODY, "DESLIGADO", 1, "LIGADO", temp_c);
        }
    }
    return len;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err_in) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;

    if (err_in != ERR_OK) {
        DEBUG_printf("tcp_server_recv erro %d\n", err_in);
        if (p) pbuf_free(p);
        return tcp_close_client_connection(con_state, pcb, err_in);
    }

    if (!p) {
        DEBUG_printf("conexao fechada pelo cliente (p eh NULL)\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    
    assert(con_state && con_state->pcb == pcb);

    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d bytes do cliente\n", p->tot_len);

        pbuf_copy_partial(p, con_state->headers,
                          p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len,
                          0);
        con_state->headers[p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len] = '\0';

        if (strncmp(HTTP_GET, con_state->headers, strlen(HTTP_GET)) == 0) {
            char *request_uri_start = con_state->headers + strlen(HTTP_GET);
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

            DEBUG_printf("Caminho requisicao: '%s', Parametros: '%s'\n", request_path, params ? params : "null");

            con_state->result_len = server_content_handler(request_path, params, con_state->result, sizeof(con_state->result));

            if (con_state->result_len >= sizeof(con_state->result)) {
                DEBUG_printf("Muitos dados no resultado %d para buffer %zu\n", con_state->result_len, sizeof(con_state->result));
                pbuf_free(p);
                return tcp_close_client_connection(con_state, pcb, ERR_MEM); 
            }

            if (con_state->result_len > 0) {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                    200, con_state->result_len);
            } else {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT_ROOT,
                    ipaddr_ntoa(con_state->gw)); 
                DEBUG_printf("Enviando redirecionamento para raiz: %s", con_state->headers);
                con_state->result_len = 0; 
            }

             if (con_state->header_len >= sizeof(con_state->headers)) {
                DEBUG_printf("Muitos dados no cabecalho %d para buffer %zu\n", con_state->header_len, sizeof(con_state->headers));
                pbuf_free(p);
                return tcp_close_client_connection(con_state, pcb, ERR_MEM);
            }

            err_t write_err = tcp_write(pcb, con_state->headers, con_state->header_len, TCP_WRITE_FLAG_COPY);
            if (write_err != ERR_OK) {
                DEBUG_printf("falha ao escrever dados do cabecalho %d\n", write_err);
                pbuf_free(p);
                return tcp_close_client_connection(con_state, pcb, write_err);
            }

            if (con_state->result_len > 0) {
                write_err = tcp_write(pcb, con_state->result, con_state->result_len, TCP_WRITE_FLAG_COPY);
                if (write_err != ERR_OK) {
                    DEBUG_printf("falha ao escrever dados do resultado %d\n", write_err);
                    pbuf_free(p);
                    return tcp_close_client_connection(con_state, pcb, write_err);
                }
            }
        }
        tcp_recved(pcb, p->tot_len); 
    }
    pbuf_free(p); 
    return ERR_OK;
}

static err_t tcp_server_poll_cb(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_poll_cb para pcb %p\n", (void*)pcb);
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
        DEBUG_printf("falha no callback de accept: err %d, client_pcb eh %s\n", err, client_pcb ? "valido" : "NULL");
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
    DEBUG_printf("Pico W Ponto de Acesso - Controle de LED e Temperatura\n");

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
        // Adicionada leitura e impressão da temperatura no loop principal
        adc_select_input(4); // Seleciona o canal do sensor de temperatura
        uint16_t raw_temp = adc_read();
        float temp_c = convert_to_celsius(raw_temp);
        printf("Temperatura interna atual: %.2f C\n", temp_c);
        
        sleep_ms(1000); // Imprime a temperatura a cada 1 segundo
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
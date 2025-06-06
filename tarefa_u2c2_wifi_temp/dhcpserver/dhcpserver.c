/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018-2019 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

// For DHCP specs see:
//  https://www.ietf.org/rfc/rfc2131.txt
//  https://tools.ietf.org/html/rfc2132 -- DHCP Options and BOOTP Vendor Extensions

#include <stdio.h>      // Para printf (usado em logs)
#include <string.h>     // Para funcoes de manipulacao de memoria como memcpy, memset
#include <errno.h>      // Para definicoes de codigos de erro (ex: ENOMEM)

#include "cyw43_config.h" // Provavelmente para cyw43_hal_ticks_ms()
#include "dhcpserver.h"   // Header file para este modulo (definicoes de estruturas, etc.)
#include "lwip/udp.h"     // Para a API UDP do LwIP (udp_new, udp_bind, udp_sendto, udp_recv)

// --- Defines para Tipos de Mensagem DHCP ---
#define DHCPDISCOVER    (1) // Cliente procurando por servidores DHCP
#define DHCPOFFER       (2) // Servidor oferecendo um endereco IP
#define DHCPREQUEST     (3) // Cliente requisitando o endereco IP oferecido (ou um especifico)
#define DHCPDECLINE     (4) // Cliente indicando que o endereco IP oferecido ja esta em uso
#define DHCPACK         (5) // Servidor confirmando a alocacao do IP
#define DHCPNACK        (6) // Servidor negando a requisicao do cliente
#define DHCPRELEASE     (7) // Cliente liberando o endereco IP
#define DHCPINFORM      (8) // Cliente requisitando parametros locais (sem precisar de um IP novo)

// --- Defines para Opcoes DHCP (RFC 2132) ---
// Estas sao tags que identificam diferentes tipos de informacao na secao de 'options' da mensagem DHCP.
#define DHCP_OPT_PAD                (0)   // Padding, usado para alinhar opcoes
#define DHCP_OPT_SUBNET_MASK        (1)   // Mascara de sub-rede
#define DHCP_OPT_ROUTER             (3)   // Endereco do roteador/gateway
#define DHCP_OPT_DNS                (6)   // Enderecos dos servidores DNS
#define DHCP_OPT_HOST_NAME          (12)  // Nome do host do cliente
#define DHCP_OPT_REQUESTED_IP       (50)  // IP especifico requisitado pelo cliente
#define DHCP_OPT_IP_LEASE_TIME      (51)  // Tempo de concessao do IP (em segundos)
#define DHCP_OPT_MSG_TYPE           (53)  // Tipo da mensagem DHCP (DHCPDISCOVER, DHCPOFFER, etc.)
#define DHCP_OPT_SERVER_ID          (54)  // Identificador do servidor DHCP (geralmente seu IP)
#define DHCP_OPT_PARAM_REQUEST_LIST (55)  // Lista de parametros que o cliente gostaria de receber
#define DHCP_OPT_MAX_MSG_SIZE       (57)  // Tamanho maximo da mensagem DHCP que o cliente pode aceitar
#define DHCP_OPT_VENDOR_CLASS_ID    (60)  // Identificador da classe do vendor (fabricante) do cliente
#define DHCP_OPT_CLIENT_ID          (61)  // Identificador unico do cliente (ex: MAC address)
#define DHCP_OPT_END                (255) // Marca o fim da lista de opcoes

// --- Portas UDP para DHCP ---
#define PORT_DHCP_SERVER (67) // Servidor DHCP escuta na porta 67
#define PORT_DHCP_CLIENT (68) // Cliente DHCP escuta na porta 68 (servidor envia respostas para esta porta)

#define DEFAULT_LEASE_TIME_S (24 * 60 * 60) // Tempo de concessao padrao para um IP: 24 horas em segundos

#define MAC_LEN (6) // Comprimento de um endereco MAC em bytes
// Macro para construir um endereco IPv4 a partir de 4 bytes (nao usada neste arquivo diretamente)
#define MAKE_IP4(a, b, c, d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))

// Estrutura que representa uma mensagem DHCP (conforme RFC 2131)
typedef struct {
    uint8_t op;         // Opcode da mensagem: 1 para requisicao (BOOTREQUEST), 2 para resposta (BOOTREPLY)
    uint8_t htype;      // Tipo de endereco de hardware (ex: 1 para Ethernet 10Mb)
    uint8_t hlen;       // Comprimento do endereco de hardware (ex: 6 para Ethernet)
    uint8_t hops;       // Usado por agentes de relay (geralmente 0 para comunicacao direta)
    uint32_t xid;       // ID da transacao, um numero aleatorio escolhido pelo cliente, usado para parear requisicoes e respostas
    uint16_t secs;      // Segundos decorridos desde que o cliente comecou o processo de boot ou aquisicao de endereco
    uint16_t flags;     // Flags (ex: bit de broadcast)
    uint8_t ciaddr[4];  // Endereco IP do cliente (se ja tiver um e estiver renovando)
    uint8_t yiaddr[4];  // 'Seu' (do cliente) endereco IP (oferecido pelo servidor)
    uint8_t siaddr[4];  // Endereco IP do proximo servidor a ser usado no boot (ex: TFTP)
    uint8_t giaddr[4];  // Endereco IP do agente de relay (se houver)
    uint8_t chaddr[16]; // Endereco de hardware do cliente (ex: MAC address). Apenas os primeiros 'hlen' bytes sao usados.
    uint8_t sname[64];  // Nome do host do servidor (opcional)
    uint8_t file[128];  // Nome do arquivo de boot (opcional, para boot via rede)
    uint8_t options[312]; // Campo de opcoes variaveis. Comeca com um 'magic cookie' (99, 130, 83, 99).
} dhcp_msg_t;


// Funcao auxiliar para criar um novo socket UDP (usando LwIP)
static int dhcp_socket_new_dgram(struct udp_pcb **udp, void *cb_data, udp_recv_fn cb_udp_recv) {
    *udp = udp_new(); // Cria um novo PCB (Protocol Control Block) UDP
    if (*udp == NULL) { // Se a criacao falhar (ex: falta de memoria)
        return -ENOMEM; // Retorna erro de falta de memoria
    }
    // Registra a funcao de callback que sera chamada quando dados UDP forem recebidos neste PCB
    // 'cb_data' e um ponteiro para dados que serao passados para a funcao de callback
    udp_recv(*udp, cb_udp_recv, (void *)cb_data);
    return 0; // Sucesso
}

// Funcao auxiliar para liberar/fechar um socket UDP
static void dhcp_socket_free(struct udp_pcb **udp) {
    if (*udp != NULL) { // Se o PCB existir
        udp_remove(*udp); // Remove o PCB do LwIP (libera recursos)
        *udp = NULL;      // Define o ponteiro para NULL para evitar uso futuro
    }
}

// Funcao auxiliar para fazer o bind de um socket UDP a uma porta local
static int dhcp_socket_bind(struct udp_pcb **udp, uint16_t port) {
    // Faz o bind do PCB UDP para qualquer endereco IP local (IP_ANY_TYPE) e a porta especificada
    return udp_bind(*udp, IP_ANY_TYPE, port);
}

// Funcao auxiliar para enviar dados UDP
static int dhcp_socket_sendto(struct udp_pcb **udp, struct netif *nif, const void *buf, size_t len, uint32_t ip, uint16_t port) {
    if (len > 0xffff) { // Limita o tamanho maximo do datagrama UDP
        len = 0xffff;
    }

    // Aloca um pbuf (buffer de pacote) do LwIP para armazenar os dados a serem enviados
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, len, PBUF_RAM);
    if (p == NULL) { // Se a alocacao falhar
        return -ENOMEM; // Retorna erro de falta de memoria
    }

    memcpy(p->payload, buf, len); // Copia os dados do buffer da aplicacao para o pbuf

    ip_addr_t dest; // Estrutura para o endereco IP de destino
    // Converte o IP de destino (uint32_t) para o formato ip_addr_t do LwIP
    IP4_ADDR(ip_2_ip4(&dest), ip >> 24 & 0xff, ip >> 16 & 0xff, ip >> 8 & 0xff, ip & 0xff);
    
    err_t err; // Variavel para armazenar o resultado da operacao de envio
    if (nif != NULL) { // Se uma interface de rede especifica foi fornecida
        err = udp_sendto_if(*udp, p, &dest, port, nif); // Envia pela interface especificada
    } else { // Caso contrario, envia pela interface padrao/roteada
        err = udp_sendto(*udp, p, &dest, port);
    }

    pbuf_free(p); // Libera o pbuf apos o envio (ou tentativa de envio)

    if (err != ERR_OK) { // Se o envio falhou
        return err; // Retorna o codigo de erro do LwIP
    }
    return len; // Sucesso, retorna o numero de bytes enviados
}

// Funcao auxiliar para encontrar uma opcao especifica na secao de 'options' de uma mensagem DHCP
static uint8_t *opt_find(uint8_t *opt, uint8_t cmd) {
    // Itera pela lista de opcoes (cada opcao tem formato: [tag][comprimento][dados...])
    for (int i = 0; i < 308 && opt[i] != DHCP_OPT_END;) { // 308 e o tamanho maximo aproximado do campo de opcoes
        if (opt[i] == cmd) { // Se a tag da opcao atual corresponde a 'cmd' procurada
            return &opt[i];  // Retorna um ponteiro para o inicio desta opcao
        }
        i += 2 + opt[i + 1]; // Avanca para a proxima opcao (2 bytes para tag e comprimento + 'comprimento' bytes de dados)
    }
    return NULL; // Opcao nao encontrada
}

// Funcoes auxiliares para escrever opcoes DHCP no buffer de mensagem
// 'opt' e um ponteiro para um ponteiro para a posicao atual no buffer de opcoes.
// Ele e atualizado para apontar para depois da opcao escrita.

// Escreve uma opcao com 'n' bytes de dados
static void opt_write_n(uint8_t **opt, uint8_t cmd, size_t n, const void *data) {
    uint8_t *o = *opt; // Ponteiro para a posicao atual
    *o++ = cmd;        // Escreve a tag da opcao
    *o++ = n;          // Escreve o comprimento dos dados
    memcpy(o, data, n); // Copia os dados
    *opt = o + n;      // Atualiza o ponteiro para a proxima posicao livre
}

// Escreve uma opcao com 1 byte de dados (uint8_t)
static void opt_write_u8(uint8_t **opt, uint8_t cmd, uint8_t val) {
    uint8_t *o = *opt;
    *o++ = cmd;
    *o++ = 1; // Comprimento e 1
    *o++ = val;
    *opt = o;
}

// Escreve uma opcao com 4 bytes de dados (uint32_t), em network byte order (big-endian)
static void opt_write_u32(uint8_t **opt, uint8_t cmd, uint32_t val) {
    uint8_t *o = *opt;
    *o++ = cmd;
    *o++ = 4; // Comprimento e 4
    *o++ = val >> 24; // Byte mais significativo
    *o++ = val >> 16;
    *o++ = val >> 8;
    *o++ = val;       // Byte menos significativo
    *opt = o;
}

// Callback principal que processa as mensagens DHCP recebidas dos clientes.
// Esta funcao e registrada com udp_recv e chamada pelo LwIP quando um pacote UDP chega na porta do servidor DHCP.
static void dhcp_server_process(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t *src_addr, u16_t src_port) {
    dhcp_server_t *d = arg; // 'arg' e o ponteiro para a estrutura de estado do servidor DHCP (dhcp_server_t)
    (void)upcb;     // PCB do socket UDP que recebeu o pacote (nao usado diretamente aqui)
    (void)src_addr; // Endereco IP de origem do pacote (nao usado, pois DHCP geralmente usa broadcast ou IPs especificos)
    (void)src_port; // Porta de origem do pacote

    dhcp_msg_t dhcp_msg; // Estrutura para armazenar a mensagem DHCP recebida/a ser enviada

    #define DHCP_MIN_SIZE (240 + 3) // Tamanho minimo de uma mensagem DHCP valida (sem muitas opcoes)
    if (p->tot_len < DHCP_MIN_SIZE) { // Se o pacote recebido for muito pequeno
        goto ignore_request; // Ignora a requisicao
    }

    // Copia os dados do pbuf recebido para a estrutura dhcp_msg
    size_t len = pbuf_copy_partial(p, &dhcp_msg, sizeof(dhcp_msg), 0);
    if (len < DHCP_MIN_SIZE) { // Se a copia resultar em menos bytes que o minimo
        goto ignore_request; // Ignora
    }

    // Prepara a mensagem de resposta: Opcode sera DHCPOFFER ou DHCPACK
    dhcp_msg.op = DHCPOFFER; // Define inicialmente como DHCPOFFER, pode mudar para DHCPACK
    // Copia o IP do servidor (d->ip) para o campo 'yiaddr' (Your IP Address) da mensagem
    // Este sera o IP oferecido/confirmado ao cliente.
    memcpy(&dhcp_msg.yiaddr, &ip4_addr_get_u32(ip_2_ip4(&d->ip)), 4); // Copia os 3 primeiros bytes
                                                                    // O ultimo byte sera definido abaixo.

    uint8_t *opt = (uint8_t *)&dhcp_msg.options; // Ponteiro para o inicio da secao de opcoes
    opt += 4; // Pula o 'magic cookie' (0x63825363) que indica o inicio das opcoes DHCP

    // Encontra a opcao 'DHCP Message Type' na mensagem recebida
    uint8_t *msgtype = opt_find(opt, DHCP_OPT_MSG_TYPE);
    if (msgtype == NULL) { // Se a mensagem nao tiver o tipo definido
        goto ignore_request; // Ignora
    }

    // Processa a mensagem com base no seu tipo (o valor da opcao esta em msgtype[2])
    switch (msgtype[2]) {
        case DHCPDISCOVER: { // Cliente esta descobrindo servidores DHCP
            int yi = DHCPS_MAX_IP; // Indice para o array de leases (concessoes de IP)
                                   // Inicializado com um valor invalido para indicar que nenhum IP foi encontrado ainda.
            // Procura por um lease existente para este cliente (pelo MAC address) ou um lease livre/expirado
            for (int i = 0; i < DHCPS_MAX_IP; ++i) {
                if (memcmp(d->lease[i].mac, dhcp_msg.chaddr, MAC_LEN) == 0) { // Se o MAC do cliente ja tem um lease
                    yi = i; // Usa este indice
                    break;
                }
                // Se ainda nao encontrou um IP para este MAC, procura por um IP livre ou expirado
                if (yi == DHCPS_MAX_IP) { 
                    if (memcmp(d->lease[i].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) { // Se o lease esta livre (MAC zerado)
                        yi = i; // Marca este como candidato
                    }
                    // Verifica se o lease expirou (usando cyw43_hal_ticks_ms() para o tempo atual)
                    uint32_t expiry = d->lease[i].expiry << 16 | 0xffff; // Reconstrutor de tempo de expiracao
                    if ((int32_t)(expiry - cyw43_hal_ticks_ms()) < 0) { // Se o tempo de expiracao passou
                        memset(d->lease[i].mac, 0, MAC_LEN); // Libera o lease (zera o MAC)
                        yi = i; // Marca este como candidato
                    }
                }
            }
            if (yi == DHCPS_MAX_IP) { // Se nenhum IP estiver disponivel
                goto ignore_request; // Ignora
            }
            // Define o ultimo byte do endereco IP a ser oferecido
            // DHCPS_BASE_IP e o inicio da faixa de IPs gerenciada por este servidor (ex: 16 para 192.168.4.16)
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi; 
            // Prepara a resposta como DHCPOFFER
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPOFFER);
            break;
        }

        case DHCPREQUEST: { // Cliente esta requisitando um IP (confirmando uma oferta ou pedindo um IP especifico)
            // Procura pela opcao 'Requested IP Address' na requisicao do cliente
            uint8_t *o = opt_find(opt, DHCP_OPT_REQUESTED_IP);
            if (o == NULL) { // Se o cliente nao especificou qual IP quer
                goto ignore_request; // Ignora (ou poderia enviar NACK)
            }
            // Verifica se o IP requisitado pertence a este servidor (compara os 3 primeiros bytes do IP)
            if (memcmp(o + 2, &ip4_addr_get_u32(ip_2_ip4(&d->ip)), 3) != 0) {
                goto ignore_request; // IP nao e da nossa rede (ou poderia enviar NACK)
            }
            // Calcula o indice do lease com base no ultimo byte do IP requisitado
            uint8_t yi = o[5] - DHCPS_BASE_IP; 
            if (yi >= DHCPS_MAX_IP) { // Se o indice estiver fora da faixa gerenciada
                goto ignore_request; // Ignora (ou NACK)
            }

            // Verifica se o lease esta disponivel para este cliente
            if (memcmp(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN) == 0) { // Se o MAC ja corresponde ao lease (renovacao)
                // OK, pode usar
            } else if (memcmp(d->lease[yi].mac, "\x00\x00\x00\x00\x00\x00", MAC_LEN) == 0) { // Se o lease esta livre
                memcpy(d->lease[yi].mac, dhcp_msg.chaddr, MAC_LEN); // Associa o MAC do cliente ao lease
            } else { // IP ja esta em uso por outro MAC
                goto ignore_request; // Ignora (ou NACK)
            }
            // Atualiza o tempo de expiracao do lease
            d->lease[yi].expiry = (cyw43_hal_ticks_ms() + DEFAULT_LEASE_TIME_S * 1000) >> 16; // Armazena os bits mais significativos do tempo
            // Define o ultimo byte do IP a ser confirmado
            dhcp_msg.yiaddr[3] = DHCPS_BASE_IP + yi;
            // Prepara a resposta como DHCPACK (confirmacao)
            opt_write_u8(&opt, DHCP_OPT_MSG_TYPE, DHCPACK);
            // Loga a conexao do cliente
            printf("DHCPS: cliente conectado: MAC=%02x:%02x:%02x:%02x:%02x:%02x IP=%u.%u.%u.%u\n",
                dhcp_msg.chaddr[0], dhcp_msg.chaddr[1], dhcp_msg.chaddr[2], dhcp_msg.chaddr[3], dhcp_msg.chaddr[4], dhcp_msg.chaddr[5],
                dhcp_msg.yiaddr[0], dhcp_msg.yiaddr[1], dhcp_msg.yiaddr[2], dhcp_msg.yiaddr[3]);
            break;
        }

        default: // Outros tipos de mensagem DHCP nao sao tratados por este servidor simples
            goto ignore_request;
    }

    // Adiciona outras opcoes DHCP a resposta:
    opt_write_n(&opt, DHCP_OPT_SERVER_ID, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip)));       // IP do servidor DHCP
    opt_write_n(&opt, DHCP_OPT_SUBNET_MASK, 4, &ip4_addr_get_u32(ip_2_ip4(&d->nm)));     // Mascara de sub-rede
    opt_write_n(&opt, DHCP_OPT_ROUTER, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip)));          // Roteador/Gateway (o proprio servidor)
    opt_write_n(&opt, DHCP_OPT_DNS, 4, &ip4_addr_get_u32(ip_2_ip4(&d->ip)));             // Servidor DNS (o proprio servidor)
    opt_write_u32(&opt, DHCP_OPT_IP_LEASE_TIME, DEFAULT_LEASE_TIME_S);                   // Tempo de concessao do IP
    *opt++ = DHCP_OPT_END; // Marca o fim das opcoes

    // Envia a mensagem DHCP de resposta (DHCPOFFER ou DHCPACK)
    // Obtem a interface de rede pela qual a requisicao foi recebida
    struct netif *nif = ip_current_input_netif(); 
    // Envia a mensagem para o endereco de broadcast (0xffffffff) na porta do cliente DHCP
    dhcp_socket_sendto(&d->udp, nif, &dhcp_msg, opt - (uint8_t *)&dhcp_msg, 0xffffffff, PORT_DHCP_CLIENT);

ignore_request: // Label para pular o processamento e apenas liberar o pbuf
    pbuf_free(p); // Libera o pbuf da mensagem recebida
}

// Inicializa o servidor DHCP.
void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm) {
    // 'd' e a estrutura de estado do servidor DHCP
    // 'ip' e o endereco IP deste servidor DHCP
    // 'nm' e a mascara de sub-rede da rede que este servidor ira gerenciar
    
    ip_addr_copy(d->ip, *ip); // Copia o IP do servidor para o estado
    ip_addr_copy(d->nm, *nm); // Copia a mascara de sub-rede para o estado
    memset(d->lease, 0, sizeof(d->lease)); // Zera a tabela de leases (concessoes de IP)

    // Cria e configura o socket UDP para o servidor DHCP
    if (dhcp_socket_new_dgram(&d->udp, d, dhcp_server_process) != 0) { // 'd' e passado como argumento para o callback
        return; // Falha ao criar socket
    }
    dhcp_socket_bind(&d->udp, PORT_DHCP_SERVER); // Faz o bind na porta do servidor DHCP (67)
}

// Desinicializa o servidor DHCP.
void dhcp_server_deinit(dhcp_server_t *d) {
    dhcp_socket_free(&d->udp); // Libera o socket UDP
}
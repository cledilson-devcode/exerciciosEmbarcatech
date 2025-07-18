/* 
    // ARQUIVO DHCPSERVER.H

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
#ifndef MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H // Guarda de inclusao para evitar multiplas definicoes
#define MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H

#include "lwip/ip_addr.h" // Necessario para o tipo ip_addr_t (endereco IP do LwIP)

// Define o byte inicial da faixa de IPs que o servidor DHCP pode alocar.
// Ex: Se o IP do servidor for 192.168.4.1, os IPs alocados comecarao em 192.168.4.16.
#define DHCPS_BASE_IP (16)

// Define o numero maximo de enderecos IP que este servidor DHCP pode gerenciar/alocar.
#define DHCPS_MAX_IP (8)

// Estrutura para representar uma concessao (lease) de IP individual.
typedef struct _dhcp_server_lease_t {
    uint8_t mac[6];   // Endereco MAC do cliente que recebeu este lease.
    uint16_t expiry;  // Informacao sobre o tempo de expiracao do lease (formato depende da implementacao).
} dhcp_server_lease_t;

// Estrutura principal para armazenar o estado do servidor DHCP.
typedef struct _dhcp_server_t {
    ip_addr_t ip;  // Endereco IP do proprio servidor DHCP.
    ip_addr_t nm;  // Mascara de sub-rede da rede que este servidor esta gerenciando.
    // Array para armazenar as concessoes (leases) de IP ativas.
    // O tamanho do array e definido por DHCPS_MAX_IP.
    dhcp_server_lease_t lease[DHCPS_MAX_IP];
    struct udp_pcb *udp; // Ponteiro para o Protocol Control Block (PCB) UDP do LwIP,
                         // usado para a comunicacao de rede do servidor DHCP.
} dhcp_server_t;

// Prototipo da funcao para inicializar o servidor DHCP.
// 'd' e um ponteiro para a estrutura de estado do servidor a ser inicializada.
// 'ip' e o endereco IP do servidor.
// 'nm' e a mascara de sub-rede.
void dhcp_server_init(dhcp_server_t *d, ip_addr_t *ip, ip_addr_t *nm);

// Prototipo da funcao para desinicializar o servidor DHCP e liberar recursos.
// 'd' e um ponteiro para a estrutura de estado do servidor a ser desinicializada.
void dhcp_server_deinit(dhcp_server_t *d);

#endif // MICROPY_INCLUDED_LIB_NETUTILS_DHCPSERVER_H
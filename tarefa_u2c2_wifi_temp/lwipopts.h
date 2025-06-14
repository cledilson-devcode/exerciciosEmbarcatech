// ARQUIVO LWIPOPTS.h
#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// Configuracoes comuns para exemplos do Pico W
// Veja https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html para detalhes

// Permite sobrescrever em alguns exemplos
#ifndef NO_SYS
#define NO_SYS                      1 
#endif
// Permite sobrescrever em alguns exemplos
#ifndef LWIP_SOCKET
#define LWIP_SOCKET                 0 
#endif

// --- Gerenciamento de Memoria ---
#if PICO_CYW43_ARCH_POLL
#define MEM_LIBC_MALLOC             1 // 1: LwIP usa malloc/free da biblioteca C padrao.
#else

#define MEM_LIBC_MALLOC             0 // 0: LwIP usa seu proprio gerenciador de heap (MEM_SIZE) ou o do RTOS.
#endif

#define MEM_ALIGNMENT               4 // Alinhamento de memoria em bytes (4 para processadores 32-bit).
#define MEM_SIZE                    (16 * 1024) // Tamanho total do heap do LwIP (em bytes) se MEM_LIBC_MALLOC=0. Aumentado para 16KB.
#define MEMP_NUM_TCP_SEG            64 // Numero de segmentos TCP que podem ser enfileirados/armazenados. Aumentado.
#define MEMP_NUM_ARP_QUEUE          10 // Tamanho da fila para pacotes ARP esperando resolucao.
#define PBUF_POOL_SIZE              48 // Numero de pbufs (buffers de pacote) na pool principal. Aumentado.

// --- Protocolos de Rede Habilitados ---
#define LWIP_ARP                    1 // 1: Habilita o protocolo ARP (Address Resolution Protocol).
#define LWIP_ETHERNET               1 // 1: Suporte para quadros Ethernet (necessario para Wi-Fi).
#define LWIP_ICMP                   1 // 1: Habilita o protocolo ICMP (Internet Control Message Protocol, ex: ping).
#define LWIP_RAW                    1 // 1: Habilita suporte para sockets RAW (nao usado diretamente aqui).
#define LWIP_DHCP                   1 // 1: Habilita o cliente DHCP do LwIP (para obter IP automaticamente).
#define LWIP_IPV4                   1 // 1: Habilita o protocolo IPv4.
#define LWIP_TCP                    1 // 1: Habilita o protocolo TCP.
#define LWIP_UDP                    1 // 1: Habilita o protocolo UDP.
#define LWIP_DNS                    1 // 1: Habilita o cliente DNS do LwIP (para resolver nomes de dominio).

// --- Configuracoes TCP ---
#define TCP_WND                     (8 * TCP_MSS) // Tamanho da janela de recepcao TCP (quanto pode receber antes de ACK).
#define TCP_MSS                     1460          // Tamanho Maximo do Segmento TCP (payload maximo de um pacote TCP).
#define TCP_SND_BUF                 (8 * TCP_MSS) // Tamanho do buffer de envio por conexao TCP.
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS)) // Maximo de pbufs na fila de envio TCP.
#define LWIP_TCP_KEEPALIVE          1 // 1: Habilita pacotes TCP keepalive para detectar conexoes mortas.

// --- Configuracoes de Interface de Rede (Netif) ---
#define LWIP_NETIF_STATUS_CALLBACK  1 // 1: Habilita callback para mudancas de status da interface.
#define LWIP_NETIF_LINK_CALLBACK    1 // 1: Habilita callback para mudancas de status do link (ex: cabo conectado/desconectado).
#define LWIP_NETIF_HOSTNAME         1 // 1: Permite definir um nome de host para o dispositivo.
#define LWIP_NETIF_TX_SINGLE_PBUF   1 // 1: Otimizacao para transmitir um unico pbuf por vez (comum em embarcados).

// --- APIs LwIP ---
#define LWIP_NETCONN                0 // 0: Desabilita a API Netconn (sequencial, mais alto nivel que RAW).
                                      // Estamos usando API RAW (callbacks).

// --- Estatisticas e Debug (geralmente desabilitados para producao para economizar espaco/performance) ---
#define MEM_STATS                   0 // 0: Desabilita estatisticas de uso do heap principal. (1 para habilitar).
#define SYS_STATS                   0 // 0: Desabilita estatisticas do sistema operacional (se NO_SYS=0).
#define MEMP_STATS                  0 // 0: Desabilita estatisticas de uso das pools de memoria fixa (MEMP).
#define LINK_STATS                  0 // 0: Desabilita estatisticas da camada de enlace.

// --- Diversos ---
// #define ETH_PAD_SIZE                2 // Padding para alinhar o cabecalho IP em quadros Ethernet. Geralmente 0 ou 2.
                                      // Para CYW43, o driver geralmente lida com isso.
#define LWIP_CHKSUM_ALGORITHM       3 // Algoritmo de checksum (3 e geralmente otimizado).
#define DHCP_DOES_ARP_CHECK         0 // 0: Cliente DHCP nao faz verificacao ARP antes de usar um IP.
#define LWIP_DHCP_DOES_ACD_CHECK    0 // 0: Cliente DHCP nao faz Address Conflict Detection.

// --- Configuracoes de Debug por Modulo (habilitadas se LWIP_DEBUG=1) ---
#ifndef NDEBUG // Se NDEBUG nao estiver definido (ou seja, estamos em modo Debug)
#define LWIP_DEBUG                  1 // Habilita a compilacao de codigo de debug do LwIP.
#define LWIP_STATS                  1 // Habilita a compilacao de codigo de estatisticas.
#define LWIP_STATS_DISPLAY          1 // Habilita funcoes para exibir estatisticas.
#else // Se NDEBUG estiver definido (modo Release)
#define LWIP_DEBUG                  0 // Desabilita codigo de debug.
#define LWIP_STATS                  0 // Desabilita codigo de estatisticas.
#define LWIP_STATS_DISPLAY          0 // Desabilita funcoes de exibicao de estatisticas.
#endif

// Define o nivel de debug para cada modulo. LWIP_DBG_OFF desabilita, LWIP_DBG_ON habilita.
// Outros niveis: LWIP_DBG_LEVEL_WARNING, LWIP_DBG_LEVEL_SERIOUS, LWIP_DBG_LEVEL_SEVERE.
#define ETHARP_DEBUG                LWIP_DBG_OFF
#define NETIF_DEBUG                 LWIP_DBG_OFF
#define PBUF_DEBUG                  LWIP_DBG_OFF // Habilitar para depurar problemas com pbufs.
#define API_LIB_DEBUG               LWIP_DBG_OFF
#define API_MSG_DEBUG               LWIP_DBG_OFF
#define SOCKETS_DEBUG               LWIP_DBG_OFF
#define ICMP_DEBUG                  LWIP_DBG_OFF
#define INET_DEBUG                  LWIP_DBG_OFF
#define IP_DEBUG                    LWIP_DBG_OFF
#define IP_REASS_DEBUG              LWIP_DBG_OFF
#define RAW_DEBUG                   LWIP_DBG_OFF
#define MEM_DEBUG                   LWIP_DBG_OFF // Habilitar para depurar problemas de alocacao de memoria.
#define MEMP_DEBUG                  LWIP_DBG_OFF // Habilitar para depurar problemas com pools de memoria fixa.
#define SYS_DEBUG                   LWIP_DBG_OFF
#define TCP_DEBUG                   LWIP_DBG_OFF // Habilitar para depurar o protocolo TCP.
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF // Habilitar para ver o que o TCP esta tentando enviar.
#define TCP_RTO_DEBUG               LWIP_DBG_OFF
#define TCP_CWND_DEBUG              LWIP_DBG_OFF
#define TCP_WND_DEBUG               LWIP_DBG_OFF
#define TCP_FR_DEBUG                LWIP_DBG_OFF
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF
#define TCP_RST_DEBUG               LWIP_DBG_OFF
#define UDP_DEBUG                   LWIP_DBG_OFF
#define TCPIP_DEBUG                 LWIP_DBG_OFF // Debug da thread principal TCPIP (se NO_SYS=0).
#define PPP_DEBUG                   LWIP_DBG_OFF
#define SLIP_DEBUG                  LWIP_DBG_OFF
#define DHCP_DEBUG                  LWIP_DBG_OFF
// #define DNS_DEBUG                   LWIP_DBG_ON // Exemplo de como habilitar debug para DNS.

// --- Configuracoes para FreeRTOS (se NO_SYS=0, como em threadsafe_background) ---
#if NO_SYS == 0
#define TCPIP_THREAD_STACKSIZE      1024        // Tamanho da stack para a thread principal do LwIP.
#define TCPIP_THREAD_PRIO           (configMAX_PRIORITIES - 2) // Prioridade da thread LwIP (exemplo para FreeRTOS).
#define TCPIP_MBOX_SIZE             12           // Tamanho da mailbox principal da thread TCPIP. Aumentado.
#define DEFAULT_TCP_RECVMBOX_SIZE   12           // Tamanho da mailbox para segmentos TCP recebidos por conexao. Aumentado.
#define DEFAULT_UDP_RECVMBOX_SIZE   12           // Tamanho da mailbox para datagramas UDP recebidos por conexao. Aumentado.
#define DEFAULT_ACCEPTMBOX_SIZE     12           // Tamanho da mailbox para novas conexoes TCP aceitas. Aumentado.
#endif

#endif /* __LWIPOPTS_H__ */
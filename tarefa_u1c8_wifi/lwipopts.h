// lwipopts.h - Opções de configuração para a pilha de rede LwIP no Raspberry Pi Pico W

#ifndef __LWIPOPTS_H__
#define __LWIPOPTS_H__

// --- Configurações Comuns da LwIP para Exemplos do Pico W ---
// Para detalhes sobre cada opção, consulte a documentação oficial da LwIP:
// https://www.nongnu.org/lwip/2_1_x/group__lwip__opts.html

// NO_SYS: Define se a LwIP roda com um sistema operacional (0) ou em modo "bare-metal" (1).
// 1 significa que não há um sistema operacional multitarefa gerenciando a LwIP;
// o próprio programa principal (ou interrupções) é responsável por chamar as funções da LwIP.
#ifndef NO_SYS // Permite sobrescrever esta opção em exemplos específicos
#define NO_SYS                      1
#endif

// LWIP_SOCKET: Habilita (1) ou desabilita (0) a API de sockets compatível com Berkeley/POSIX.
// Se 0, usa-se a API nativa "raw" da LwIP, que é mais leve.
#ifndef LWIP_SOCKET // Permite sobrescrever
#define LWIP_SOCKET                 0
#endif

// MEM_LIBC_MALLOC: Define se a LwIP usará as funções malloc/free da biblioteca C padrão (1)
// ou o gerenciador de memória interno da LwIP (0).
#if PICO_CYW43_ARCH_POLL // Se o driver Wi-Fi do Pico estiver em modo polling
#define MEM_LIBC_MALLOC             1 // Pode usar malloc/free da libc
#else
// Em modo não-polling (baseado em interrupções), usar malloc/free da libc pode não ser seguro
// devido a possíveis problemas de reentrância, então usa-se o gerenciador da LwIP.
#define MEM_LIBC_MALLOC             0
#endif

#define MEM_ALIGNMENT               4 // Alinhamento de memória em bytes (geralmente 4 para ARM 32-bit)
#define MEM_SIZE                    4000 // Tamanho total da heap de memória gerenciada pela LwIP (se MEM_LIBC_MALLOC=0)

// MEMP_NUM_TCP_SEG: Número de segmentos TCP que podem ser enfileirados para transmissão.
// Afeta o buffer de saída TCP.
#define MEMP_NUM_TCP_SEG            32

// MEMP_NUM_ARP_QUEUE: Número de pacotes que podem ser enfileirados esperando resolução ARP.
#define MEMP_NUM_ARP_QUEUE          10

// PBUF_POOL_SIZE: Número de pbufs (buffers de pacotes) no pool principal da LwIP.
// pbufs são usados para armazenar dados de pacotes de entrada e saída.
#define PBUF_POOL_SIZE              24

// LWIP_ARP: Habilita (1) ou desabilita (0) o protocolo ARP (Address Resolution Protocol).
// Essencial para redes Ethernet/Wi-Fi para mapear IPs para endereços MAC.
#define LWIP_ARP                    1

// LWIP_ETHERNET: Habilita (1) o suporte genérico para interfaces do tipo Ethernet.
// Necessário para comunicação via Wi-Fi, que opera na camada de enlace de dados.
#define LWIP_ETHERNET               1

// LWIP_ICMP: Habilita (1) o protocolo ICMP (Internet Control Message Protocol).
// Usado para mensagens de erro e diagnóstico (ex: ping).
#define LWIP_ICMP                   1

// LWIP_RAW: Habilita (1) a API "raw" da LwIP, permitindo enviar/receber pacotes IP diretamente.
#define LWIP_RAW                    1

// TCP_WND: Tamanho da janela de recepção TCP (em bytes).
// É o quanto de dados o receptor pode aceitar antes de enviar uma confirmação (ACK).
// (8 * TCP_MSS) significa 8 segmentos de tamanho máximo.
#define TCP_WND                     (8 * TCP_MSS)

// TCP_MSS: Tamanho Máximo do Segmento TCP (Maximum Segment Size).
// Quantidade máxima de dados que pode ser enviada em um único segmento TCP.
// 1460 é um valor comum para Ethernet.
#define TCP_MSS                     1460

// TCP_SND_BUF: Tamanho do buffer de envio TCP (em bytes).
// Quanto de dados pode ser enfileirado para envio antes de bloquear.
#define TCP_SND_BUF                 (8 * TCP_MSS)

// TCP_SND_QUEUELEN: Comprimento da fila de envio TCP (em número de pbufs).
// Relacionado a TCP_SND_BUF e TCP_MSS.
#define TCP_SND_QUEUELEN            ((4 * (TCP_SND_BUF) + (TCP_MSS - 1)) / (TCP_MSS))

// LWIP_NETIF_STATUS_CALLBACK: Habilita (1) callbacks para mudança de status da interface de rede (ex: IP atribuído).
#define LWIP_NETIF_STATUS_CALLBACK  1

// LWIP_NETIF_LINK_CALLBACK: Habilita (1) callbacks para mudança de status do link da interface (ex: link up/down).
#define LWIP_NETIF_LINK_CALLBACK    1

// LWIP_NETIF_HOSTNAME: Habilita (1) o suporte para definir um nome de host (hostname) para a interface.
#define LWIP_NETIF_HOSTNAME         1

// LWIP_NETCONN: Habilita (0) a API Netconn, uma API sequencial de mais alto nível sobre a API raw.
// Desabilitada (0) se LWIP_SOCKET também estiver desabilitada e NO_SYS=1, para economizar recursos.
#define LWIP_NETCONN                0

// Opções para habilitar estatísticas internas da LwIP (geralmente para debug).
#define MEM_STATS                   0 // Estatísticas do gerenciador de memória (heap)
#define SYS_STATS                   0 // Estatísticas do sistema (semáforos, mutexes, se NO_SYS=0)
#define MEMP_STATS                  0 // Estatísticas dos pools de memória fixa (memp)
#define LINK_STATS                  0 // Estatísticas da camada de enlace

// ETH_PAD_SIZE: Preenchimento adicionado ao cabeçalho Ethernet para alinhamento.
// Comentado pois geralmente é tratado pelo driver da interface.
// #define ETH_PAD_SIZE                2

// LWIP_CHKSUM_ALGORITHM: Define o algoritmo de checksum.
// 3 = Algoritmo otimizado que processa 4 bytes por vez e depois os bytes restantes.
#define LWIP_CHKSUM_ALGORITHM       3

// LWIP_DHCP: Habilita (1) o cliente DHCP para obter configuração de IP automaticamente.
#define LWIP_DHCP                   1

// LWIP_IPV4: Habilita (1) o suporte ao protocolo IPv4.
#define LWIP_IPV4                   1

// LWIP_TCP: Habilita (1) o suporte ao protocolo TCP.
#define LWIP_TCP                    1

// LWIP_UDP: Habilita (1) o suporte ao protocolo UDP.
#define LWIP_UDP                    1

// LWIP_DNS: Habilita (1) o cliente DNS para resolução de nomes de domínio.
#define LWIP_DNS                    1

// LWIP_TCP_KEEPALIVE: Habilita (1) o envio de pacotes TCP keepalive para manter conexões ativas.
#define LWIP_TCP_KEEPALIVE          1

// LWIP_NETIF_TX_SINGLE_PBUF: Otimização para transmissão; se habilitado (1), a função de saída da
// interface de rede (linkoutput) recebe um único pbuf em vez de uma cadeia de pbufs.
// Simplifica o driver da interface, mas pode exigir cópia de dados se o MTU for excedido.
#define LWIP_NETIF_TX_SINGLE_PBUF   1

// DHCP_DOES_ARP_CHECK: Se 0, o cliente DHCP não fará uma verificação ARP pelo endereço IP
// oferecido antes de aceitá-lo. Desabilitar pode acelerar a obtenção do IP.
#define DHCP_DOES_ARP_CHECK         0

// LWIP_DHCP_DOES_ACD_CHECK: Se 0, o cliente DHCP não fará uma verificação de conflito de endereço (ACD)
// usando ARP Probe. Similar ao anterior, pode acelerar a configuração.
#define LWIP_DHCP_DOES_ACD_CHECK    0


// --- Configurações de Debug da LwIP ---
// Estas opções habilitam mensagens de debug para diferentes módulos da LwIP.
// LWIP_DEBUG habilita globalmente o sistema de debug.
// LWIP_STATS e LWIP_STATS_DISPLAY habilitam e mostram estatísticas se NDEBUG não estiver definido.
#ifndef NDEBUG // Se não for uma compilação de "Release" (NDEBUG não definido)
#define LWIP_DEBUG                  1 // Habilita sistema de debug LwIP
#define LWIP_STATS                  1 // Habilita coleta de estatísticas LwIP
#define LWIP_STATS_DISPLAY          1 // Habilita exibição de estatísticas
#endif

// Níveis de debug para módulos específicos (LWIP_DBG_OFF desabilita debug para o módulo).
// Outros níveis podem ser LWIP_DBG_ON, LWIP_DBG_MIN_LEVEL, etc.
#define ETHARP_DEBUG                LWIP_DBG_OFF // Debug do ARP e da camada Ethernet
#define NETIF_DEBUG                 LWIP_DBG_OFF // Debug das interfaces de rede
#define PBUF_DEBUG                  LWIP_DBG_OFF // Debug dos buffers de pacotes (pbuf)
#define API_LIB_DEBUG               LWIP_DBG_OFF // Debug da API de sockets (se habilitada)
#define API_MSG_DEBUG               LWIP_DBG_OFF // Debug das mensagens da API (se Netconn habilitada)
#define SOCKETS_DEBUG               LWIP_DBG_OFF // Debug específico da API de sockets
#define ICMP_DEBUG                  LWIP_DBG_OFF // Debug do ICMP
#define INET_DEBUG                  LWIP_DBG_OFF // Debug de funções relacionadas a endereços (inet_addr, etc.)
#define IP_DEBUG                    LWIP_DBG_OFF // Debug da camada IP
#define IP_REASS_DEBUG              LWIP_DBG_OFF // Debug da remontagem de fragmentos IP
#define RAW_DEBUG                   LWIP_DBG_OFF // Debug dos PCBs raw
#define MEM_DEBUG                   LWIP_DBG_OFF // Debug do gerenciador de memória (heap)
#define MEMP_DEBUG                  LWIP_DBG_OFF // Debug dos pools de memória fixa
#define SYS_DEBUG                   LWIP_DBG_OFF // Debug da camada de abstração do SO (se NO_SYS=0)
#define TCP_DEBUG                   LWIP_DBG_OFF // Debug geral do TCP
#define TCP_INPUT_DEBUG             LWIP_DBG_OFF // Debug do processamento de entrada TCP
#define TCP_OUTPUT_DEBUG            LWIP_DBG_OFF // Debug do processamento de saída TCP
#define TCP_RTO_DEBUG               LWIP_DBG_OFF // Debug do cálculo de Retransmission Timeout (RTO)
#define TCP_CWND_DEBUG              LWIP_DBG_OFF // Debug da janela de congestionamento (CWND)
#define TCP_WND_DEBUG               LWIP_DBG_OFF // Debug da janela de recepção/envio
#define TCP_FR_DEBUG                LWIP_DBG_OFF // Debug da recuperação rápida (Fast Retransmit/Recovery)
#define TCP_QLEN_DEBUG              LWIP_DBG_OFF // Debug do tamanho das filas TCP
#define TCP_RST_DEBUG               LWIP_DBG_OFF // Debug de pacotes TCP RST (reset)
#define UDP_DEBUG                   LWIP_DBG_OFF // Debug do UDP
#define TCPIP_DEBUG                 LWIP_DBG_OFF // Debug do thread principal tcpip_thread (se NO_SYS=0)
#define PPP_DEBUG                   LWIP_DBG_OFF // Debug do PPP (Point-to-Point Protocol)
#define SLIP_DEBUG                  LWIP_DBG_OFF // Debug do SLIP (Serial Line IP)
#define DHCP_DEBUG                  LWIP_DBG_OFF // Debug do cliente DHCP

#endif /* __LWIPOPTS_H__ */
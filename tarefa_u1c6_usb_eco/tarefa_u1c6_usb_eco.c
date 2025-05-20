/**
 * @file main.c
 * @brief Dispositivo CDC (USB) responsivo com identificação visual para BitDogLab.
 *        Tarefa Unidade 1, Capítulo 6.
 *        Recebe comandos de cor via serial USB, faz eco e acende o LED correspondente.
 */

#include <stdio.h>
#include <string.h>     // Para strcmp, strlen, strcspn
#include "pico/stdlib.h"
#include "tusb.h"       // Biblioteca TinyUSB

// --- Definições de Pinos dos LEDs (Conforme BitDogLab e Tarefa) ---
#define LED_PIN_VERMELHO 13
#define LED_PIN_VERDE    11
#define LED_PIN_AZUL     12

// --- Comandos Esperados ---
const char *CMD_VERMELHO = "vermelho";
const char *CMD_VERDE    = "verde";
const char *CMD_AZUL     = "azul";


#define BOARD_TUD_RHPORT 0


// ==========================================================================
// --- Protótipos de Função (Assinaturas) ---
// ==========================================================================
void configurar_hardware_inicial();
void processar_comando_cdc(char *buffer, uint32_t tamanho_buffer);
void controlar_led_por_comando(const char *comando);
void apagar_todos_os_leds();

// Callbacks TinyUSB (opcionais para funcionalidade básica, mas boas práticas)
void tud_cdc_mount_cb(uint8_t itf);
void tud_cdc_umount_cb(uint8_t itf);
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding);

// ==========================================================================
// --- Função Principal ---
// ==========================================================================
int main() {
    // Inicializa stdio e hardware básico (LEDs)
    configurar_hardware_inicial();

    // Inicializa a pilha TinyUSB
    tud_init(BOARD_TUD_RHPORT);

    printf("Dispositivo CDC Eco com LEDs. Aguardando conexão USB...\n");

    // Loop principal
    while (true) {
        tud_task(); // Tarefa principal do TinyUSB - DEVE SER CHAMADA REPETIDAMENTE

        // Verifica se o dispositivo CDC está montado e se há dados disponíveis
        if (tud_cdc_connected() && tud_cdc_available()) {
            char rx_buffer[64]; // Buffer para dados recebidos
            // Lê os dados, deixando espaço para o terminador nulo
            uint32_t count = tud_cdc_read(rx_buffer, sizeof(rx_buffer) - 1);

            if (count > 0) {
                processar_comando_cdc(rx_buffer, count);
            }
        }
    }
    return 0; // Nunca alcançado
}

// ==========================================================================
// --- Implementações das Funções Auxiliares ---
// ==========================================================================

/**
 * Configura os pinos dos LEDs como saída e inicializa stdio.
 */
void configurar_hardware_inicial() {
    stdio_init_all(); // Inicializa stdio (USB e/ou UART)

    gpio_init(LED_PIN_VERMELHO);
    gpio_set_dir(LED_PIN_VERMELHO, GPIO_OUT);

    gpio_init(LED_PIN_VERDE);
    gpio_set_dir(LED_PIN_VERDE, GPIO_OUT);

    gpio_init(LED_PIN_AZUL);
    gpio_set_dir(LED_PIN_AZUL, GPIO_OUT);

    apagar_todos_os_leds();
    printf("Hardware (LEDs e stdio) configurado.\n");
}

/**
 * Apaga todos os LEDs do RGB.
 */
void apagar_todos_os_leds() {
    gpio_put(LED_PIN_VERMELHO, 0);
    gpio_put(LED_PIN_VERDE, 0);
    gpio_put(LED_PIN_AZUL, 0);
}

/**
 * Acende um LED específico por 1 segundo com base no comando recebido.
 * @param comando String contendo o comando de cor ("vermelho", "verde", "azul").
 */
void controlar_led_por_comando(const char *comando) {
    apagar_todos_os_leds(); // Garante que outros LEDs estejam apagados

    if (strcmp(comando, CMD_VERMELHO) == 0) {
        printf("-> Acionando LED Vermelho\n");
        gpio_put(LED_PIN_VERMELHO, 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_VERMELHO, 0);
    } else if (strcmp(comando, CMD_VERDE) == 0) {
        printf("-> Acionando LED Verde\n");
        gpio_put(LED_PIN_VERDE, 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_VERDE, 0);
    } else if (strcmp(comando, CMD_AZUL) == 0) {
        printf("-> Acionando LED Azul\n");
        gpio_put(LED_PIN_AZUL, 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_AZUL, 0);
    } else {
        printf("-> Comando de cor desconhecido: '%s'\n", comando);
        // Opcional: piscar todos os LEDs brevemente para indicar erro de comando
    }
}

/**
 * Processa os dados recebidos do host via CDC.
 * Imprime o recebido, identifica o comando, controla o LED e faz o eco.
 * @param buffer Ponteiro para o buffer com os dados recebidos.
 * @param tamanho_buffer Número de bytes recebidos.
 */
void processar_comando_cdc(char *buffer, uint32_t tamanho_buffer) {
    // Adiciona terminador nulo para tratar como string C
    buffer[tamanho_buffer] = '\0';

    // Remove caracteres de nova linha ou carriage return do final
    // strcspn encontra o índice do primeiro caractere de \r ou \n
    buffer[strcspn(buffer, "\r\n")] = 0;

    printf("Host enviou: '%s'\n", buffer);

    // Identifica o comando e controla o LED (Ação 3 e 6)
    controlar_led_por_comando(buffer);

    // Realiza o ECO do comando original (Ação 2 e 5)
    tud_cdc_write_str("Eco: ");
    tud_cdc_write(buffer, strlen(buffer)); // Envia a string limpa
    tud_cdc_write_str("\n");
    tud_cdc_write_flush(); // Envia os dados imediatamente
}

// ==========================================================================
// --- Callbacks do TinyUSB (Opcionais para este exemplo, mas boas práticas) ---
// ==========================================================================

/**
 * Chamada quando o dispositivo CDC é montado (conexão USB estabelecida com o host).
 */
void tud_cdc_mount_cb(uint8_t itf) {
    (void)itf; // Evita warning de parâmetro não utilizado
    printf("Interface CDC USB conectada!\n");
    // Envia mensagem de boas-vindas
    // tud_cdc_write_str("Pico CDC EcoLED pronto.\r\n");
    // tud_cdc_write_flush();
}

/**
 * Chamada quando o dispositivo CDC é desmontado.
 */
void tud_cdc_umount_cb(uint8_t itf) {
    (void)itf;
    printf("Interface CDC USB desconectada.\n");
}

/**
 * Chamada quando a configuração da linha serial (taxa de bits, etc.) é alterada pelo host.
 * Não é essencial para a funcionalidade de eco, mas útil para depuração.
 */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p_line_coding) {
  (void) itf;
  printf("CDC Line Coding: bitrate %lu, stop bits %u, parity %u, data bits %u\n",
         p_line_coding->bit_rate, p_line_coding->stop_bits,
         p_line_coding->parity, p_line_coding->data_bits);
}
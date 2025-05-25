#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Definições de hardware
#define LED_RED 13       // GPIO13 - LED vermelho
#define LED_BLUE 12      // GPIO12 - LED azul
#define LED_GREEN 11     // GPIO11 - LED verde
#define BUZZER 10        // GPIO10 - Buzzer ativo
#define JOYSTICK_Y 26    // GPIO26 (ADC0) - Eixo Y do joystick
#define JOYSTICK_SW 22   // GPIO22 - Botão do joystick

// Estados do sistema
volatile uint8_t flag_estado = 1;  // 1=Baixo, 2=Moderado, 3=Alto

// Limiares do ADC (12-bit: 0-4095)
#define LIMIAR_BAIXO 1500     // <1500 = Estado 1
#define LIMIAR_MODERADO 2500  // 1500-2500 = Estado 2
#define LIMIAR_ALTO 3500      // >2500 = Estado 3

/**
 * Callback periódico para enviar estado via FIFO
 * Retorna intervalo para próximo alarme (2s)
 */
int64_t alarm_callback(alarm_id_t id, void *user_data) {
    multicore_fifo_push_blocking(flag_estado);
    return 2000000; 
}

/**
 * Núcleo 0: Leitura de entradas
 * - Configura ADC para joystick
 * - Ativa pull-up no botão
 * - Atualiza estado conforme posição do joystick
 * - Força estado crítico se botão pressionado
 */
void core0_entry() {
    adc_init();
    adc_gpio_init(JOYSTICK_Y);
    adc_select_input(0);

    gpio_init(JOYSTICK_SW);
    gpio_set_dir(JOYSTICK_SW, GPIO_IN);
    gpio_pull_up(JOYSTICK_SW);

    add_alarm_in_ms(2000, alarm_callback, NULL, true);

    while (true) {
        uint16_t joystick_val = adc_read();

        if (joystick_val < LIMIAR_BAIXO) flag_estado = 1;
        else if (joystick_val < LIMIAR_MODERADO) flag_estado = 2;
        else flag_estado = 3;

        if (!gpio_get(JOYSTICK_SW)) {
            flag_estado = 3;
            sleep_ms(200);  // Debounce
        }
    }
}

/**
 * Núcleo 1: Controle de saídas
 * - Configura LEDs e buzzer
 * - Recebe estados via FIFO
 * - Aciona saídas conforme estado:
 *   1: LED verde
 *   2: LED azul
 *   3: LED vermelho + buzzer intermitente
 */
void core1_entry() {
    // Inicializa LEDs
    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);
    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    // Inicializa buzzer
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);

    while (true) {
        uint8_t estado = multicore_fifo_pop_blocking();

        // Reset de todos os LEDs
        gpio_put(LED_RED, 0);
        gpio_put(LED_GREEN, 0);
        gpio_put(LED_BLUE, 0);

        switch (estado) {
            case 1:
                gpio_put(LED_GREEN, 1);
                gpio_put(BUZZER, 0);
                break;
            case 2:
                gpio_put(LED_BLUE, 1);
                gpio_put(BUZZER, 0);
                break;
            case 3:
                gpio_put(LED_RED, 1);
                // Buzzer intermitente (5 pulsos de 100ms)
                for (int i = 0; i < 5; i++) {
                    gpio_put(BUZZER, 1);
                    sleep_ms(100);
                    gpio_put(BUZZER, 0);
                    sleep_ms(100);
                }
                break;
        }
    }
}

/**
 * Função principal
 * - Inicializa comunicação serial
 * - Lança core1_entry() no segundo núcleo
 * - Executa core0_entry() no núcleo principal
 */
int main() {
    stdio_init_all();
    multicore_launch_core1(core1_entry);
    core0_entry();
    return 0;
}
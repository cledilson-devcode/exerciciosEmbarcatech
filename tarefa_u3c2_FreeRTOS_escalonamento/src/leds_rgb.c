#include "leds_rgb.h"         // Funções de controle dos LEDs
#include "botoes.h"           // Inicialização dos botões
#include "tarefas_rgb_smp.h"  // Declaração das tarefas
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

// ============================================================================
// Inicializa os três canais de um LED RGB como saída digital
// ============================================================================
void init_rgb_led(int r, int g, int b) {
    gpio_init(r); gpio_set_dir(r, GPIO_OUT);
    gpio_init(g); gpio_set_dir(g, GPIO_OUT);
    gpio_init(b); gpio_set_dir(b, GPIO_OUT);
}

// ============================================================================
// Desliga os três canais de um LED RGB
// ============================================================================
void clear_rgb(int r, int g, int b) {
    gpio_put(r, 0);
    gpio_put(g, 0);
    gpio_put(b, 0);
}

// ============================================================================
// Mostra sequencialmente as cores vermelha, verde e azul com 500ms cada
// ============================================================================
void test_rgb_sequence(int r, int g, int b) {
    clear_rgb(r, g, b);
    gpio_put(r, 1); vTaskDelay(pdMS_TO_TICKS(500)); gpio_put(r, 0);
    gpio_put(g, 1); vTaskDelay(pdMS_TO_TICKS(500)); gpio_put(g, 0);
    gpio_put(b, 1); vTaskDelay(pdMS_TO_TICKS(500)); gpio_put(b, 0);
}

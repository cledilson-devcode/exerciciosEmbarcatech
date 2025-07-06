#include "leds_rgb.h"         // Funções de controle dos LEDs
#include "botoes.h"           // Inicialização dos botões
#include "tarefas_rgb_smp.h"  // Declaração das tarefas
#include "pico/stdlib.h"

// ============================================================================
// Configura o pino do botão como entrada com resistor de pull-up interno
// ============================================================================
void init_button(int gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_up(gpio);
}

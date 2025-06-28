#include "pico/stdlib.h"
#include <stdbool.h>
#include "semaforo_rgb.h"

// Inicializa um LED RGB
void rgb_init(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b) {
    gpio_init(gpio_r); gpio_set_dir(gpio_r, GPIO_OUT);
    gpio_init(gpio_g); gpio_set_dir(gpio_g, GPIO_OUT);
    gpio_init(gpio_b); gpio_set_dir(gpio_b, GPIO_OUT);

    gpio_put(gpio_r, 0);
    gpio_put(gpio_g, 0);
    gpio_put(gpio_b, 0);
}

// Define a cor do LED RGB
void rgb_set_color(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b, bool r, bool g, bool b) {
    gpio_put(gpio_r, r);
    gpio_put(gpio_g, g);
    gpio_put(gpio_b, b);
}

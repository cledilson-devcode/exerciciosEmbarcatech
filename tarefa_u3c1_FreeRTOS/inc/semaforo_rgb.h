#ifndef SEMAFORO_RGB_H
#define SEMAFORO_RGB_H

#include <stdbool.h>
#include <stdint.h>

void rgb_init(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b);
void rgb_set_color(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b, bool r, bool g, bool b);

#endif

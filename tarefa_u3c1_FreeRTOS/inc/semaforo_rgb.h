//
// semaforo_rgb.h
//
#ifndef SEMAFORO_RGB_H
#define SEMAFORO_RGB_H

#include <stdint.h>

void rgb_init(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b);

// --- DECLARAÇÃO CORRETA E ÚNICA PARA A VERSÃO PWM ---
// Aceita valores de 0 a 255 para cada cor.
void rgb_set_color(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b, 
                   uint8_t r_val, uint8_t g_val, uint8_t b_val);

#endif
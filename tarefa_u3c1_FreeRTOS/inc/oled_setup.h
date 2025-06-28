#ifndef OLED_SETUP_H
#define OLED_SETUP_H

#include "hardware/i2c.h"
#include "ssd1306_utils.h"
#include <stdint.h>

// Inicializa o I2C, o display OLED e prepara o buffer e a área
void setup_display(i2c_inst_t *porta, uint sda, uint scl, uint freq_khz,uint8_t *buffer, struct render_area *area);

#endif // OLED_SETUP_H

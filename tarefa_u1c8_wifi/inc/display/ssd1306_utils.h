#ifndef SSD1306_UTILS_H
#define SSD1306_UTILS_H

#include "ssd1306_i2c.h"

void calculate_render_area_buffer_length(struct render_area *area);
void render_on_display(uint8_t *buffer, struct render_area *area);
void oled_clear(uint8_t *buffer, struct render_area *area);
void limpar_oled_com_delay(uint8_t *buffer, struct render_area *area, uint delay_ms);
void limpar_oled(uint8_t *buffer, struct render_area *area);

#endif // SSD1306_UTILS_H
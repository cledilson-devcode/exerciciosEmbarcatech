#ifndef SSD1306_TEXT_H
#define SSD1306_TEXT_H

#include <stdint.h>
#include "ssd1306_init.h"  // <- Necessário para struct render_area

void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character);
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, const char *string);
void ssd1306_draw_utf8_multiline(uint8_t *ssd, int16_t x, int16_t y, const char *utf8_string);
void exibir_e_esperar(uint8_t *buffer, struct render_area *area, const char *mensagem, int linha_y);

#endif // SSD1306_TEXT_H

#ifndef SSD1306_GRAPHICS_H
#define SSD1306_GRAPHICS_H

#include "ssd1306_i2c.h"

void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set);
void ssd1306_draw_line(uint8_t *ssd, int x0, int y0, int x1, int y1, bool set);

#endif // SSD1306_GRAPHICS_H

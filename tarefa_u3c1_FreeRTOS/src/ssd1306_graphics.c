#include "ssd1306_graphics.h"

void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set) {
    if (x < 0 || x >= ssd1306_width || y < 0 || y >= ssd1306_height) return;
    int index = (y / 8) * ssd1306_width + x;
    if (set)
        ssd[index] |= (1 << (y % 8));
    else
        ssd[index] &= ~(1 << (y % 8));
}

void ssd1306_draw_line(uint8_t *ssd, int x0, int y0, int x1, int y1, bool set) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    while (1) {
        ssd1306_set_pixel(ssd, x0, y0, set);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

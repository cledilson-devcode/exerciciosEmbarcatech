#include "ssd1306_i2c.h"
#include "digitos_grandes_utils.h"

#define DIGITO_LARGURA         25   // colunas
#define DIGITO_ALTURA_TOTAL    8    // páginas no bitmap
#define DIGITO_ALTURA_USADA    6    // páginas visíveis (da página 2 a 7)
#define DIGITOS_OFFSET_VERTICAL 2   // começa na 3ª página (pixel 16)
#define DOUBLE_DOT_LARGURA         2   // colunas
#define DOUBLE_DOT_ALTURA_TOTAL    8   // altura total do bitmap em páginas
#define DOUBLE_DOT_ALTURA_USADA    2   // páginas visíveis (ex: página 2 e 3)
#define DOUBLE_DOT_OFFSET_VERTICAL 2

const uint8_t double_dot[4] = {
    0x18, 0x18,  // ponto superior (página 3)
    0x18, 0x18   // ponto inferior (página 5)
};

const uint8_t ponto_decimal[2] = {
    0x18, 0x18  // dois pixels centrais ligados nas duas colunas
};

void exibir_digito_grande(uint8_t *buffer, uint8_t x, const uint8_t *bitmap) {
    for (uint8_t col = 0; col < DIGITO_LARGURA; col++) {
        for (uint8_t page = 0; page < DIGITO_ALTURA_USADA; page++) {
            int index = col * DIGITO_ALTURA_TOTAL + (page + DIGITOS_OFFSET_VERTICAL);
            int pos = (page + DIGITOS_OFFSET_VERTICAL) * ssd1306_width + (x + col);
            if (pos < ssd1306_buffer_length) {
                buffer[pos] = bitmap[index];
            }
        }
    }
}

void exibir_double_dot(uint8_t *buffer, uint8_t x) {
    // Ponto superior: página 3 (pixel 24)
    buffer[4 * ssd1306_width + x]     = double_dot[0];
    buffer[4 * ssd1306_width + x + 1] = double_dot[1];

    // Ponto inferior: página 5 (pixel 40)
    buffer[5 * ssd1306_width + x]     = double_dot[2];
    buffer[5 * ssd1306_width + x + 1] = double_dot[3];
}

void exibir_ponto_decimal(uint8_t *buffer, uint8_t x) {
    // Página 7 = última (pixels 56 a 63)
    buffer[7 * ssd1306_width + x]     = ponto_decimal[0];
    buffer[7 * ssd1306_width + x + 1] = ponto_decimal[1];
}
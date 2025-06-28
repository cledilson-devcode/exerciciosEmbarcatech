/**
 * ssd1306_text.c
 * 
 * Módulo de texto para o display OLED SSD1306 usando o barramento I2C.
 * Este arquivo fornece funções para desenhar caracteres, strings e textos com acentuação,
 * inclusive com quebra automática de linha e suporte a UTF-8 básico (convertido para Latin-1).
 * 
 * Ideal para uso em projetos com o Raspberry Pi Pico (RP2040) com C99 e o Pico SDK.
 * 
 * Funções principais:
 * - ssd1306_draw_char(): desenha um único caractere no buffer.
 * - ssd1306_draw_string(): desenha uma string simples (ASCII).
 * - ssd1306_draw_utf8_multiline(): desenha string UTF-8 com acentuação e quebra de linha.
 * - exibir_e_esperar(): exibe uma mensagem por 2 segundos e limpa a tela.
 * 
 * Autor: Orlando (base) + Assistente Virtual (refatoração)
 */

#include "ssd1306_text.h"
#include "ssd1306_font.h"
#include "ssd1306_utils.h"  // Para oled_clear e render_on_display
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"    // Para sleep_ms

// Variáveis globais (devem ser definidas em outro módulo, ex: main.c)
extern uint8_t buffer_oled[];
extern struct render_area area;

// ------------------------------------------------------------
// Função auxiliar para obter índice da fonte no array `font`
// ------------------------------------------------------------
static inline int ssd1306_get_font(uint8_t character) {
    if (character >= 'A' && character <= 'Z') return character - 'A' + 1;
    if (character >= '0' && character <= '9') return character - '0' + 27;
    if (character >= 'a' && character <= 'z') return character - 'a' + 37;
    if (character == '.') return 63;
    if (character == ':') return 64;
    if (character == 0x23) return 65;  // #
    if (character == 0x21) return 66;  // !
    if (character == 0x3F) return 67;  // ?
    if (character == 0xC3) return 68;  // Ã
    if (character == 0xC2) return 69;  // Â
    if (character == 0xC1) return 70;  // Á
    if (character == 0xC0) return 71;  // À
    if (character == 0xC9) return 72;  // É
    if (character == 0xCA) return 73;  // Ê
    if (character == 0xCD) return 74;  // Í
    if (character == 0xD3) return 75;  // Ó
    if (character == 0xD4) return 76;  // Ô
    if (character == 0xD5) return 77;  // Õ
    if (character == 0xDA) return 78;  // Ú
    if (character == 0xC7) return 79;  // Ç
    if (character == 0xE7) return 80;  // ç
    if (character == 0xE3) return 81;  // ã
    if (character == 0xE1) return 82;  // á
    if (character == 0xE0) return 83;  // à
    if (character == 0xE2) return 84;  // â
    if (character == 0xE9) return 85;  // é
    if (character == 0xEA) return 86;  // ê
    if (character == 0xED) return 87;  // í
    if (character == 0xF3) return 88;  // ó
    if (character == 0xF4) return 89;  // ô
    if (character == 0xFA) return 90;  // ú
    if (character == 0x2C) return 91;  // ,

    return 0; // caractere vazio/inválido
}

// ------------------------------------------------------------
// Desenha um caractere único no buffer
// ------------------------------------------------------------
void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) return;
    y /= 8;
    int idx = ssd1306_get_font(character);
    int fb_idx = y * ssd1306_width + x;
    for (int i = 0; i < 8; i++) {
        ssd[fb_idx++] = font[idx * 8 + i];
    }
}

// ------------------------------------------------------------
// Desenha uma string simples (ASCII puro) sem quebra de linha
// ------------------------------------------------------------
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) return;
    while (*string) {
        ssd1306_draw_char(ssd, x, y, *string++);
        x += 8;
    }
}

// ----------------------------------------------------------------------
// Desenha string UTF-8 com suporte a caracteres acentuados (Latin-1)
// Quebra automaticamente a linha ao atingir o limite horizontal.
// Esta versão evita o uso de `continue` para facilitar leitura e ensino.
// ----------------------------------------------------------------------
void ssd1306_draw_utf8_multiline(uint8_t *ssd, int16_t x, int16_t y, const char *utf8_string) {
    const int char_width = 8;
    const int char_height = 8;
    const int max_x = ssd1306_width - char_width;
    const int max_y = ssd1306_height - char_height;

    while (*utf8_string && y <= max_y) {
        uint8_t current = (uint8_t)*utf8_string;
        uint8_t latin1_char;
        bool desenhar = false;

        if ((current & 0x80) == 0) {
            // ASCII puro
            latin1_char = current;
            utf8_string++;
            desenhar = true;
        } else if ((current & 0xE0) == 0xC0) {
            // UTF-8 de 2 bytes → Latin-1
            uint8_t first = (uint8_t)*utf8_string++;
            uint8_t second = (uint8_t)*utf8_string++;
            latin1_char = ((first & 0x1F) << 6) | (second & 0x3F);
            desenhar = true;
        } else {
            // UTF-8 > 2 bytes (não suportado)
            utf8_string++;
        }

        if (desenhar) {
            ssd1306_draw_char(ssd, x, y, latin1_char);
            x += char_width;

            if (x > max_x) {
                x = 0;
                y += char_height;
            }
        }
    }
}

// ------------------------------------------------------------
// Exibe uma mensagem no OLED por 2 segundos e limpa a tela
// ------------------------------------------------------------
void exibir_e_esperar(uint8_t *buffer, struct render_area *area, const char *mensagem, int linha_y) {
    oled_clear(buffer, area);
    ssd1306_draw_utf8_multiline(buffer, 0, linha_y, mensagem);
    render_on_display(buffer, area);
}


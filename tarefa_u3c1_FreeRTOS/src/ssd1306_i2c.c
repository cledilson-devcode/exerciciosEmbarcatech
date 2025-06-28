#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/i2c.h"
#include "ssd1306_font.h"
#include "ssd1306_i2c.h"

/*
 * Calcula o tamanho do buffer necessário para renderizar uma área específica do display.
 * A estrutura render_area possui colunas e páginas inicial e final, e o tamanho do buffer
 * é o produto entre a quantidade de colunas e a quantidade de páginas selecionadas.
 */
void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) * (area->end_page - area->start_page + 1);
}

/*
 * Envia um comando único para o display via barramento I2C.
 * O protocolo do SSD1306 exige que cada comando seja precedido de um byte de controle (0x80).
 */
void ssd1306_send_command(uint8_t command) {
    uint8_t buffer[2] = {0x80, command};  // 0x80 indica comando
    i2c_write_blocking(i2c1, ssd1306_i2c_address, buffer, 2, false);
}

/*
 * Envia uma lista de comandos sequenciais para o display.
 * Útil para inicialização ou configuração, em que vários comandos precisam ser enviados.
 */
void ssd1306_send_command_list(uint8_t *ssd, int number) {
    for (int i = 0; i < number; i++) {
        ssd1306_send_command(ssd[i]);
    }
}

/*
 * Envia um bloco de dados ao display.
 * Cria um buffer temporário maior que o original, adicionando no início o byte de controle (0x40),
 * que indica ao SSD1306 que os próximos bytes são dados e não comandos.
 */
void ssd1306_send_buffer(uint8_t ssd[], int buffer_length) {
    uint8_t *temp_buffer = malloc(buffer_length + 1);

    temp_buffer[0] = 0x40;  // 0x40 indica dados
    memcpy(temp_buffer + 1, ssd, buffer_length);

    i2c_write_blocking(i2c1, ssd1306_i2c_address, temp_buffer, buffer_length + 1, false);

    free(temp_buffer);  // Libera o buffer temporário
}

/*
 * Envia a sequência de comandos de inicialização do display.
 * Essa configuração define parâmetros como multiplexação, contraste, direção das linhas,
 * configurações de carregamento e habilita a exibição.
 */
void ssd1306_init() {
    uint8_t commands[] = {
        ssd1306_set_display, ssd1306_set_memory_mode, 0x00,
        ssd1306_set_display_start_line, ssd1306_set_segment_remap | 0x01, 
        ssd1306_set_mux_ratio, ssd1306_height - 1,
        ssd1306_set_common_output_direction | 0x08, ssd1306_set_display_offset,
        0x00, ssd1306_set_common_pin_configuration,

#if ((ssd1306_width == 128) && (ssd1306_height == 32))
    0x02,
#elif ((ssd1306_width == 128) && (ssd1306_height == 64))
    0x12,
#else
    0x02,
#endif
        ssd1306_set_display_clock_divide_ratio, 0x80, ssd1306_set_precharge,
        0xF1, ssd1306_set_vcomh_deselect_level, 0x30, ssd1306_set_contrast,
        0xFF, ssd1306_set_entire_on, ssd1306_set_normal_display,
        ssd1306_set_charge_pump, 0x14, ssd1306_set_scroll | 0x00,
        ssd1306_set_display | 0x01,
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

/*
 * Ativa ou desativa o "scrolling" horizontal no display.
 * O comando depende do valor do parâmetro "set".
 */
void ssd1306_scroll(bool set) {
    uint8_t commands[] = {
        ssd1306_set_horizontal_scroll | 0x00, 0x00, 0x00, 0x00, 0x03,
        0x00, 0xFF, ssd1306_set_scroll | (set ? 0x01 : 0)
    };

    ssd1306_send_command_list(commands, count_of(commands));
}

/*
 * Renderiza (atualiza) uma área específica do display com os dados do buffer.
 * Envia comandos para definir o endereço da coluna e da página antes dos dados.
 */
void render_on_display(uint8_t *ssd, struct render_area *area) {
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page
    };

    ssd1306_send_command_list(commands, count_of(commands));
    ssd1306_send_buffer(ssd, area->buffer_length);
}

/*
 * Liga ou desliga um pixel específico do buffer de vídeo, com base nas coordenadas (x, y).
 * Cada byte do buffer representa 8 pixels em coluna; a função faz a manipulação de bits apropriada.
 */
void ssd1306_set_pixel(uint8_t *ssd, int x, int y, bool set) {
    assert(x >= 0 && x < ssd1306_width && y >= 0 && y < ssd1306_height);

    const int bytes_per_row = ssd1306_width;

    int byte_idx = (y / 8) * bytes_per_row + x;  // Índice do byte correspondente ao pixel
    uint8_t byte = ssd[byte_idx];

    if (set) {
        byte |= 1 << (y % 8);     // Liga o bit (acende o pixel)
    }
    else {
        byte &= ~(1 << (y % 8));  // Desliga o bit (apaga o pixel)
    }

    ssd[byte_idx] = byte;
}

/*
 * Desenha uma linha reta entre dois pontos utilizando o algoritmo de Bresenham.
 * Este algoritmo permite desenhar linhas rápidas e eficientes usando apenas operações inteiras.
 */
void ssd1306_draw_line(uint8_t *ssd, int x_0, int y_0, int x_1, int y_1, bool set) {
    int dx = abs(x_1 - x_0); // Deslocamento em x
    int dy = -abs(y_1 - y_0);
    int sx = x_0 < x_1 ? 1 : -1; // Direção x
    int sy = y_0 < y_1 ? 1 : -1; // Direção y
    int error = dx + dy; // Erro acumulado
    int error_2;

    while (true) {
        ssd1306_set_pixel(ssd, x_0, y_0, set); // Desenha o pixel atual
        if (x_0 == x_1 && y_0 == y_1) {
            break; // Chegou ao ponto final
        }

        error_2 = 2 * error; // Calcula novo erro

        if (error_2 >= dy) {
            error += dy;
            x_0 += sx; // Avança x
        }
        if (error_2 <= dx) {
            error += dx;
            y_0 += sy; // Avança y
        }
    }
}

/*
 * Obtém o índice de um caractere na tabela de fontes definida em ssd1306_font.h.
 * O índice é diferente para letras e números. Outros caracteres retornam 0.
 */
inline int ssd1306_get_font(uint8_t character)
{
  if (character >= 'A' && character <= 'Z') {
    return character - 'A' + 1;
  }
  else if (character >= '0' && character <= '9') {
    return character - '0' + 27;
  }
  else
    return 0;
}

/*
 * Desenha um caractere individual no display, usando a fonte definida em ssd1306_font.h.
 * O caractere é desenhado a partir da posição (x, y) no buffer.
 */
void ssd1306_draw_char(uint8_t *ssd, int16_t x, int16_t y, uint8_t character) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    y = y / 8; // Cada byte representa 8 pixels na vertical

    character = toupper(character);
    int idx = ssd1306_get_font(character);
    int fb_idx = y * 128 + x; // Calcula o índice no framebuffer

    for (int i = 0; i < 8; i++) {
        ssd[fb_idx++] = font[idx * 8 + i];
    }
}

/*
 * Desenha uma string de caracteres na tela, caracter por caracter, a partir da posição (x, y).
 * O cursor avança 8 pixels a cada caractere.
 */
void ssd1306_draw_string(uint8_t *ssd, int16_t x, int16_t y, char *string) {
    if (x > ssd1306_width - 8 || y > ssd1306_height - 8) {
        return;
    }

    while (*string) {
        ssd1306_draw_char(ssd, x, y, *string++);
        x += 8;  // Avança o cursor horizontalmente
    }
}

/*
 * Envia um comando individual ao display usando uma estrutura ssd1306_t.
 * Essa estrutura armazena informações da instância como porta I2C e endereço.
 */
void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
	ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false );
}

/*
 * Configura o display com uma sequência padrão de comandos, no contexto bitmap.
 * Muito semelhante à inicialização, mas trabalha com a estrutura ssd1306_t.
 */
void ssd1306_config(ssd1306_t *ssd) {
    ssd1306_command(ssd, ssd1306_set_display | 0x00);
    ssd1306_command(ssd, ssd1306_set_memory_mode);
    ssd1306_command(ssd, 0x01);
    ssd1306_command(ssd, ssd1306_set_display_start_line | 0x00);
    ssd1306_command(ssd, ssd1306_set_segment_remap | 0x01);
    ssd1306_command(ssd, ssd1306_set_mux_ratio);
    ssd1306_command(ssd, ssd1306_height - 1);
    ssd1306_command(ssd, ssd1306_set_common_output_direction | 0x08);
    ssd1306_command(ssd, ssd1306_set_display_offset);
    ssd1306_command(ssd, 0x00);
    ssd1306_command(ssd, ssd1306_set_common_pin_configuration);
    ssd1306_command(ssd, 0x12);
    ssd1306_command(ssd, ssd1306_set_display_clock_divide_ratio);
    ssd1306_command(ssd, 0x80);
    ssd1306_command(ssd, ssd1306_set_precharge);
    ssd1306_command(ssd, 0xF1);
    ssd1306_command(ssd, ssd1306_set_vcomh_deselect_level);
    ssd1306_command(ssd, 0x30);
    ssd1306_command(ssd, ssd1306_set_contrast);
    ssd1306_command(ssd, 0xFF);
    ssd1306_command(ssd, ssd1306_set_entire_on);
    ssd1306_command(ssd, ssd1306_set_normal_display);
    ssd1306_command(ssd, ssd1306_set_charge_pump);
    ssd1306_command(ssd, 0x14);
    ssd1306_command(ssd, ssd1306_set_display | 0x01);
}

/*
 * Inicializa uma estrutura ssd1306_t para uso com bitmaps.
 * Define tamanho, endereço, porta I2C e aloca o buffer para os dados de vídeo.
 */
void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8U;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->bufsize = ssd->pages * ssd->width + 1;
    ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
    ssd->ram_buffer[0] = 0x40; // Primeiro byte é o controle de dados
    ssd->port_buffer[0] = 0x80; // Primeiro byte é o controle de comando
}

/*
 * Envia o conteúdo do buffer de vídeo (ram_buffer) para o display físico.
 * Antes, define os endereços de coluna e página para garantir que a escrita
 * ocorra na área correta.
 */
void ssd1306_send_data(ssd1306_t *ssd) {
    ssd1306_command(ssd, ssd1306_set_column_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->width - 1);
    ssd1306_command(ssd, ssd1306_set_page_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->pages - 1);
    i2c_write_blocking(
    ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false );
}

/*
 * Desenha um bitmap (imagem) completo no display.
 * Copia os dados do bitmap para o buffer de vídeo e envia ao display.
 */
void ssd1306_draw_bitmap(ssd1306_t *ssd, const uint8_t *bitmap) {
    for (int i = 0; i < ssd->bufsize - 1; i++) {
        ssd->ram_buffer[i + 1] = bitmap[i];
        ssd1306_send_data(ssd);
    }
}
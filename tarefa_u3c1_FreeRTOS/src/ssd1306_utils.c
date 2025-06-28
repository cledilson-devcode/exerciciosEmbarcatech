#include "ssd1306_utils.h"
#include "ssd1306_init.h"  // <- ESSENCIAL para declarar corretamente as funções usadas
#include <string.h>


void calculate_render_area_buffer_length(struct render_area *area) {
    area->buffer_length = (area->end_column - area->start_column + 1) *
                          (area->end_page - area->start_page + 1);
}

void render_on_display(uint8_t *buffer, struct render_area *area) {
    uint8_t commands[] = {
        ssd1306_set_column_address, area->start_column, area->end_column,
        ssd1306_set_page_address, area->start_page, area->end_page
    };
    ssd1306_send_command_list(commands, sizeof(commands));
    ssd1306_send_buffer(buffer, area->buffer_length);
}

void oled_clear(uint8_t *buffer, struct render_area *area) {
    memset(buffer, 0x00, area->buffer_length);
}

// --------------------------------------------------------------
// Apaga o display após um tempo de espera (em milissegundos)
// --------------------------------------------------------------
void limpar_oled_com_delay(uint8_t *buffer, struct render_area *area, uint delay_ms) {
    sleep_ms(delay_ms);
    oled_clear(buffer, area);
    render_on_display(buffer, area);
}

// --------------------------------------------------------------
// Apaga o display imediatamente, sem espera
// --------------------------------------------------------------
void limpar_oled(uint8_t *buffer, struct render_area *area) {
    oled_clear(buffer, area);
    render_on_display(buffer, area);
}
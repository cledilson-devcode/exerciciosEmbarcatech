#include "inc/display/oled_setup.h"
#include "pico/stdlib.h"
#include "inc/display/ssd1306_init.h"
#include <string.h>

void setup_display(i2c_inst_t *porta, uint sda, uint scl, uint freq_khz,
                   uint8_t *buffer, struct render_area *area) {
    // Inicializa o barramento I2C
    i2c_init(porta, freq_khz * 1000);
    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_pull_up(scl);

    // Inicializa o display
    ssd1306_init();

    // Configura a área de renderização
    area->start_column = 0;
    area->end_column   = ssd1306_width - 1;
    area->start_page   = 0;
    area->end_page     = ssd1306_n_pages - 1;

    calculate_render_area_buffer_length(area);

    // Zera o buffer
    memset(buffer, 0x00, ssd1306_buffer_length);
}

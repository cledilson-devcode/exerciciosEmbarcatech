#include "inc/display/ssd1306_init.h"
#include <stdlib.h>
#include <string.h>

void ssd1306_send_command(uint8_t command) {
    uint8_t buffer[2] = {0x80, command};
    i2c_write_blocking(i2c1, ssd1306_i2c_address, buffer, 2, false);
}

void ssd1306_send_command_list(uint8_t *cmd_list, int number) {
    for (int i = 0; i < number; i++) {
        ssd1306_send_command(cmd_list[i]);
    }
}

void ssd1306_send_buffer(uint8_t *buffer, int length) {
    uint8_t *temp = malloc(length + 1);
    temp[0] = 0x40;
    memcpy(temp + 1, buffer, length);
    i2c_write_blocking(i2c1, ssd1306_i2c_address, temp, length + 1, false);
    free(temp);
}

void ssd1306_init(void) {
    uint8_t cmds[] = {
        ssd1306_set_display,
        ssd1306_set_memory_mode, 0x00,
        ssd1306_set_display_start_line,
        ssd1306_set_segment_remap | 0x01,
        ssd1306_set_mux_ratio, ssd1306_height - 1,
        ssd1306_set_common_output_direction | 0x08,
        ssd1306_set_display_offset, 0x00,
        ssd1306_set_common_pin_configuration, 0x12,
        ssd1306_set_display_clock_divide_ratio, 0x80,
        ssd1306_set_precharge, 0xF1,
        ssd1306_set_vcomh_deselect_level, 0x30,
        ssd1306_set_contrast, 0xFF,
        ssd1306_set_entire_on,
        ssd1306_set_normal_display,
        ssd1306_set_charge_pump, 0x14,
        ssd1306_set_scroll | 0x00,
        ssd1306_set_display | 0x01,
    };
    ssd1306_send_command_list(cmds, sizeof(cmds));
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command) {
    ssd->port_buffer[1] = command;
    i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->port_buffer, 2, false);
}

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

void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c) {
    ssd->width = width;
    ssd->height = height;
    ssd->pages = height / 8;
    ssd->address = address;
    ssd->i2c_port = i2c;
    ssd->external_vcc = external_vcc;
    ssd->bufsize = ssd->pages * ssd->width + 1;
    ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
    ssd->ram_buffer[0] = 0x40;
    ssd->port_buffer[0] = 0x80;
}

void ssd1306_send_data(ssd1306_t *ssd) {
    ssd1306_command(ssd, ssd1306_set_column_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->width - 1);
    ssd1306_command(ssd, ssd1306_set_page_address);
    ssd1306_command(ssd, 0);
    ssd1306_command(ssd, ssd->pages - 1);
    i2c_write_blocking(ssd->i2c_port, ssd->address, ssd->ram_buffer, ssd->bufsize, false);
}

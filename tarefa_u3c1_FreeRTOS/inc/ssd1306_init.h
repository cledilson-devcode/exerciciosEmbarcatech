#ifndef SSD1306_INIT_H
#define SSD1306_INIT_H

#include "ssd1306_i2c.h"

void ssd1306_send_command(uint8_t command);
void ssd1306_send_command_list(uint8_t *cmd_list, int number);
void ssd1306_send_buffer(uint8_t *buffer, int length);
void ssd1306_init(void);
void ssd1306_command(ssd1306_t *ssd, uint8_t command);
void ssd1306_config(ssd1306_t *ssd);
void ssd1306_init_bm(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c);
void ssd1306_send_data(ssd1306_t *ssd);

#endif // SSD1306_INIT_H

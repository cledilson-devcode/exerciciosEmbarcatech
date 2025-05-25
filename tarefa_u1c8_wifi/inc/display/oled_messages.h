// inc/oled_messages.h
#ifndef OLED_MESSAGES_H
#define OLED_MESSAGES_H

#include "ssd1306_i2c.h"
#include <stdio.h>
#include <stdbool.h>
#include "pico/time.h"
#include "hardware/i2c.h"

typedef struct {
    const char *message_active;
    const char *message_inactive;
    int16_t line_y_pixel;
    uint32_t interval_ms;
    bool visible;
    absolute_time_t last_toggle_time;
    bool initialized;
    bool active;
    // uint led_pin; // Alternativa: armazenar o pino do LED na struct
} BlinkingAlertState;

void oled_alert_init(BlinkingAlertState *alert_state,
                     const char *active_msg,
                     const char *inactive_msg,
                     int16_t line_y,
                     uint32_t interval_ms,
                     bool start_active,
                     i2c_inst_t *i2c_port,
                     uint8_t *buffer,
                     struct render_area *area,
                     uint led_pin); // <<< NOVO: Parâmetro para o pino do LED

bool oled_alert_update(uint8_t *buffer, struct render_area *area, i2c_inst_t *i2c_port,
                       BlinkingAlertState *alert_state,
                       uint led_pin); // <<< NOVO

void oled_alert_toggle_active(BlinkingAlertState *alert_state, uint8_t *buffer, struct render_area *area, i2c_inst_t *i2c_port,
                              uint led_pin); // <<< NOVO

#endif // OLED_MESSAGES_H
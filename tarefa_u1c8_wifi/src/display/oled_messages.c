// src/oled_messages.c
#include "inc/display/oled_messages.h"
#include "inc/display/ssd1306_text.h"
#include "inc/display/ssd1306_utils.h"
#include "inc/display/ssd1306_i2c.h" // Para ssd1306_width (ou o .h que define)
#include "pico/stdlib.h"
#include "hardware/gpio.h" // <<< NOVO: Para gpio_put
#include <string.h>

// Função auxiliar para desenhar mensagem centralizada (mantida como está)
static void draw_centered_message(uint8_t *buffer, int16_t line_y, const char *message) {
    if (!message) return;
    int message_width_pixels = strlen(message) * 8;
    int16_t start_x = (ssd1306_width - message_width_pixels) / 2;
    if (start_x < 0) start_x = 0;
    ssd1306_draw_string(buffer, start_x, line_y, message);
}

void oled_alert_init(BlinkingAlertState *alert_state,
                     const char *active_msg,
                     const char *inactive_msg,
                     int16_t line_y,
                     uint32_t interval_ms,
                     bool start_active,
                     i2c_inst_t *i2c_port,
                     uint8_t *buffer,
                     struct render_area *area,
                     uint led_pin) { // <<< NOVO parâmetro
    if (!alert_state) return;
    alert_state->message_active = active_msg;
    alert_state->message_inactive = inactive_msg;
    alert_state->line_y_pixel = line_y;
    alert_state->interval_ms = interval_ms;
    alert_state->visible = true;
    alert_state->last_toggle_time = get_absolute_time();
    alert_state->active = start_active;
    alert_state->initialized = true;
    // alert_state->led_pin = led_pin; // Se tivesse adicionado à struct

    // Renderização inicial e estado do LED
    oled_clear(buffer, area);
    if (alert_state->active) {
        if (alert_state->visible) {
            draw_centered_message(buffer, alert_state->line_y_pixel, alert_state->message_active);
            gpio_put(led_pin, 1); // <<< NOVO: Liga o LED
        } else {
            gpio_put(led_pin, 0); // <<< NOVO: Desliga o LED
        }
    } else {
        draw_centered_message(buffer, alert_state->line_y_pixel, alert_state->message_inactive);
        gpio_put(led_pin, 0); // <<< NOVO: Garante que o LED está desligado
    }
    render_on_display(buffer, area);
}

void oled_alert_toggle_active(BlinkingAlertState *alert_state, uint8_t *buffer, struct render_area *area, i2c_inst_t *i2c_port,
                              uint led_pin) { // <<< NOVO parâmetro
    if (!alert_state || !alert_state->initialized) return;

    alert_state->active = !alert_state->active;

    oled_clear(buffer, area);

    if (alert_state->active) {
        printf("Alerta ATIVADO: %s\n", alert_state->message_active);
        alert_state->visible = true;
        alert_state->last_toggle_time = get_absolute_time();
        draw_centered_message(buffer, alert_state->line_y_pixel, alert_state->message_active);
        gpio_put(led_pin, 1); // <<< NOVO: Liga o LED
    } else {
        printf("Alerta DESATIVADO: %s\n", alert_state->message_inactive);
        draw_centered_message(buffer, alert_state->line_y_pixel, alert_state->message_inactive);
        gpio_put(led_pin, 0); // <<< NOVO: Desliga o LED
    }
    render_on_display(buffer, area);
}

bool oled_alert_update(uint8_t *buffer, struct render_area *area, i2c_inst_t *i2c_port,
                       BlinkingAlertState *alert_state,
                       uint led_pin) { // <<< NOVO parâmetro
    if (!alert_state || !alert_state->initialized) {
        return false;
    }

    if (!alert_state->active) {
        // Garante que o LED está desligado se o alerta não estiver ativo
        // (oled_alert_toggle_active já deve ter feito isso, mas por segurança)
        // gpio_put(led_pin, 0); // Pode ser redundante se toggle_active for robusto
        return false;
    }

    absolute_time_t current_time = get_absolute_time();
    int64_t diff_us = absolute_time_diff_us(alert_state->last_toggle_time, current_time);
    uint64_t interval_us = (uint64_t)alert_state->interval_ms * 50;

    bool needs_display_refresh = false;

    if (diff_us >= interval_us) {
        alert_state->visible = !alert_state->visible;
        alert_state->last_toggle_time = current_time;
        needs_display_refresh = true;
    }

     if (needs_display_refresh) {
        oled_clear(buffer, area);
        if (alert_state->visible) {
            draw_centered_message(buffer, alert_state->line_y_pixel, alert_state->message_active);
            gpio_put(led_pin, 1); // ACENDE O LED
        } else {
            gpio_put(led_pin, 0); // APAGA O LED
        }
        render_on_display(buffer, area);
    }
    return false;
}
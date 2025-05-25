// inc/display/display_app.h
#ifndef DISPLAY_APP_H
#define DISPLAY_APP_H

#include "display/oled_messages.h"
#include "display/ssd1306_i2c.h"
#include "hardware/i2c.h"

bool display_application_init(uint8_t *buffer_ptr, struct render_area *area_ptr,
                              BlinkingAlertState *alert_state_ptr, i2c_inst_t *i2c_port_ptr);

void display_application_process(void);

// Novas funções para controle explícito
void display_application_activate_alert(void);
void display_application_deactivate_alert(void);

// Opcional: se você ainda quiser uma função de toggle geral
// void display_application_toggle_alert_state(void);

#endif // DISPLAY_APP_H
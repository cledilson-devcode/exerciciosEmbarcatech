// src/display/display_app.c
#include "display/display_app.h" // Caminho correto para o include
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <string.h>

// Inclua os cabeçalhos necessários, ajustando os caminhos se necessário
#include "display/oled_setup.h"    // Para setup_display
#include "display/ssd1306_i2c.h"   // Para definições do SSD1306 (ou "display/ssd1306.h")
// oled_messages.h já deve estar incluído por display/display_app.h

// --- Variáveis estáticas para manter o estado da aplicação ---
static uint8_t *s_app_buffer_oled = NULL;
static struct render_area *s_app_area = NULL;
static BlinkingAlertState *s_app_alert_state = NULL;
static i2c_inst_t *s_app_i2c_port = NULL;

// Definições consistentes com o que setup_display espera ou usa internamente.
#define APP_SDA_PIN_INTERNAL 14
#define APP_SCL_PIN_INTERNAL 15
#define APP_FREQ_KHZ_INTERNAL 400

// Defina o pino do LED vermelho usado por este módulo
#define LED_VERMELHO_PIN_APP 13   // Use o pino GPIO real do seu LED

bool display_application_init(uint8_t *buffer_ptr, struct render_area *area_ptr,
                              BlinkingAlertState *alert_state_ptr, i2c_inst_t *i2c_port_ptr) {
    if (!buffer_ptr || !area_ptr || !alert_state_ptr || !i2c_port_ptr) {
        return false;
    }

    s_app_buffer_oled = buffer_ptr;
    s_app_area = area_ptr;
    s_app_alert_state = alert_state_ptr;
    s_app_i2c_port = i2c_port_ptr;

    setup_display(s_app_i2c_port,
                  APP_SDA_PIN_INTERNAL,
                  APP_SCL_PIN_INTERNAL,
                  APP_FREQ_KHZ_INTERNAL,
                  s_app_buffer_oled,
                  s_app_area);

    gpio_init(LED_VERMELHO_PIN_APP);
    gpio_set_dir(LED_VERMELHO_PIN_APP, GPIO_OUT);
    gpio_put(LED_VERMELHO_PIN_APP, 0);

    // Inicializa o estado do alerta
    oled_alert_init(s_app_alert_state,
                      "EVACUAR!!!",
                    "SEGURO",
                    24,                    // Linha Y
                    // ***************************************************************
                    // MODIFICAÇÃO AQUI: Diminuindo AINDA MAIS o intervalo do pisca-pisca
                    // ***************************************************************
                    5,                    // Intervalo de pisca em milissegundos.
                                           // Anteriormente 150ms.
                                           // Um valor menor aqui fará piscar ainda mais rápido.
                                           // Com 75ms:
                                           // - Aceso por 75ms
                                           // - Apagado por 75ms
                                           // - Ciclo completo = 150ms
                                           // - Aproximadamente 1000ms / 150ms = 6.67 piscadas por segundo.
                                           //
                                           // Você pode tentar valores como 50ms (10 piscadas/seg)
                                           // ou até menos, mas muito baixo pode parecer cintilação.
                    // ***************************************************************
                    false,                 // Começa INATIVO ("SEGURO") por padrão
                    s_app_i2c_port,
                    s_app_buffer_oled,
                    s_app_area,
                    LED_VERMELHO_PIN_APP);

    return true;
}

// Função para ser chamada pelo main para ATIVAR o alerta no display
void display_application_activate_alert(void) {
    if (s_app_alert_state && s_app_buffer_oled && s_app_area && s_app_i2c_port) {
        if (!s_app_alert_state->active) {
            oled_alert_toggle_active(s_app_alert_state, s_app_buffer_oled, s_app_area, s_app_i2c_port, LED_VERMELHO_PIN_APP);
        }
    }
}

// Função para ser chamada pelo main para DESATIVAR o alerta no display
void display_application_deactivate_alert(void) {
    if (s_app_alert_state && s_app_buffer_oled && s_app_area && s_app_i2c_port) {
        if (s_app_alert_state->active) {
            oled_alert_toggle_active(s_app_alert_state, s_app_buffer_oled, s_app_area, s_app_i2c_port, LED_VERMELHO_PIN_APP);
        }
    }
}

// Função chamada repetidamente pelo loop principal do main.c para atualizar o display (piscar)
void display_application_process(void) {
    if (s_app_buffer_oled && s_app_area && s_app_alert_state && s_app_i2c_port) {
        oled_alert_update(s_app_buffer_oled, s_app_area, s_app_i2c_port, s_app_alert_state, LED_VERMELHO_PIN_APP);
    }
}
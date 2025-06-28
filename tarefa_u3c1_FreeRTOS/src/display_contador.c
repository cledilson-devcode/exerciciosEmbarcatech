#include "display_contador.h"
#include "oled_display.h"
#include "numeros_grandes.h"
#include "digitos_grandes_utils.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// A fila deve ser definida externamente em main.c
extern QueueHandle_t fila_oled;

void tarefa_display_contador(void *p) {
    // Buffer para o display OLED e configuração da área de renderização
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area area;

    // Inicializa o display I2C (i2c1 nos pinos GPIO14 - SDA, GPIO15 - SCL)
    setup_display(i2c1, 14, 15, 400, buffer, &area);
    oled_clear(buffer, &area);
    render_on_display(buffer, &area);

    int valor = 0;

    while (1) {
        BaseType_t recebido = xQueueReceive(fila_oled, &valor, portMAX_DELAY);

        if (recebido == pdTRUE) {
            if (valor > 0) {
                
                for (int i = valor; i >= 1; i--) {
                    oled_clear(buffer, &area);

                    int dezena  = (i / 10) % 10;
                    int unidade = i % 10;
                        if (valor == 60)
                            ssd1306_draw_utf8_multiline(buffer,0,0,"AVENIDA");
                        else if (valor = 20)
                            ssd1306_draw_utf8_multiline(buffer,0,0,"PEDESTRE");
                    exibir_digito_grande(buffer, 30, numeros_grandes[dezena]);
                    exibir_digito_grande(buffer, 70, numeros_grandes[unidade]);

                    render_on_display(buffer, &area);
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }

                // Limpa o display ao final do contador
                oled_clear(buffer, &area);
                render_on_display(buffer, &area);
            } else {
                // Limpa diretamente se valor == 0
                oled_clear(buffer, &area);
                render_on_display(buffer, &area);
            }
        }
    }
}

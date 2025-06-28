//
// display_contador.c
//
#include "display_contador.h"
#include "oled_display.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Declaração da fila que é criada no main.c
extern QueueHandle_t fila_oled;

void tarefa_display_contador(void *p) {
    // Buffer para o display OLED e configuração da área de renderização
    uint8_t buffer[ssd1306_buffer_length];
    struct render_area area;

    // Inicializa o display I2C (i2c1 nos pinos GPIO14 - SDA, GPIO15 - SCL)
    setup_display(i2c1, 14, 15, 400, buffer, &area);
    oled_clear(buffer, &area);
    render_on_display(buffer, &area);

    int tempo_recebido = 0;

    while (1) {
        // Espera bloqueado até receber um novo valor de tempo da fila
        if (xQueueReceive(fila_oled, &tempo_recebido, portMAX_DELAY) == pdTRUE) {
            
            // Inicia a contagem regressiva no display
            for (int i = tempo_recebido; i >= 1; i--) {
                oled_clear(buffer, &area);

                // Extrai dezena e unidade para exibir
                int dezena  = (i / 10) % 10;
                int unidade = i % 10;
                
                // Exibe os dígitos grandes no centro do display
                exibir_digito_grande(buffer, 30, numeros_grandes[dezena]);
                exibir_digito_grande(buffer, 70, numeros_grandes[unidade]);

                render_on_display(buffer, &area);
                
                // Espera 1 segundo antes de atualizar o display novamente
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            // Limpa o display ao final da contagem
            oled_clear(buffer, &area);
            render_on_display(buffer, &area);
        }
    }
}
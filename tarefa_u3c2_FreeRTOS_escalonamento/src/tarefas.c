#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "perifericos.h"
#include "bitdoglab_pins.h"

extern SemaphoreHandle_t self_test_sem;
extern const float ADC_CONVERSION_FACTOR;

/**
 * @brief Tarefa 1: Realiza um autoteste e, ao final, SINALIZA para as outras tarefas.
 */
void task_self_test(void *params) {
    printf("\n[TAREFA 1] Iniciando Self-Test...\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    test_leds_rgb();         vTaskDelay(pdMS_TO_TICKS(500));
    test_buzzer();           vTaskDelay(pdMS_TO_TICKS(500));
    test_botoes();           vTaskDelay(pdMS_TO_TICKS(500));
    test_joystick_analog();  vTaskDelay(pdMS_TO_TICKS(500));
    test_microfone();        vTaskDelay(pdMS_TO_TICKS(500));

    printf("[TAREFA 1] Self-Test concluido. Sinalizando para Tarefas 2 e 3.\n\n");

    xSemaphoreGive(self_test_sem);
    xSemaphoreGive(self_test_sem);

    vTaskDelete(NULL);
}

/**
 * @brief Tarefa 2: AGUARDA o sinal da Tarefa 1 antes de começar a piscar o LED.
 */
void task_alive(void *params) {
    if (xSemaphoreTake(self_test_sem, portMAX_DELAY) == pdTRUE) {
        printf("[TAREFA 2] Sinal recebido. Iniciando Alive Task...\n");
        while (true) {
            gpio_put(LED_ALIVE_PIN, 1);
            vTaskDelay(pdMS_TO_TICKS(500));
            gpio_put(LED_ALIVE_PIN, 0);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
    }
}

/**
 * @brief Tarefa 3: AGUARDA o sinal da Tarefa 1, depois monitora o joystick
 *        e aciona um alarme se necessário.
 */
void task_monitor_joystick(void *params) {
    if (xSemaphoreTake(self_test_sem, portMAX_DELAY) == pdTRUE) {
        printf("[TAREFA 3] Sinal recebido. Iniciando Monitor Task...\n");

        while (true) {
            // Leitura do Eixo Y (ADC0)
            adc_select_input(0);
            float voltageY = adc_read() * ADC_CONVERSION_FACTOR;

            // Leitura do Eixo X (ADC1)
            adc_select_input(1);
            float voltageX = adc_read() * ADC_CONVERSION_FACTOR;

            // Imprime as leituras no terminal
            printf("JOYSTICK -> X: %.2f V, Y: %.2f V\n", voltageX, voltageY);

            // Lógica do Alarme
            if (voltageX > 3.0f || voltageY > 3.0f) {
                gpio_put(BUZZER_PIN, 1);
            } else {
                gpio_put(BUZZER_PIN, 0);
            }

            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
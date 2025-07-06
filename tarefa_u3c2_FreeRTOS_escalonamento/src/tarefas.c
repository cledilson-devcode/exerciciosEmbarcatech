#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "perifericos.h"

/**
 * @brief Tarefa 1: Realiza um autoteste de todos os periféricos
 *        na inicialização e depois se auto-deleta.
 */
void task_self_test(void *params) {
    printf("\n[TAREFA 1] Iniciando Self-Test...\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay para garantir que a mensagem seja lida

    // Executa a sequência de testes
    test_leds_rgb();
    vTaskDelay(pdMS_TO_TICKS(500));

    test_buzzer();
    vTaskDelay(pdMS_TO_TICKS(500));

    test_botoes();
    vTaskDelay(pdMS_TO_TICKS(500));

    test_joystick_analog();
    vTaskDelay(pdMS_TO_TICKS(500));

    test_microfone();
    vTaskDelay(pdMS_TO_TICKS(500));

    printf("[TAREFA 1] Self-Test concluido. Deletando a si mesma.\n\n");

    // A tarefa deleta a si mesma, liberando seus recursos
    vTaskDelete(NULL);
}
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "perifericos.h"
#include "bitdoglab_pins.h"
#include "semphr.h" // Header para semáforos


// Declaração externa do handle do semáforo criado no main.c
extern SemaphoreHandle_t self_test_sem;

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

    printf("[TAREFA 1] Self-Test concluido. Sinalizando para a Tarefa 2 e deletando a si mesma.\n\n");

    // "Entrega o bastão" para a Tarefa 2
    xSemaphoreGive(self_test_sem);

    // A tarefa deleta a si mesma, liberando seus recursos
    vTaskDelete(NULL);
}


/**
 * @brief Tarefa 2: Pisca o LED vermelho (GPIO 13) para indicar
 *        que o sistema está funcionando (Alive).
 *        Tarefa 2: AGUARDA o sinal da Tarefa 1 antes de começar a piscar o LED.
 */
void task_alive(void *params) {
    // "Espera pelo bastão"
    // A tarefa ficará bloqueada aqui, sem consumir CPU, até que o semáforo seja liberado.
    if (xSemaphoreTake(self_test_sem, portMAX_DELAY) == pdTRUE) {
        
        // O código a partir daqui só executa DEPOIS que a Tarefa 1 sinalizou.
        printf("[TAREFA 2] Sinal recebido da Tarefa 1. Iniciando Alive Task...\n");
        
        while (true) {
            gpio_put(LED_ALIVE_PIN, 1); // Liga o LED
            vTaskDelay(pdMS_TO_TICKS(500)); // Mantém aceso por 500ms

            gpio_put(LED_ALIVE_PIN, 0); // Desliga o LED
            vTaskDelay(pdMS_TO_TICKS(500)); // Mantém apagado por 500ms
        }
    }
}
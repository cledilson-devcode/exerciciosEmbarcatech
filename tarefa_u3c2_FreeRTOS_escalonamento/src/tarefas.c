#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "perifericos.h"
#include "bitdoglab_pins.h"

// Declaração para informar que estas variáveis globais existem em outro arquivo (main.c).
extern SemaphoreHandle_t self_test_sem;
extern const float ADC_CONVERSION_FACTOR;

// Tarefa 1: Executa uma rotina de autoteste dos periféricos na inicialização.
void task_self_test(void *params) {
    printf("\n[TAREFA 1] Iniciando Self-Test...\n");
    // Pausa inicial para dar tempo de ler a mensagem no terminal.
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Sequência de chamadas para as funções de teste de cada periférico.
    test_leds_rgb();         vTaskDelay(pdMS_TO_TICKS(500));
    test_buzzer_pwm();       vTaskDelay(pdMS_TO_TICKS(500));
    test_botoes();           vTaskDelay(pdMS_TO_TICKS(500));
    test_joystick_analog();  vTaskDelay(pdMS_TO_TICKS(500));
    test_microfone();        vTaskDelay(pdMS_TO_TICKS(500));

    printf("[TAREFA 1] Self-Test concluido. Sinalizando para Tarefas 2 e 3.\n\n");

    // Libera o semáforo duas vezes para desbloquear as duas tarefas que estão esperando.
    xSemaphoreGive(self_test_sem);
    xSemaphoreGive(self_test_sem);
    xSemaphoreGive(self_test_sem);

    // Deleta a si mesma, liberando os recursos que estava utilizando (pilha).
    vTaskDelete(NULL);
}

// Tarefa 2: Pisca um LED para indicar que o sistema está ativo ("sinal de vida").
void task_alive(void *params) {
    // Tenta "pegar" o semáforo. A tarefa ficará bloqueada aqui até a Tarefa 1 liberá-lo.
    if (xSemaphoreTake(self_test_sem, portMAX_DELAY) == pdTRUE) {
        // Este bloco só executa após a Tarefa 1 terminar.
        printf("[TAREFA 2] Sinal recebido. Iniciando Alive Task...\n");
        
        // Loop infinito para a funcionalidade principal da tarefa.
        while (true) {
            gpio_put(LED_ALIVE_PIN, 1);      // Liga o LED.
            vTaskDelay(pdMS_TO_TICKS(500));  // Espera 500ms.
            gpio_put(LED_ALIVE_PIN, 0);      // Desliga o LED.
            vTaskDelay(pdMS_TO_TICKS(500));  // Espera 500ms.
        }
    }
}

// Tarefa 3: Monitora o joystick e aciona um alarme sonoro se os limites forem excedidos.
void task_monitor_joystick(void *params) {
    // Também aguarda o sinal da Tarefa 1 para iniciar sua execução.
    if (xSemaphoreTake(self_test_sem, portMAX_DELAY) == pdTRUE) {
        printf("[TAREFA 3] Sinal recebido. Iniciando Monitor Task...\n");
        // xSemaphoreGive(self_test_sem);
        while (true) {
            // Seleciona e lê o canal ADC do eixo Y do joystick.
            adc_select_input(0);
            float voltageY = adc_read() * ADC_CONVERSION_FACTOR;
            // Seleciona e lê o canal ADC do eixo X do joystick.
            adc_select_input(1);
            float voltageX = adc_read() * ADC_CONVERSION_FACTOR;
            // Imprime os valores de tensão lidos no terminal.
            printf("JOYSTICK -> X: %.2f V, Y: %.2f V\n", voltageX, voltageY);

            // Lógica de decisão do alarme.
            if (voltageX > 3.0f || voltageY > 3.0f) {
                buzzer_set_alarm(true); // Ativa o som do buzzer.
            } else {
                buzzer_set_alarm(false); // Desativa o som do buzzer.
            }

            // Pausa a tarefa por 50ms antes da próxima leitura.
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
}
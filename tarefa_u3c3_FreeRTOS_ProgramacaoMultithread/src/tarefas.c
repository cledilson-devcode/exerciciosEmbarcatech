//---------------------------------------------------------------------------------------------//
// tarefas.c
//---------------------------------------------------------------------------------------------//
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"

#include "perifericos.h"
#include "bitdoglab_pins.h"
#include "tarefas.h" // Inclui a nova definição da fila e os handles

// --- ALTERAÇÃO: Implementação das novas tarefas ---



// Tarefa 1: Lê os eixos do joystick a cada 100ms e envia para a fila.
void task_leitura_eixos(void *params) {
    queue_msg_t msg;
    msg.type = JOYSTICK_DATA;

    while (true) {
        adc_select_input(0); // Canal do eixo Y
        msg.y = adc_read();
        adc_select_input(1); // Canal do eixo X
        msg.x = adc_read();

        // Envia a mensagem para a fila. Não bloqueia se a fila estiver cheia.
        xQueueSend(xQueueEventos, &msg, 0);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// Tarefa 2: Verifica o botão a cada 50ms e envia um evento para a fila ao ser clicado.
void task_leitura_botao(void *params) {
    queue_msg_t msg;
    msg.type = BUTTON_CLICK;
    bool btn_pressed_flag = false;

    while (true) {
        // Lógica de debounce simples: só envia o evento uma vez por pressionamento.
        if (!gpio_get(BTN_JOY_SW_PIN) && !btn_pressed_flag) {
            btn_pressed_flag = true;
            xQueueSend(xQueueEventos, &msg, 0);
        } else if (gpio_get(BTN_JOY_SW_PIN)) {
            btn_pressed_flag = false;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa 3: Recebe eventos da fila, processa e exibe no terminal.
void task_processamento(void *params) {
    queue_msg_t msg;
    while (true) {
        // Bloqueia até receber uma mensagem da fila.
        if (xQueueReceive(xQueueEventos, &msg, portMAX_DELAY) == pdTRUE) {
            
            // Protege o acesso ao printf com o Mutex.
            if (xSemaphoreTake(xMutexPrintf, portMAX_DELAY) == pdTRUE) {
                if (msg.type == JOYSTICK_DATA) {
                    printf("Joystick -> X: %d, Y: %d\n", msg.x, msg.y);
                } else if (msg.type == BUTTON_CLICK) {
                    printf("Evento -> Botao Pressionado!\n");
                }
                xSemaphoreGive(xMutexPrintf);
            }

            // Lógica para acionar o buzzer
            bool trigger_buzzer = false;
            if (msg.type == BUTTON_CLICK) {
                trigger_buzzer = true;
            } else if (msg.type == JOYSTICK_DATA) {
                // Movimento significativo: valores muito altos ou muito baixos
                if (msg.x > 3500 || msg.x < 500 || msg.y > 3500 || msg.y < 500) {
                    trigger_buzzer = true;
                }
            }

            if (trigger_buzzer) {
                // Libera o semáforo para a tarefa do buzzer.
                xSemaphoreGive(xCountingSemBuzzer);
            }
        }
    }
}

// Tarefa 4: Controla o buzzer. Fica bloqueada até ser sinalizada pelo semáforo.
void task_controle_buzzer(void *params) {
    while (true) {
        // Bloqueia até que o semáforo seja liberado pela tarefa de processamento.
        if (xSemaphoreTake(xCountingSemBuzzer, portMAX_DELAY) == pdTRUE) {
            buzzer_set_alarm(true);
            vTaskDelay(pdMS_TO_TICKS(100)); // Mantém o som por 100ms
            buzzer_set_alarm(false);
        }
    }
}
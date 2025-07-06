#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "perifericos.h"
#include "tarefas.h"

// Handles para as tarefas
TaskHandle_t handle_self_test = NULL;
TaskHandle_t handle_alive_task = NULL;
TaskHandle_t handle_monitor_task = NULL;

// Handle para o semáforo de sincronização
SemaphoreHandle_t self_test_sem = NULL;

int main() {
    stdio_init_all();
    
    while (!stdio_usb_connected()) sleep_ms(200);

    init_perifericos();
    buzzer_pwm_init();

    self_test_sem = xSemaphoreCreateBinary();
    if (self_test_sem == NULL) {
        printf("ERRO: Nao foi possivel criar o semaforo.\n");
        while(1);
    }

    printf("Sistema inicializado. Criando tarefas FreeRTOS com afinidade de nucleo (SMP)...\n");

    // ======================================================================
    // Criação das tarefas com afinidade de núcleo
    // ======================================================================

    // Tarefa 1: Self-Test (será executada no Núcleo 0)
    xTaskCreate(task_self_test, "Self-Test Task", 2048, NULL, 1, &handle_self_test);
    // vTaskCoreAffinitySet(handle_self_test, (1 << 0)); // Prende a tarefa ao Núcleo 0

    // Tarefa 2: Alive Task (será executada no Núcleo 1)
    xTaskCreate(task_alive, "Alive Task", 256, NULL, 2, &handle_alive_task);
    // vTaskCoreAffinitySet(handle_alive_task, (1 << 1)); // Prende a tarefa ao Núcleo 1

    // Tarefa 3: Monitor (será executada no Núcleo 0)
    xTaskCreate(task_monitor_joystick, "Monitor Task", 1024, NULL, 3, &handle_monitor_task);
    // vTaskCoreAffinitySet(handle_monitor_task, (1 << 0)); // Prende a tarefa ao Núcleo 0

    // Inicia o escalonador do FreeRTOS para ambos os núcleos
    vTaskStartScheduler();

    // O código nunca deve chegar aqui
    while (true) {}
}
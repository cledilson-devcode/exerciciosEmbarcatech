#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "perifericos.h"
#include "tarefas.h"

// Handle para a tarefa, permite controle futuro
TaskHandle_t handle_self_test = NULL;

int main() {
    // Inicializa a comunicação serial USB e aguarda a conexão
    stdio_init_all();

    while (!stdio_usb_connected()) sleep_ms(200);

    // Inicializa todo o hardware da placa BitDogLab
    init_perifericos();

    printf("Sistema inicializado. Criando tarefas FreeRTOS...\n");

    // Criação da Tarefa 1: Self-Test
    xTaskCreate(task_self_test,      // Função da tarefa
                "Self-Test Task",    // Nome da tarefa
                2048,                // Tamanho da pilha (aumentado por causa do printf)
                NULL,                // Parâmetros da tarefa
                1,                   // Prioridade
                &handle_self_test);  // Handle da tarefa

    // AQUI SERÃO CRIADAS AS TAREFAS 2 E 3 NO FUTURO

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();

    // O código nunca deve chegar aqui
    while (true) {}
}
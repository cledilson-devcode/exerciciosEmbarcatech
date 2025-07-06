#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h" // Header para semáforos

#include "perifericos.h"
#include "tarefas.h"


// Handle para a tarefa, permite controle futuro
TaskHandle_t handle_self_test = NULL;
TaskHandle_t handle_alive_task = NULL;
// Handle para o semáforo de sincronização
SemaphoreHandle_t self_test_sem = NULL;

int main() {
    // Inicializa a comunicação serial USB e aguarda a conexão
    stdio_init_all();

    while (!stdio_usb_connected()) sleep_ms(200);

    // Inicializa todo o hardware da placa BitDogLab
    init_perifericos();

    // Cria o semáforo binário
    // O semáforo começa "vazio". Ninguém pode pegá-lo até que seja "dado".
    self_test_sem = xSemaphoreCreateBinary();
    if (self_test_sem == NULL) {
        printf("ERRO: Nao foi possivel criar o semaforo.\n");
        while(1); // Trava o sistema se o semáforo não puder ser criado
    }

    printf("Sistema inicializado. Criando tarefas FreeRTOS...\n");

    // Criação da Tarefa 1: Self-Test
    xTaskCreate(task_self_test,      // Função da tarefa
                "Self-Test Task",    // Nome da tarefa
                2048,                // Tamanho da pilha (aumentado por causa do printf)
                NULL,                // Parâmetros da tarefa
                1,                   // Prioridade
                &handle_self_test);  // Handle da tarefa
    

     // Criação da Tarefa 2: Alive Task
    xTaskCreate(task_alive,          // Função da tarefa
                "Alive Task",        // Nome da tarefa
                256,                 // Tamanho da pilha (tarefa simples, não precisa de muito)
                NULL,                // Parâmetros da tarefa
                2,                   // Prioridade (maior que a do self-test)
                &handle_alive_task); // Handle da tarefa

    // AQUI SERÃO CRIADAS AS TAREFAS  3 NO FUTURO

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();

    // O código nunca deve chegar aqui
    while (true) {}
}
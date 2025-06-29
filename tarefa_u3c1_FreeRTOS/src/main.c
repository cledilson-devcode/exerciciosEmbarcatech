//
// main.c
//

// Inclusão dos arquivos de cabeçalho necessários para o projeto.
#include "pico/stdlib.h"        // Funções padrão do SDK do Pico (ex: stdio_init_all).
#include "FreeRTOS.h"           // Definições e tipos centrais do FreeRTOS.
#include "task.h"               // API de gerenciamento de tarefas (xTaskCreate, vTaskCoreAffinitySet, etc.).
#include "queue.h"              // API de gerenciamento de filas (xQueueCreate).
#include "fsm_semaforo.h"       // Declaração da tarefa que controla a lógica do semáforo.
#include "display_contador.h"   // Declaração da tarefa que controla o display OLED.
#include "button_handler.h"     // Declaração da tarefa que monitora o botão.

// Fila global para comunicação entre a tarefa do semáforo e a do display.
// A FSM envia o tempo de contagem, e o display o recebe.
QueueHandle_t fila_oled;

// Handle (identificador) global para a tarefa do semáforo.
// É definido como global para que a tarefa do botão (button_handler) possa
// acessá-lo externamente para suspender e resumir sua execução.
TaskHandle_t hFSM = NULL;

// Ponto de entrada principal do programa.
int main() {
    // Inicializa toda a E/S padrão, habilitando a comunicação USB serial
    // para que a função printf funcione como uma ferramenta de depuração.
    stdio_init_all();

    // Cria a fila 'fila_oled'. Ela pode armazenar até 2 itens do tamanho de um 'int'.
    fila_oled = xQueueCreate(2, sizeof(int));

    // --- Criação da Tarefa do Semáforo ---
    // Cria a tarefa que executa a função 'tarefa_fsm_semaforo', com 512 bytes de
    // stack, prioridade 1, e armazena seu handle na variável global 'hFSM'.
    xTaskCreate(tarefa_fsm_semaforo, "FSM_Semaforo", 512, NULL, 1, &hFSM);
    // Fixa a tarefa do semáforo para executar exclusivamente no núcleo 0 do RP2040.
    // Isso é ideal para tarefas de tempo real, garantindo que não sejam interrompidas
    // por outras tarefas menos críticas.
    vTaskCoreAffinitySet(hFSM, (1 << 0));

    // --- Criação da Tarefa do Display ---
    // Cria a tarefa que executa 'tarefa_display_contador', com 1024 bytes de stack
    // (mais stack pois lida com display e buffers) e prioridade 1.
    TaskHandle_t hOLED = NULL;
    xTaskCreate(tarefa_display_contador, "OLED_Display", 1024, NULL, 1, &hOLED);
    // Fixa a tarefa do display para executar no núcleo 1. Isso isola a comunicação I2C
    // (que pode ser bloqueante) do núcleo 0, protegendo a precisão da tarefa do semáforo.
    vTaskCoreAffinitySet(hOLED, (1 << 1));

    // --- Criação da Tarefa do Botão ---
    // Cria a tarefa que executa 'button_task', com 256 bytes de stack e prioridade 1.
    TaskHandle_t hButton = NULL;
    xTaskCreate(button_task, "ButtonTask", 256, NULL, 1, &hButton);
    // Fixa a tarefa do botão no núcleo 1, compartilhando-o com a tarefa do display.
    vTaskCoreAffinitySet(hButton, (1 << 1));

    // Inicia o escalonador do FreeRTOS. A partir deste ponto, o FreeRTOS assume o
    // controle do processador e começa a gerenciar a execução das tarefas criadas.
    // Esta função nunca retorna.
    vTaskStartScheduler();

    // Este loop infinito é um código de segurança que nunca deve ser alcançado.
    // Se 'vTaskStartScheduler' retornar (ex: por falta de memória), o programa
    // ficará preso aqui em vez de travar de forma indefinida.
    while (1);
}
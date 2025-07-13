//---------------------------------------------------------------------------------------------//
// main.c
//---------------------------------------------------------------------------------------------//
#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#include "perifericos.h"
#include "tarefas.h"

// --- ALTERAÇÃO: Declaração dos novos handles ---
// Os handles são definidos aqui e declarados como 'extern' em tarefas.c
QueueHandle_t xQueueEventos;
SemaphoreHandle_t xMutexPrintf;
SemaphoreHandle_t xCountingSemBuzzer;

// Função principal, ponto de entrada do programa.
int main() {
    // Inicializa a comunicação serial via USB.
    stdio_init_all();
    // sleep_ms(2000); // Delay para garantir que o terminal serial esteja pronto
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }


    // Chama a função que inicializa os pinos de GPIO (LEDs, botões) e o ADC.
    init_perifericos();
    // Chama a função que configura o hardware de PWM para o buzzer.
    buzzer_pwm_init();

    printf("Sistema de Aquisicao - Tarefa Unidade 3\n");

    // --- ALTERAÇÃO: Criação dos novos recursos de sincronização ---

    // 1. Cria a fila para até 10 mensagens do tipo queue_msg_t.
    xQueueEventos = xQueueCreate(10, sizeof(queue_msg_t));
    if (xQueueEventos == NULL) {
        printf("ERRO: Nao foi possivel criar a fila.\n");
        while(1);
    }

    // 2. Cria o mutex para proteger o printf.
    xMutexPrintf = xSemaphoreCreateMutex();
    if (xMutexPrintf == NULL) {
        printf("ERRO: Nao foi possivel criar o mutex.\n");
        while(1);
    }

    // 3. Cria o semáforo contador para o buzzer (máx 2 acessos, começa com 0).
    xCountingSemBuzzer = xSemaphoreCreateCounting(2, 0);
    if (xCountingSemBuzzer == NULL) {
        printf("ERRO: Nao foi possivel criar o semaforo contador.\n");
        while(1);
    }

    // --- ALTERAÇÃO: Criação das novas Tarefas do FreeRTOS ---

    // Prioridades: Processamento > Leituras > Buzzer
    xTaskCreate(task_leitura_eixos, "Leitura Eixos", 256, NULL, 2, NULL);
    xTaskCreate(task_leitura_botao, "Leitura Botao", 256, NULL, 2, NULL);
    xTaskCreate(task_processamento, "Processamento", 1024, NULL, 3, NULL);
    xTaskCreate(task_controle_buzzer, "Controle Buzzer", 256, NULL, 1, NULL);

    // Inicia o escalonador do FreeRTOS. A partir deste ponto, o RTOS assume o controle.
    vTaskStartScheduler();

    // Este loop infinito nunca deve ser alcançado.
    while (true) {}
}
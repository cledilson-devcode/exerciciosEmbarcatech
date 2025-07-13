#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "perifericos.h"
#include "tarefas.h"

// Ponteiro para controlar a tarefa de autoteste (ex: para deletá-la).
TaskHandle_t handle_self_test = NULL;
// Ponteiro para controlar a tarefa de "sinal de vida".
TaskHandle_t handle_alive_task = NULL;
// Ponteiro para controlar a tarefa de monitoramento do joystick.
TaskHandle_t handle_monitor_task = NULL;

// Ponteiro para o semáforo que sincroniza o início das tarefas 2 e 3.
SemaphoreHandle_t self_test_sem = NULL;

// Função principal, ponto de entrada do programa.
int main() {
    // Inicializa a comunicação serial via USB.
    stdio_init_all();
    sleep_ms(2000);
    // Aguarda em um loop até que a conexão USB seja estabelecida com o computador.
    // while (!stdio_usb_connected()) {
    //     sleep_ms(200);
    // }

    // Chama a função que inicializa os pinos de GPIO (LEDs, botões) e o ADC.
    init_perifericos();
    // Chama a função que configura o hardware de PWM para o buzzer.
    buzzer_pwm_init();

    // Cria um semáforo binário. Ele será usado para sinalizar que a Tarefa 1 terminou.
    self_test_sem = xSemaphoreCreateBinary();
    // Verifica se o semáforo foi criado com sucesso. Trava o sistema em caso de falha.
    if (self_test_sem == NULL) {
        printf("ERRO: Nao foi possivel criar o semaforo.\n");
        while(1);
    }

    printf("Sistema inicializado. Criando tarefas FreeRTOS...\n");

    // --- Criação das Tarefas do FreeRTOS ---

    // Cria a Tarefa 1 (task_self_test) com pilha de 2048 bytes e prioridade 1 (baixa).
    xTaskCreate(task_self_test, "Self-Test Task", 2048, NULL, 1, &handle_self_test);
    // A linha abaixo, se descomentada, prenderia a Tarefa 1 para executar apenas no Núcleo 0.
    // vTaskCoreAffinitySet(handle_self_test, (1 << 0));

    // Cria a Tarefa 2 (task_alive) com pilha de 256 bytes e prioridade 2 (média).
    xTaskCreate(task_alive, "Alive Task", 256, NULL, 2, &handle_alive_task);
    // A linha abaixo, se descomentada, prenderia a Tarefa 2 para executar apenas no Núcleo 1.
    vTaskCoreAffinitySet(handle_alive_task, (1 << 1));

    // Cria a Tarefa 3 (task_monitor_joystick) com pilha de 1024 bytes e prioridade 3 (alta).
    xTaskCreate(task_monitor_joystick, "Monitor Task", 1024, NULL, 3, &handle_monitor_task);
    // A linha abaixo, se descomentada, prenderia a Tarefa 3 para executar apenas no Núcleo 0.
    // vTaskCoreAffinitySet(handle_monitor_task, (1 << 0));

    // Inicia o escalonador do FreeRTOS. A partir deste ponto, o RTOS assume o controle.
    vTaskStartScheduler();

    // Este loop infinito nunca deve ser alcançado, pois o escalonador nunca retorna.
    while (true) {}
}
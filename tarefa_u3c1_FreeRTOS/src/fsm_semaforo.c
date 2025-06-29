//
// fsm_semaforo.c
//

// Inclusão das bibliotecas e arquivos de cabeçalho necessários.
#include "fsm_semaforo.h"       // Definições específicas desta máquina de estados (pinos).
#include "semaforo_rgb.h"       // Funções para controlar o LED RGB via PWM.
#include "pico/stdlib.h"        // Funções padrão do SDK do Raspberry Pi Pico.
#include "FreeRTOS.h"           // Tipos e funções principais do FreeRTOS.
#include "task.h"               // Funções de gerenciamento de tarefas (vTaskDelayUntil, etc.).
#include "queue.h"              // Funções de gerenciamento de filas (xQueueSend).
#include <stdio.h>              // Funções de entrada/saída padrão (printf).

// Declara que 'fila_oled' é uma variável global definida em outro arquivo (main.c).
// É usada para enviar dados para a tarefa do display.
extern QueueHandle_t fila_oled;

// Constantes para definir os tempos de cada estado do semáforo em segundos.
#define TEMPO_VERMELHO_S 5
#define TEMPO_VERDE_S    5
#define TEMPO_AMARELO_S  3

// Constante para definir o nível de brilho dos LEDs (0-255).
#define BRIGHTNESS 128

// Função principal da tarefa que implementa a máquina de estados do semáforo.
void tarefa_fsm_semaforo(void *p) {
    // Inicializa os pinos do LED RGB para operarem com PWM.
    rgb_init(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO);
    
    // Variável para armazenar o último tempo de ativação da tarefa.
    // Usada por vTaskDelayUntil para manter uma frequência de execução precisa.
    TickType_t xLastWakeTime;

    // Imprime uma mensagem inicial no terminal para indicar o início da simulação.
    printf("--- Iniciando Simulacao do Semaforo com PWM ---\n");

    // Loop infinito que garante a execução contínua da tarefa.
    while (1) {
        // Obtém o tempo atual do sistema. Esta linha é crucial para ressicronizar a
        // tarefa após ser resumida de um estado de suspensão (pausa), garantindo
        // que o próximo delay seja calculado a partir do momento presente.
        xLastWakeTime = xTaskGetTickCount();

        // --- Início do ESTADO VERMELHO ---
        printf("Estado: VERMELHO (%d segundos)\n", TEMPO_VERMELHO_S);
        int tempo_vermelho = TEMPO_VERMELHO_S;
        // Envia o tempo de duração deste estado para a fila do display OLED.
        xQueueSend(fila_oled, &tempo_vermelho, portMAX_DELAY);
        // Define a cor do LED para vermelho com o brilho configurado.
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, BRIGHTNESS, 0, 0);
        // Pausa a tarefa até que o tempo exato do estado vermelho tenha passado.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_VERMELHO_S * 1000));

        // Ressincroniza o tempo antes de passar para o próximo estado.
        xLastWakeTime = xTaskGetTickCount();

        // --- Início do ESTADO VERDE ---
        printf("Estado: VERDE   (%d segundos)\n", TEMPO_VERDE_S);
        int tempo_verde = TEMPO_VERDE_S;
        // Envia o tempo para a fila do display.
        xQueueSend(fila_oled, &tempo_verde, portMAX_DELAY);
        // Define a cor do LED para verde.
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, 0, BRIGHTNESS, 0);
        // Pausa a tarefa pela duração exata do estado verde.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_VERDE_S * 1000));

        // Ressincroniza o tempo antes do último estado.
        xLastWakeTime = xTaskGetTickCount();

        // --- Início do ESTADO AMARELO ---
        printf("Estado: AMARELO (%d segundos)\n", TEMPO_AMARELO_S);
        int tempo_amarelo = TEMPO_AMARELO_S;
        // Envia o tempo para a fila do display.
        xQueueSend(fila_oled, &tempo_amarelo, portMAX_DELAY);
        // Define a cor do LED para amarelo (fusão de vermelho e verde).
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, BRIGHTNESS, BRIGHTNESS, 0);
        // Pausa a tarefa pela duração exata do estado amarelo, completando o ciclo.
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_AMARELO_S * 1000));
    }
}
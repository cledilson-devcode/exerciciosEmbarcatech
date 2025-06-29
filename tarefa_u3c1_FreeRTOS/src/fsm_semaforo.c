//
// fsm_semaforo.c
//
#include "fsm_semaforo.h"
#include "semaforo_rgb.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

extern QueueHandle_t fila_oled;

#define TEMPO_VERMELHO_S 5
#define TEMPO_VERDE_S    5
#define TEMPO_AMARELO_S  3

// NOVO: Define um nível de brilho para os LEDs (0-255)
// Um valor em torno de 128 representa ~50% de brilho, sendo mais suave.
#define BRIGHTNESS 64

void tarefa_fsm_semaforo(void *p) {
    rgb_init(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO);
    TickType_t xLastWakeTime = xTaskGetTickCount();

    printf("--- Iniciando Simulacao do Semaforo com PWM ---\n");

    while (1) {
        // --- ESTADO VERMELHO ---
        printf("Estado: VERMELHO (%d segundos)\n", TEMPO_VERMELHO_S);
        int tempo_vermelho = TEMPO_VERMELHO_S;
        xQueueSend(fila_oled, &tempo_vermelho, portMAX_DELAY);
        // Chama a função com o valor de brilho para o vermelho
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, BRIGHTNESS, 0, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_VERMELHO_S * 1000));

        // --- ESTADO VERDE ---
        printf("Estado: VERDE   (%d segundos)\n", TEMPO_VERDE_S);
        int tempo_verde = TEMPO_VERDE_S;
        xQueueSend(fila_oled, &tempo_verde, portMAX_DELAY);
        // Chama a função com o valor de brilho para o verde
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, 0, BRIGHTNESS, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_VERDE_S * 1000));

        // --- ESTADO AMARELO ---
        printf("Estado: AMARELO (%d segundos)\n", TEMPO_AMARELO_S);
        int tempo_amarelo = TEMPO_AMARELO_S;
        xQueueSend(fila_oled, &tempo_amarelo, portMAX_DELAY);
        // Chama a função com o valor de brilho para vermelho e verde
        rgb_set_color(LED_VERMELHO_GPIO, LED_VERDE_GPIO, LED_AZUL_GPIO, BRIGHTNESS, BRIGHTNESS, 0);
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(TEMPO_AMARELO_S * 1000));
    }
}
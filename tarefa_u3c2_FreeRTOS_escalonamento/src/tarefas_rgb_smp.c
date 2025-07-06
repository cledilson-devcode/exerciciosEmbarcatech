#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "leds_rgb.h"
#include "botoes.h"

// ============================================================================
// Tarefa de mensagem inicial: executa apenas uma vez após conexão USB
// ============================================================================
void task_mensagens(void *params) {
    printf("[Sistema] USB conectada. Iniciando FCFS com FreeRTOS SMP...\n");
    vTaskSuspend(NULL);
}

// ============================================================================
// Tarefa de autoteste geral: executa LEDs RGB A, B, C em sequência
// ============================================================================
void task_autoteste_geral(void *params) {
    test_rgb_sequence(LED_A_R, LED_A_G, LED_A_B);
    test_rgb_sequence(LED_B_R, LED_B_G, LED_B_B);
    test_rgb_sequence(LED_C_R, LED_C_G, LED_C_B);
    vTaskSuspend(NULL);
}

// ============================================================================
// Botão A: dispara teste no LED RGB A e imprime no terminal com info do núcleo
// ============================================================================
void task_botao_a(void *params) {
    while (1) {
        if (!gpio_get(BTN_A)) {
            printf("[Botão A] Iniciando autoteste do LED RGB A (núcleo %d)...\n", get_core_num());
            test_rgb_sequence(LED_A_R, LED_A_G, LED_A_B);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Botão B: dispara teste no LED RGB B e imprime no terminal com info do núcleo
// ============================================================================
void task_botao_b(void *params) {
    while (1) {
        if (!gpio_get(BTN_B)) {
            printf("[Botão B] Iniciando autoteste do LED RGB B (núcleo %d)...\n", get_core_num());
            test_rgb_sequence(LED_B_R, LED_B_G, LED_B_B);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

// ============================================================================
// Botão Joystick: dispara teste no LED RGB C e imprime no terminal com info do núcleo
// ============================================================================
void task_botao_joystick(void *params) {
    while (1) {
        if (!gpio_get(BTN_JOY)) {
            printf("[Joystick] Iniciando autoteste do LED RGB C (núcleo %d)...\n", get_core_num());
            test_rgb_sequence(LED_C_R, LED_C_G, LED_C_B);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#include <stdio.h>
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"

#include "leds_rgb.h"
#include "botoes.h"
#include "tarefas_rgb_smp.h"
#include "task_gerenciadora.h"  // Inclusão da tarefa gerenciadora

// Handles das tarefas (permite controle futuro, como suspensão ou deleção)
TaskHandle_t handle_mensagens = NULL;
TaskHandle_t handle_autoteste = NULL;
TaskHandle_t handle_botao_a = NULL;
TaskHandle_t handle_botao_b = NULL;
TaskHandle_t handle_botao_joystick = NULL;
TaskHandle_t handle_gerenciadora = NULL;

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(200);

    // Inicialização dos LEDs RGB
    init_rgb_led(LED_A_R, LED_A_G, LED_A_B);
    init_rgb_led(LED_B_R, LED_B_G, LED_B_B);
    init_rgb_led(LED_C_R, LED_C_G, LED_C_B);

    // Inicialização dos botões
    init_button(BTN_A);
    init_button(BTN_B);
    init_button(BTN_JOY);

    // ======================================================================
    // Criação das tarefas FreeRTOS com afinidade (SMP)
    // Handles usados para controle futuro e monitoramento
    // ======================================================================

    xTaskCreate(task_mensagens, "Mensagem", 1024, NULL, 1, &handle_mensagens);
    vTaskCoreAffinitySet(handle_mensagens, (1 << 0)); // Núcleo 0

    xTaskCreate(task_autoteste_geral, "AutoTest", 1024, NULL, 3, &handle_autoteste);
    vTaskCoreAffinitySet(handle_autoteste, (1 << 0)); // Núcleo 0

    xTaskCreate(task_botao_a, "BotaoA", 1024, NULL, 2, &handle_botao_a);
    vTaskCoreAffinitySet(handle_botao_a, (1 << 1)); // Núcleo 1

    xTaskCreate(task_botao_b, "BotaoB", 1024, NULL, 2, &handle_botao_b);
    vTaskCoreAffinitySet(handle_botao_b, (1 << 1)); // Núcleo 1

    xTaskCreate(task_botao_joystick, "BotJoy", 1024, NULL, 2, &handle_botao_joystick);
    vTaskCoreAffinitySet(handle_botao_joystick, (1 << 1)); // Núcleo 1

    // Criação da tarefa gerenciadora (prioridade mais alta)
    xTaskCreate(task_gerenciadora, "Manager", 1024, NULL, 4, &handle_gerenciadora);
    vTaskCoreAffinitySet(handle_gerenciadora, (1 << 0)); // Núcleo 0

    // Início do agendador SMP
    vTaskStartScheduler();

    while (true) {}
}

#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "tarefas_rgb_smp.h"
#include "pico/multicore.h" // necessário para get_core_num()

// ============================================================================
// Tarefa responsável por encerrar dinamicamente as tarefas dos botões
// após um período de tempo (15 segundos)
// ============================================================================
void task_gerenciadora(void *params) {
    printf("[Gerenciadora] Iniciando no núcleo %d\n", get_core_num());
    printf("[Gerenciadora] Aguardando 15 segundos para remover tarefas dos botões...\n");
    vTaskDelay(pdMS_TO_TICKS(15000));  // Delay inicial

    if (handle_botao_a) {
        printf("[Gerenciadora] Removendo tarefa Botão A...\n");
        vTaskDelete(handle_botao_a);
        handle_botao_a = NULL;
    }

    if (handle_botao_b) {
        printf("[Gerenciadora] Removendo tarefa Botão B...\n");
        vTaskDelete(handle_botao_b);
        handle_botao_b = NULL;
    }

    if (handle_botao_joystick) {
        printf("[Gerenciadora] Removendo tarefa Joystick...\n");
        vTaskDelete(handle_botao_joystick);
        handle_botao_joystick = NULL;
    }

    printf("[Gerenciadora] Finalizando a si mesma.\n");
    vTaskDelete(NULL); // A própria tarefa se autodeleta
}


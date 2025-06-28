#include "fsm_semaforo.h"
#include "semaforo_rgb.h"
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semaforo_rgb.h"

// Fila usada para enviar o tempo restante ao display OLED
extern QueueHandle_t fila_oled;

// Definição do novo botão (botão da Rua B)
#define BTN_B 6  // GPIO 6 utilizado como botão da Rua B

void tarefa_fsm_semaforo(void *p) {
    // Inicialização dos LEDs RGB dos veículos e pedestres
    rgb_init(AV_A_R, AV_A_G, AV_A_B);
    rgb_init(AV_B_R, AV_B_G, AV_B_B);
    rgb_init(PED_A_R, PED_A_G, PED_A_B);
    rgb_init(PED_B_R, PED_B_G, PED_B_B);

    // Inicialização do botão de pedestre
    gpio_init(BTN_PEDESTRE);
    gpio_set_dir(BTN_PEDESTRE, GPIO_IN);
    gpio_pull_up(BTN_PEDESTRE); // Ativa pull-up interno

    // Inicialização do botão da Rua B
    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B); // Ativa pull-up interno

    // Estados anteriores para debounce dos botões
    int estado_anterior_pedestre = 1;
    int estado_anterior_b = 1;

    // Variável para identificar se houve solicitação
    int pedido = 0;

    while (1) {
        // ================================
        // Fase Verde para veículos nas duas vias
        // ================================
        rgb_set_color(AV_A_R, AV_A_G, AV_A_B, 0, 1, 0);  // Verde A
        rgb_set_color(AV_B_R, AV_B_G, AV_B_B, 0, 1, 0);  // Verde B
        rgb_set_color(PED_A_R, PED_A_G, PED_A_B, 1, 0, 0); // Vermelho pedestre A
        rgb_set_color(PED_B_R, PED_B_G, PED_B_B, 1, 0, 0); // Vermelho pedestre B

        pedido = 0;

        // ================================
        // Espera por solicitação via botão (pedestre ou botão B)
        // ================================
        while (!pedido) {
            int leitura_pedestre = gpio_get(BTN_PEDESTRE);
            int leitura_b = gpio_get(BTN_B);

            // Verifica transição de 1 para 0 (botão pressionado)
            if ((leitura_pedestre == 0 && estado_anterior_pedestre == 1) ||
                (leitura_b == 0 && estado_anterior_b == 1)) {
                pedido = 1; // Solicitação detectada
            }

            estado_anterior_pedestre = leitura_pedestre;
            estado_anterior_b = leitura_b;

            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce e economia de CPU
        }

        // ================================
        // Contagem regressiva dos veículos antes da mudança
        // ================================
        int contador = 60;
        xQueueSend(fila_oled, &contador, portMAX_DELAY); // Envia tempo para display

        // Primeiros 51 segundos (verde constante)
        for (int i = 60; i >= 9; i--) {
            vTaskDelay(pdMS_TO_TICKS(1000)); // Espera de 1 segundo por passo
        }

        // Últimos 8 segundos com amarelo piscante para alertar os veículos
        for (int i = 9; i >= 1; i--) {
            rgb_set_color(AV_A_R, AV_A_G, AV_A_B, 1, 1, 0); // Amarelo A
            rgb_set_color(AV_B_R, AV_B_G, AV_B_B, 1, 1, 0); // Amarelo B
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // ================================
        // Fase Vermelha para veículos e Verde para pedestres
        // ================================
        rgb_set_color(AV_A_R, AV_A_G, AV_A_B, 1, 0, 0);  // Vermelho A
        rgb_set_color(AV_B_R, AV_B_G, AV_B_B, 1, 0, 0);  // Vermelho B
        rgb_set_color(PED_A_R, PED_A_G, PED_A_B, 0, 1, 0); // Verde pedestre A
        rgb_set_color(PED_B_R, PED_B_G, PED_B_B, 0, 1, 0); // Verde pedestre B

        contador = 20;
        xQueueSend(fila_oled, &contador, portMAX_DELAY);

        for (int i = 21; i >= 1; i--) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // ================================
        // Fim do ciclo - volta para verde dos veículos
        // ================================
        rgb_set_color(PED_A_R, PED_A_G, PED_A_B, 1, 0, 0);
        rgb_set_color(PED_B_R, PED_B_G, PED_B_B, 1, 0, 0);

        contador = 0;
        xQueueSend(fila_oled, &contador, portMAX_DELAY);

        vTaskDelay(pdMS_TO_TICKS(100)); // Breve pausa antes do próximo ciclo
    }
}

//
// button_handler.c
//

// Inclusão dos arquivos de cabeçalho necessários.
#include "button_handler.h"     // Declaração da própria tarefa (button_task).
#include "pico/stdlib.h"        // Funções de hardware do Pico, como controle de GPIO.
#include "FreeRTOS.h"           // Tipos e funções principais do FreeRTOS.
#include "task.h"               // API de gerenciamento de tarefas, incluindo vTaskDelay.
#include <stdio.h>              // Para a função printf, usada para depuração no terminal.

// Define o pino GPIO que será usado para o botão de pausa/resumo.
#define PAUSE_BUTTON_GPIO 5

// Declara que 'hFSM' é um handle para uma tarefa que é definida em outro arquivo (main.c).
// Isso permite que esta tarefa controle (suspenda/resuma) a tarefa do semáforo.
extern TaskHandle_t hFSM;

// Função principal da tarefa que monitora o botão.
void button_task(void *pvParameters) {
    // --- Configuração do Hardware ---
    // Inicializa o pino do botão.
    gpio_init(PAUSE_BUTTON_GPIO);
    // Define a direção do pino como entrada (INPUT).
    gpio_set_dir(PAUSE_BUTTON_GPIO, GPIO_IN);
    // Habilita o resistor de pull-up interno. Isso garante que, quando o botão
    // não está pressionado, o pino leia um nível lógico ALTO (true).
    gpio_pull_up(PAUSE_BUTTON_GPIO);

    // --- Variáveis de Controle de Estado ---
    // Armazena o estado do botão na iteração anterior para detectar a mudança.
    // Inicia como 'true' (liberado) por causa do pull-up.
    bool last_button_state = true;
    // Flag para rastrear se o semáforo está pausado ou não.
    bool is_paused = false;

    // Imprime uma mensagem no terminal para confirmar que a tarefa foi iniciada.
    printf("Tarefa de controle (Pausa/Resumo) iniciada.\n");

    // Loop infinito para manter a tarefa em execução contínua.
    while (1) {
        // Lê o estado atual do pino do botão.
        // Retorna 'false' (nível BAIXO) se pressionado, 'true' (nível ALTO) se liberado.
        bool current_button_state = gpio_get(PAUSE_BUTTON_GPIO);

        // --- Lógica de Detecção de Pressionamento (Debounce) ---
        // Verifica se o estado mudou de liberado (true) para pressionado (false).
        // Isso é chamado de "detecção de borda de descida" e garante que a ação
        // ocorra apenas uma vez por clique, e não continuamente enquanto o botão é segurado.
        if (last_button_state && !current_button_state) {
            
            // Inverte o estado de pausa (se estava pausado, volta a ativo, e vice-versa).
            is_paused = !is_paused;

            if (is_paused) {
                // Se o novo estado é 'pausado', suspende a execução da tarefa do semáforo.
                vTaskSuspend(hFSM);
                printf(">> SEMAFORO PAUSADO <<\n");
            } else {
                // Se o novo estado é 'ativo', resume a execução da tarefa do semáforo.
                vTaskResume(hFSM);
                printf(">> SEMAFORO REATIVADO <<\n");
            }
        }

        // Atualiza o estado anterior com o estado atual para a próxima verificação.
        last_button_state = current_button_state;

        // Pausa esta tarefa por 50 milissegundos.
        // Isso serve a dois propósitos:
        // 1. Debounce: Evita múltiplas leituras de um único clique devido a ruído mecânico.
        // 2. Yielding: Libera o processador para que outras tarefas possam executar.
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
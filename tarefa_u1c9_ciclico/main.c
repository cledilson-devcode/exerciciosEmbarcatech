/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Ciclo principal do sistema embarcado, utilizando timers
 *      para escalonamento de tarefas:
 *
 *      - Leitura da temperatura via DMA (iniciada por timer)
 *      - Exibição da temperatura e tendência no OLED (acionada por flag/timer)
 *      - Análise da tendência da temperatura (acionada por flag/timer)
 *      - Controle NeoPixel por tendência (acionada por flag/timer)
 *
 *      O sistema utiliza watchdog para segurança, terminal USB
 *      para monitoramento e display OLED para visualização direta.
 *
 *  
 *  
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "pico/util/queue.h" // Para possível comunicação segura entre ISRs e main, se necessário
#include "hardware/timer.h"   // Para add_repeating_timer_ms

#include "inc/setup.h"
#include "inc/tarefas/tarefa1_temp.h"
#include "inc/tarefas/tarefa2_display.h"
#include "inc/tarefas/tarefa3_tendencia.h"
#include "inc/tarefas/tarefa4_controla_neopixel.h"
#include "inc/neopixel_driver.h" // Para npSetAll, npWrite, npClear em tarefa_5
#include "inc/testes_cores.h"    // Para COR_BRANCA em tarefa_5
#include "pico/stdio_usb.h"

// Variáveis globais existentes
float media;
tendencia_t t;
absolute_time_t ini_tarefa1, fim_tarefa1, ini_tarefa2, fim_tarefa2, ini_tarefa3, fim_tarefa3, ini_tarefa4, fim_tarefa4; // Manter para medição, se desejado

// --- Interruptores de Software (Flags) ---
volatile bool iniciar_leitura_temp = true; // Inicia a primeira leitura imediatamente
volatile bool nova_media_calculada = false;
volatile bool executar_tarefas_dependentes = false; // Para agrupar análise, display, neopixel

// --- Protótipos das Funções de Tarefa (já existentes) ---
void tarefa_1_wrapper(); // Wrapper para medir tempo
void tarefa_2_wrapper(); // Wrapper para medir tempo
void tarefa_3_wrapper(); // Wrapper para medir tempo
void tarefa_4_wrapper(); // Wrapper para medir tempo
void tarefa_5();         // Tarefa extra

// --- Callbacks dos Timers ---
bool timer_callback_iniciar_temp(struct repeating_timer *rt) {
    iniciar_leitura_temp = true;
    return true; // Keep repeating
}

bool timer_callback_processar_dados(struct repeating_timer *rt) {
    if (nova_media_calculada) {
        executar_tarefas_dependentes = true;
        nova_media_calculada = false; // Resetar flag após sinalizar
    }
    return true; // Keep repeating
}

int main() {
    setup();  // Inicializações: ADC, DMA, interrupções, OLED, NeoPixel, etc.

    // Aguardar conexão USB para debug (opcional)
    // while (!stdio_usb_connected()) {
    //     sleep_ms(100);
    // }
    // printf("TempCycleDMA com Timers Iniciado\n");

    // Ativa o watchdog com timeout de ~2 segundos (ajuste conforme necessidade)
    // watchdog_enable(2000, 1);

    // --- Configuração dos Timers Repetitivos ---
    // Timer para iniciar a leitura de temperatura (ex: a cada 1200 ms, > 0.5s da tarefa + margem)
    struct repeating_timer timer_temp;
    add_repeating_timer_ms(1200, timer_callback_iniciar_temp, NULL, &timer_temp);

    // Timer para processar dados e atualizar saídas (ex: a cada 200 ms)
    struct repeating_timer timer_process;
    add_repeating_timer_ms(200, timer_callback_processar_dados, NULL, &timer_process);


    uint32_t ciclo_count = 0;

    while (true) {
        //watchdog_update();  // Alimente o watchdog

        if (iniciar_leitura_temp) {
            iniciar_leitura_temp = false; // Resetar flag
            
            // Medição de tempo para Tarefa 1
            ini_tarefa1 = get_absolute_time();
            media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL); // Esta função é bloqueante por ~0.5s
            fim_tarefa1 = get_absolute_time();
            
            nova_media_calculada = true; // Sinalizar que novos dados estão prontos
            // printf("T1 concluída. Média: %.2f\n", media); // Debug
        }

        if (executar_tarefas_dependentes) {
            executar_tarefas_dependentes = false; // Resetar flag

            // --- Tarefa 3 Lógica (Análise de tendência) ---
            // (Corrigindo a chamada original do main.c)
            ini_tarefa3 = get_absolute_time();
            t = tarefa3_analisa_tendencia(media);
            fim_tarefa3 = get_absolute_time();

            // --- Tarefa 2 Lógica (Exibição no OLED) ---
            // (Corrigindo a chamada original do main.c)
            ini_tarefa2 = get_absolute_time();
            tarefa2_exibir_oled(media, t);
            fim_tarefa2 = get_absolute_time();
            
            // --- Tarefa 4: Cor da matriz NeoPixel por tendência ---
            ini_tarefa4 = get_absolute_time();
            tarefa4_matriz_cor_por_tendencia(t);
            fim_tarefa4 = get_absolute_time();

            // --- Tarefa 5: Extra ---
            // A tarefa 5 pode ser chamada aqui ou ter seu próprio mecanismo de disparo/timer
            // Se chamada aqui, será executada com a frequência do timer_process quando há novos dados.
            // Se for uma tarefa de baixa prioridade ou que não dependa diretamente dos outros dados,
            // poderia ser executada no loop while(true) com menor frequência.
            // Por ora, vamos manter a chamada aqui para seguir o fluxo anterior.
            tarefa_5(); // Executa a lógica da tarefa 5

            // --- Cálculo e Exibição dos tempos de execução (a cada ciclo de processamento) ---
            int64_t tempo1_us = absolute_time_diff_us(ini_tarefa1, fim_tarefa1); // Será o último tempo da T1
            int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
            int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
            int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4); // Último tempo da T4

            printf("[%lu] Temp: %.2f°C | T1:%.3fs T2:%.3fs T3:%.3fs T4:%.3fs | Tend: %s\n",
                   ciclo_count++,
                   media,
                   tempo1_us / 1e6,
                   tempo2_us / 1e6,
                   tempo3_us / 1e6,
                   (tempo4_us > 0 && tempo4_us < 500000) ? tempo4_us / 1e6 : 0.0, // Evita tempos inválidos se T4 não rodou
                   tendencia_para_texto(t));
        }
        
        // O loop principal pode fazer outras coisas de baixa prioridade aqui
        // ou simplesmente ceder tempo (embora os timers já façam isso de certa forma).
        // __wfi(); // Pode ser usado para economizar energia se não houver mais nada a fazer
        //           // até a próxima interrupção de timer. Cuidado para não perder flags.
        //           // Se as flags são setadas em ISRs, a CPU acordará.
        
        // Pequeno delay para não sobrecarregar o printf se as flags não forem acionadas
        // por um tempo, ou para permitir que outras interrupções de menor prioridade ocorram.
        // No entanto, com os timers, o loop principal só age quando as flags são setadas.
        // Um __wfi() aqui pode ser mais eficiente.
        if (!iniciar_leitura_temp && !executar_tarefas_dependentes) {
             tight_loop_contents(); // Ou __wfi(); se apropriado
        }
    }

    return 0; // Nunca alcançado
}


// --- Wrappers das Tarefas (se for manter a medição de tempo como antes) ---
// As medições de tempo foram movidas para o loop principal para maior clareza.
// Estas funções são chamadas pelas funções originais (tarefa_1, tarefa_2, etc. do main original)
// mas como as chamadas diretas são mais claras, estes wrappers podem não ser mais necessários
// a menos que você queira restaurar a estrutura exata de chamada do main anterior.
// Por simplicidade, as chamadas diretas foram feitas no loop principal acima.

// A função tarefa_5() não precisa de wrapper se chamada diretamente.
// A lógica de `tarefa_5()` original é:
void tarefa_5() {
    // A lógica original de piscar LEDs se media < 1
    // Esta tarefa pode precisar de uma lógica de temporização não bloqueante
    // se for para piscar continuamente sem parar o resto.
    // No modelo atual, ela executa sua lógica uma vez quando chamada.
    if (media < 1.0f && media > -50.0f) { // Adicionada verificação de limite inferior razoável
        //printf("Tarefa 5: Média < 1 (%.2f), piscando NeoPixel.\n", media); // Debug
        npSetAll(COR_BRANCA);
        npWrite();
        busy_wait_ms(200); // Usar busy_wait para delays curtos em vez de sleep_ms que pode ceder
        npClear();
        npWrite();
        busy_wait_ms(200);
    }
}

// Manter os protótipos das funções de tarefa se não estiverem em headers
// (já estão, então não precisa declarar aqui)
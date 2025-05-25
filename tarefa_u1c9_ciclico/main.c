/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Monitora temperatura e atualiza display OLED e LEDs NeoPixel.
 *      Tarefas escalonadas por timers do RP2040, com I/O no loop principal.
 *
 *      Fluxo:
 *      1. Timer A sinaliza (flag) para Tarefa 1 (leitura de temperatura) no main.
 *      2. Tarefa 1 no main atualiza 'media' e sinaliza (flag) dados prontos.
 *      3. Timer B verifica dados prontos e sinaliza (flag) para o main processar
 *         as "demais tarefas" (análise, OLED, NeoPixel, extra).
 *      4. Main executa I/O e lógica quando flags ativas; dorme (__wfi()) ocioso.
 *
 *      Atendimento aos Requisitos:
 *      - Sincronização em função da primeira:
 *          Demais tarefas dependem da flag 'flag_media_foi_lida_e_esta_pronta',
 *          setada após Tarefa 1. Timer B age sobre esta flag.
 *      - add_repeating_timer_ms nas demais tarefas:
 *          Timer B (com add_repeating_timer_ms) dispara o início da sequência
 *          das "demais tarefas" (análise no callback, I/O sinalizada para o main).
 *      - repeating_timer_callback para a Tarefa 1:
 *          Timer A usa 'callback_disparar_leitura_temp' para sinalizar (flag)
 *          a execução da Tarefa 1 no main.
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include "pico/stdlib.h"
// #include "hardware/watchdog.h" // Para watchdog, se usado
#include "hardware/timer.h"      // Para timers
#include "hardware/sync.h"       // Para __wfi, tight_loop_contents

#include "inc/setup.h"
#include "inc/tarefas/tarefa1_temp.h"
#include "inc/tarefas/tarefa2_display.h"
#include "inc/tarefas/tarefa3_tendencia.h"
#include "inc/tarefas/tarefa4_controla_neopixel.h"
#include "inc/neopixel_driver.h"
#include "inc/testes_cores.h"
// #include "pico/stdio_usb.h" // Para printf, se usado


// --- Variáveis Globais ---
float media;     // Média da temperatura (Tarefa 1).
tendencia_t t; // Tendência térmica (Tarefa 3).

// --- Flags de Controle (volatile: acesso por ISR e main) ---
volatile bool flag_main_deve_ler_temp = true;             // Timer A: main deve iniciar Tarefa 1.
volatile bool flag_media_foi_lida_e_esta_pronta = false;  // Main: Tarefa 1 concluída, 'media' pronta.
volatile bool flag_main_deve_processar_dependentes = false; // Timer B: main deve processar tarefas 2,3,4,5.


// Tarefa 5: Animação NeoPixel. Chamada pelo main.
void executar_logica_tarefa_5_no_main_loop() {
    static uint8_t t5_counter = 0; // Alterna efeito.
    if (media < 1.0f && media > -50.0f) { // Executa se 'media' válida.
        if (t5_counter % 2 == 0) {
            npSetAll(COR_MIN, COR_MIN, COR_MIN); // Branco suave.
        } else {
            npClear(); // Apaga.
        }
        npWrite(); // Atualiza NeoPixels.
    }
    t5_counter++;
}

// --- Callbacks dos Timers (Executados em Interrupção) ---

// Timer A Callback: Sinaliza para main iniciar leitura de temperatura.
bool callback_disparar_leitura_temp(struct repeating_timer *rt) {
    // Só sinaliza se não houver leitura pendente ou dados não processados.
    if (!flag_main_deve_ler_temp && !flag_media_foi_lida_e_esta_pronta) {
        flag_main_deve_ler_temp = true; // Avisa main para executar Tarefa 1.
    }
    return true; // Continua o timer.
}

// Timer B Callback: Sinaliza para main processar tarefas dependentes.
bool callback_disparar_processamento_dependentes(struct repeating_timer *rt) {
    // Só sinaliza se 'media' está pronta e não há processamento dependente já agendado.
    if (flag_media_foi_lida_e_esta_pronta && !flag_main_deve_processar_dependentes) {
        flag_main_deve_processar_dependentes = true; // Avisa main para tarefas 3,2,4,5.
    }
    return true; // Continua o timer.
}

// Função principal.
int main() {
    setup();  // Inicializações de hardware e software.

    // watchdog_enable(3000, 1); // Opcional: habilita watchdog.

    // Configura Timer A (dispara Tarefa 1 via main).
    struct repeating_timer timer_t1_obj;
    if (!add_repeating_timer_ms(-1200, callback_disparar_leitura_temp, NULL, &timer_t1_obj)) {
        while(1){ /* Erro Timer A */ } // Trava em caso de erro.
    }

    // Configura Timer B (dispara tarefas dependentes via main).
    struct repeating_timer timer_t_deps_obj;
    if (!add_repeating_timer_ms(300, callback_disparar_processamento_dependentes, NULL, &timer_t_deps_obj)) {
         while(1){ /* Erro Timer B */ } // Trava em caso de erro.
    }
    
    // Loop principal: executa tarefas com base nas flags.
    while (true) {
        // watchdog_update(); // Opcional: alimenta watchdog.

        // Se Timer A sinalizou para Tarefa 1.
        if (flag_main_deve_ler_temp) {
            flag_main_deve_ler_temp = false; // Consome flag.
            media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL); // Executa Tarefa 1.
            flag_media_foi_lida_e_esta_pronta = true; // 'media' pronta para Tarefas Dependentes.
        }

        // Se Timer B sinalizou para Tarefas Dependentes.
        if (flag_main_deve_processar_dependentes) {
            flag_main_deve_processar_dependentes = false; // Consome flag.
            
            t = tarefa3_analisa_tendencia(media);   // Tarefa 3: Análise.
            tarefa2_exibir_oled(media, t);          // Tarefa 2: OLED.
            tarefa4_matriz_cor_por_tendencia(t);    // Tarefa 4: NeoPixel.
            executar_logica_tarefa_5_no_main_loop(); // Tarefa 5: Extra.
            
            flag_media_foi_lida_e_esta_pronta = false; // Dados da 'media' foram processados.

            // Diagnóstico opcional (manter comentado para teste inicial).
            // static uint32_t diag_cnt = 0;
            // if (stdio_usb_connected()) {
            //    printf("[%lu] T:%.2fC (%s)\n", diag_cnt++, media, tendencia_para_texto(t));
            // }
        }

        // Se não há trabalho imediato e 'media' não está pendente, economiza energia.
        if (!flag_main_deve_ler_temp && 
            !flag_main_deve_processar_dependentes &&
            !flag_media_foi_lida_e_esta_pronta) {
            __wfi(); // Dorme até próxima interrupção (timer).
        } else {
            // Se há trabalho ou 'media' aguarda processamento, mantém o loop ativo.
            tight_loop_contents(); 
        }
    }
    return 0; // Nunca alcançado.
}
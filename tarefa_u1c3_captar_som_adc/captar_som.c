/**
 * Projeto: captar_som
 * Lê um microfone via ADC usando timer periódico e controla LEDs Neopixel
 * proporcionalmente à amplitude do som.
 *
 */

#include <stdio.h>      // Para printf (depuração serial)
#include <math.h>       
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "neopixel.c"     // Driver Neopixel (incluído diretamente)

// ==========================================================================
// --- Configurações do Hardware e Aplicação ---
// ==========================================================================

// --- Microfone ---
// Canal ADC e Pino GPIO correspondente para o microfone.
// NOTA: O código original usava ADC2 (GPIO28). A tarefa sugere ADC0 (GPIO26).
// Ajuste MIC_ADC_CHANNEL para 0 se estiver usando GPIO26.
#define MIC_ADC_CHANNEL 2
#define MIC_ADC_PIN     (26 + MIC_ADC_CHANNEL) // Deriva o GPIO a partir do canal ADC

// --- Parâmetros ADC ---
#define ADC_CLOCK_DIV 96.f // Divisor para clock ADC (afeta taxa de amostragem máxima)
// ** AJUSTE NECESSÁRIO **: Valor ADC esperado em silêncio (nível DC).
// Meça o valor ADC real sem som e ajuste este valor para cálculo correto da amplitude.
#define ADC_OFFSET    2048
// ** AJUSTE NECESSÁRIO **: Amplitude máxima esperada do ADC (acima/abaixo do offset).
// Usado para mapear a amplitude para a faixa completa dos LEDs. Meça o pico e ajuste.
#define ADC_MAX_EXPECTED_AMP 800

// --- Timer ---
#define ADC_READ_INTERVAL_MS 20 // Intervalo (ms) para leitura do ADC via interrupção do timer.

// --- LEDs Neopixel ---
#define LED_PIN 7             // GPIO conectado ao pino de dados (DIN) dos LEDs.
#define LED_COUNT 25          // Número total de LEDs na matriz/fita.

// --- Macros Auxiliares ---
// #define abs(x) ((x < 0) ? -(x) : (x)) // Macro alternativa para valor absoluto

// ==========================================================================
// --- Variáveis Globais ---
// ==========================================================================
// Usadas para comunicação entre a ISR do timer e o loop principal.
// 'volatile' impede otimizações do compilador que poderiam causar leituras incorretas.
volatile uint16_t g_last_adc_value = ADC_OFFSET; // Armazena a leitura ADC mais recente.
volatile uint16_t g_current_amplitude = 0;       // Armazena a amplitude calculada (desvio do offset).

// ==========================================================================
// --- Callback da Interrupção do Timer ---
// ==========================================================================
/**
 * @brief Função chamada periodicamente pelo hardware do timer.
 *        Lê o ADC e calcula a amplitude instantânea do som.
 * @param t Ponteiro para a estrutura do timer (não utilizado aqui).
 * @return true para manter o timer agendado.
 */
bool repeating_timer_callback(struct repeating_timer *t) {
    uint16_t adc_raw;
    adc_select_input(MIC_ADC_CHANNEL); // Seleciona o canal correto
    adc_raw = adc_read();              // Lê o valor analógico
    g_last_adc_value = adc_raw;        // Atualiza valor global para depuração

    // Calcula a amplitude como o desvio absoluto do ponto de silêncio (offset)
    if (adc_raw >= ADC_OFFSET) {
        g_current_amplitude = adc_raw - ADC_OFFSET;
    } else {
        g_current_amplitude = ADC_OFFSET - adc_raw;
    }
    // Alternativa: g_current_amplitude = abs(adc_raw - ADC_OFFSET);

    return true; // Continua o timer
}

// ==========================================================================
// --- Função Principal ---
// ==========================================================================
int main() {
    // Inicialização básica do sistema
    stdio_init_all();
    sleep_ms(3000); // Pausa para permitir conexão do monitor serial
    printf("--- Projeto Falando Matriz (Timer Interrupt - Proporcional) ---\n");
    printf("SDK Target: 2.1.1\n");

    // --- Inicialização dos Periféricos ---
    printf("Preparando NeoPixel (GPIO%d, %d LEDs)...\n", LED_PIN, LED_COUNT);
    npInit(LED_PIN, LED_COUNT); // Inicializa o driver Neopixel
    npClear();                  // Garante que buffer Neopixel comece zerado
    npWrite();                  // Envia o estado inicial (apagado) para os LEDs

    printf("Preparando ADC (GPIO%d, Canal %d)...\n", MIC_ADC_PIN, MIC_ADC_CHANNEL);
    adc_gpio_init(MIC_ADC_PIN); // Configura o pino como entrada ADC
    adc_init();                 // Habilita hardware ADC
    adc_select_input(MIC_ADC_CHANNEL); // Seleciona qual canal ler
    adc_set_clkdiv(ADC_CLOCK_DIV);     // Define velocidade do clock ADC
    printf("ADC Configurado! Offset esperado: %d\n", ADC_OFFSET);

    printf("Preparando Timer Periodico (%d ms)...\n", ADC_READ_INTERVAL_MS);
    struct repeating_timer timer; // Estrutura para o contexto do timer
    // Configura o timer para chamar a callback periodicamente
    if (!add_repeating_timer_ms(ADC_READ_INTERVAL_MS, repeating_timer_callback, NULL, &timer)) {
        printf("ERRO: Nao foi possivel adicionar timer!\n");
        while(true); // Trava em caso de erro na configuração do timer
    }
    printf("Timer Configurado!\n");

    printf("\n----\nIniciando loop principal...\n----\n");

    // --- Loop Principal ---
    while (true) {
        // Pega a amplitude mais recente calculada pela ISR do timer
        // Copia para uma variável local para evitar condições de corrida (embora improvável aqui)
        uint16_t amplitude = g_current_amplitude;

        // --- Lógica de Resposta Visual ---

        // 1. Mapear a amplitude para o número de LEDs
        // Converte a amplitude lida (0 a ~ADC_MAX_EXPECTED_AMP) para uma escala de 0 a LED_COUNT.
        float mapped_value = 0.0f;
        if (amplitude > 0 && ADC_MAX_EXPECTED_AMP > 0) {
             mapped_value = ((float)amplitude / (float)ADC_MAX_EXPECTED_AMP) * (float)LED_COUNT;
        }
        uint8_t leds_to_light = (uint8_t)mapped_value; // Parte inteira é o número de LEDs

        // 2. Limitar e aplicar zona morta (opcional)
        uint8_t noise_threshold_leds = 1; // Ignora ruído que acenderia apenas 1 LED
        if (leds_to_light <= noise_threshold_leds) {
            leds_to_light = 0;
        }
        if (leds_to_light > LED_COUNT) { // Garante que não exceda o total de LEDs
            leds_to_light = LED_COUNT;
        }

        // 3. Determinar a cor baseada na intensidade
        uint8_t r_val = 0, g_val = 0, b_val = 0;
        if (leds_to_light > LED_COUNT * 0.7f) {      // Nível Alto
             r_val = 100; g_val = 0; b_val = 0;   // Vermelho
        } else if (leds_to_light > LED_COUNT * 0.3f) { // Nível Médio
             r_val = 0; g_val = 100; b_val = 0;  // Verde
        } else if (leds_to_light > 0) {             // Nível Baixo
             r_val = 0; g_val = 0; b_val = 100; // Azul
        }

        // 4. Atualizar o buffer Neopixel
        npClear(); // Começa limpando o buffer
        // Acende os LEDs sequencialmente (efeito de barra de VU)
        for (uint i = 0; i < leds_to_light; ++i) {
             npSetLED(i, r_val, g_val, b_val); // Define a cor para o i-ésimo LED no buffer
        }

        // 5. Enviar dados para os LEDs físicos
        npWrite();

        // --- Depuração ---
        // Descomente para ver os valores no monitor serial. Essencial para ajustar os #defines!
        // printf("ADC: %4u | Amp: %4u | LEDs: %2u \r", g_last_adc_value, amplitude, leds_to_light);

        // --- Controle da Taxa de Loop ---
        // Pequena pausa para controlar a taxa de atualização visual e liberar CPU.
        sleep_ms(30);
    }

    return 0; // Teórica - o loop while(true) nunca termina.
}
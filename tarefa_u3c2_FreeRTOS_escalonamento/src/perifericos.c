#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bitdoglab_pins.h"
#include "hardware/pwm.h"

// Constante para converter o valor bruto do ADC (0-4095) em tensão (0-3.3V).
const float ADC_CONVERSION_FACTOR = 3.3f / (1 << 12);

// Variável estática para armazenar o número do "slice" do PWM usado pelo buzzer.
// 'static' a torna visível apenas dentro deste arquivo.
static uint slice_num;

// Inicializa os pinos de GPIO e o hardware do ADC para todos os periféricos usados.
void init_perifericos() {
    // Configura os pinos dos LEDs RGB como saídas digitais.
    gpio_init(LED_RGB_R); gpio_set_dir(LED_RGB_R, GPIO_OUT);
    gpio_init(LED_RGB_G); gpio_set_dir(LED_RGB_G, GPIO_OUT);
    gpio_init(LED_RGB_B); gpio_set_dir(LED_RGB_B, GPIO_OUT);

    // Configura o pino do buzzer como saída digital (será reconfigurado para PWM depois).
    gpio_init(BUZZER_PIN); gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Configura os pinos dos botões como entradas com resistores de pull-up internos.
    gpio_init(BTN_A_PIN); gpio_set_dir(BTN_A_PIN, GPIO_IN); gpio_pull_up(BTN_A_PIN);
    gpio_init(BTN_B_PIN); gpio_set_dir(BTN_B_PIN, GPIO_IN); gpio_pull_up(BTN_B_PIN);
    gpio_init(BTN_JOY_SW_PIN); gpio_set_dir(BTN_JOY_SW_PIN, GPIO_IN); gpio_pull_up(BTN_JOY_SW_PIN);

    // Habilita o hardware do conversor analógico-digital.
    adc_init();
    // Associa os pinos físicos do joystick e microfone à função de ADC.
    adc_gpio_init(JOY_X_ADC_PIN);
    adc_gpio_init(JOY_Y_ADC_PIN);
    adc_gpio_init(MIC_ADC_PIN);
}

// Acende e apaga sequencialmente os LEDs R, G e B para um teste visual.
void test_leds_rgb() {
    printf("  [TESTE] Testando LEDs RGB...\n");
    gpio_put(LED_RGB_R, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_R, 0);
    gpio_put(LED_RGB_G, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_G, 0);
    gpio_put(LED_RGB_B, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_B, 0);
    printf("  [OK] LEDs RGB testados.\n");
}

// Lê o estado lógico (pressionado/não pressionado) de cada botão e imprime no terminal.
void test_botoes() {
    printf("  [TESTE] Testando Botoes (0 = Pressionado)...\n");
    printf("    - Botao A: %d\n", gpio_get(BTN_A_PIN));
    printf("    - Botao B: %d\n", gpio_get(BTN_B_PIN));
    printf("    - Botao Joystick: %d\n", gpio_get(BTN_JOY_SW_PIN));
    printf("  [OK] Botoes testados.\n");
}

// Lê os valores analógicos dos eixos X e Y do joystick e os converte para tensão.
void test_joystick_analog() {
    printf("  [TESTE] Testando Joystick Analogico...\n");

    // Seleciona o canal 0 do ADC (conectado ao eixo Y).
    adc_select_input(0);
    uint16_t y_raw = adc_read();
    printf("    - Eixo Y (ADC0): %.2f V\n", y_raw * ADC_CONVERSION_FACTOR);

    // Seleciona o canal 1 do ADC (conectado ao eixo X).
    adc_select_input(1);
    uint16_t x_raw = adc_read();
    printf("    - Eixo X (ADC1): %.2f V\n", x_raw * ADC_CONVERSION_FACTOR);
    
    printf("  [OK] Joystick Analogico testado.\n");
}

// Lê o valor analógico do microfone e o converte para tensão.
void test_microfone() {
    printf("  [TESTE] Testando Microfone...\n");

    // Seleciona o canal 2 do ADC (conectado ao microfone).
    adc_select_input(2);
    uint16_t mic_raw = adc_read();
    printf("    - Leitura Microfone (ADC2): %.2f V\n", mic_raw * ADC_CONVERSION_FACTOR);
    
    printf("  [OK] Microfone testado.\n");
}

// Configura o pino do buzzer para ser controlado pelo hardware de PWM.
void buzzer_pwm_init() {
    // Associa o pino do buzzer à função de PWM.
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    // Obtém o número do "slice" de PWM correspondente ao pino.
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // --- Estratégia para gerar a frequência desejada ---
    // Define um fator de divisão para o clock do sistema, desacelerando o contador do PWM.
    float divisor = 10.0f; 
    pwm_set_clkdiv(slice_num, divisor);

    // Define o valor máximo do contador (wrap) para gerar a frequência desejada (~262 Hz).
    uint16_t wrap_value = (125000000 / divisor) / 262 - 1;
    pwm_set_wrap(slice_num, wrap_value);

    // Define o "duty cycle" (tempo em que o sinal fica alto) para 10% do ciclo total.
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), wrap_value / 10);

    // Mantém o PWM desativado por padrão.
    pwm_set_enabled(slice_num, false);
}

// Ativa ou desativa o sinal PWM para o buzzer.
void buzzer_set_alarm(bool on) {
    pwm_set_enabled(slice_num, on);
}

// Realiza um teste audível no buzzer, ativando o PWM por 300ms.
void test_buzzer_pwm() { 
    printf("  [TESTE] Testando Buzzer com PWM...\n");
    buzzer_set_alarm(true); // Liga o som.
    vTaskDelay(pdMS_TO_TICKS(300)); // Mantém o som por 300ms.
    buzzer_set_alarm(false); // Desliga o som.
    printf("  [OK] Buzzer testado.\n");
}
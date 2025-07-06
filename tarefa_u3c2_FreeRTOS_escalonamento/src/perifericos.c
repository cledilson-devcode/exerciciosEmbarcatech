#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "FreeRTOS.h"
#include "task.h"
#include "bitdoglab_pins.h"
#include "hardware/pwm.h"

// Fator de conversão para o ADC de 12 bits (3.3V / 4095)
const float ADC_CONVERSION_FACTOR = 3.3f / (1 << 12);

// Variável global para guardar o "slice" do PWM
static uint slice_num;

/**
 * @brief Inicializa todos os periféricos necessários para a aplicação.
 */
void init_perifericos() {
    // Inicializa LEDs RGB como saída
    gpio_init(LED_RGB_R); gpio_set_dir(LED_RGB_R, GPIO_OUT);
    gpio_init(LED_RGB_G); gpio_set_dir(LED_RGB_G, GPIO_OUT);
    gpio_init(LED_RGB_B); gpio_set_dir(LED_RGB_B, GPIO_OUT);

    // Inicializa Buzzer como saída
    gpio_init(BUZZER_PIN); gpio_set_dir(BUZZER_PIN, GPIO_OUT);

    // Inicializa Botões como entrada com pull-up
    gpio_init(BTN_A_PIN); gpio_set_dir(BTN_A_PIN, GPIO_IN); gpio_pull_up(BTN_A_PIN);
    gpio_init(BTN_B_PIN); gpio_set_dir(BTN_B_PIN, GPIO_IN); gpio_pull_up(BTN_B_PIN);
    gpio_init(BTN_JOY_SW_PIN); gpio_set_dir(BTN_JOY_SW_PIN, GPIO_IN); gpio_pull_up(BTN_JOY_SW_PIN);

    // Inicializa o hardware do ADC
    adc_init();
    adc_gpio_init(JOY_X_ADC_PIN);
    adc_gpio_init(JOY_Y_ADC_PIN);
    adc_gpio_init(MIC_ADC_PIN);
}

/**
 * @brief Testa a sequência de cores do LED RGB.
 */
void test_leds_rgb() {
    printf("  [TESTE] Testando LEDs RGB...\n");
    gpio_put(LED_RGB_R, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_R, 0);
    gpio_put(LED_RGB_G, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_G, 0);
    gpio_put(LED_RGB_B, 1); vTaskDelay(pdMS_TO_TICKS(250)); gpio_put(LED_RGB_B, 0);
    printf("  [OK] LEDs RGB testados.\n");
}

// /**
//  * @brief Gera um som simples no buzzer.
//  */
// void test_buzzer() {
//     printf("  [TESTE] Testando Buzzer...\n");
//     gpio_put(BUZZER_PIN, 1);
//     vTaskDelay(pdMS_TO_TICKS(300));
//     gpio_put(BUZZER_PIN, 0);
//     printf("  [OK] Buzzer testado.\n");
// }

/**
 * @brief Lê e imprime o estado dos botões.
 */
void test_botoes() {
    printf("  [TESTE] Testando Botoes (0 = Pressionado)...\n");
    printf("    - Botao A: %d\n", gpio_get(BTN_A_PIN));
    printf("    - Botao B: %d\n", gpio_get(BTN_B_PIN));
    printf("    - Botao Joystick: %d\n", gpio_get(BTN_JOY_SW_PIN));
    printf("  [OK] Botoes testados.\n");
}

/**
 * @brief Lê e imprime as tensões dos eixos do joystick.
 */
void test_joystick_analog() {
    printf("  [TESTE] Testando Joystick Analogico...\n");

    // Leitura do Eixo Y (ADC0)
    adc_select_input(0); // Seleciona ADC0
    uint16_t y_raw = adc_read();
    printf("    - Eixo Y (ADC0): %.2f V\n", y_raw * ADC_CONVERSION_FACTOR);

    // Leitura do Eixo X (ADC1)
    adc_select_input(1); // Seleciona ADC1
    uint16_t x_raw = adc_read();
    printf("    - Eixo X (ADC1): %.2f V\n", x_raw * ADC_CONVERSION_FACTOR);
    
    printf("  [OK] Joystick Analogico testado.\n");
}

/**
 * @brief Lê e imprime a tensão do microfone.
 */
void test_microfone() { // <-- FUNÇÃO NOVA
    printf("  [TESTE] Testando Microfone...\n");

    // Leitura do Microfone (ADC2)
    adc_select_input(2); // Seleciona ADC2
    uint16_t mic_raw = adc_read();
    printf("    - Leitura Microfone (ADC2): %.2f V\n", mic_raw * ADC_CONVERSION_FACTOR);
    
    printf("  [OK] Microfone testado.\n");
}


/**
 * @brief Inicializa o hardware de PWM para o pino do buzzer.
 */
void buzzer_pwm_init() {
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);

    // --- ESTRATÉGIA CORRIGIDA PARA FREQUÊNCIAS BAIXAS ---
    // 1. Escolhemos um divisor para desacelerar o clock do PWM.
    // O divisor pode ser um número de ponto flutuante de 1.0 a 255.99...
    float divisor = 10.0f; 
    pwm_set_clkdiv(slice_num, divisor);

    // 2. Calculamos o valor de wrap para a frequência desejada com o novo clock.
    // Frequência desejada: ~262 Hz (Dó Central)
    // Wrap = (125MHz / divisor) / freq - 1
    uint16_t wrap_value = (125000000 / divisor) / 262 - 1;
    pwm_set_wrap(slice_num, wrap_value);

    // 3. Ajustamos o duty cycle para 50% do novo valor de wrap.
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER_PIN), wrap_value / 10);

    // Inicialmente, o PWM fica desabilitado
    pwm_set_enabled(slice_num, false);
}

/**
 * @brief Liga ou desliga o som do alarme.
 * @param on true para ligar, false para desligar.
 */
void buzzer_set_alarm(bool on) {
    pwm_set_enabled(slice_num, on);
}

/**
 * @brief Realiza um teste audível no buzzer usando PWM.
 */
void test_buzzer_pwm() { 
    printf("  [TESTE] Testando Buzzer com PWM...\n");
    buzzer_set_alarm(true); // Liga o som
    vTaskDelay(pdMS_TO_TICKS(300)); // Deixa tocar por 300ms
    buzzer_set_alarm(false); // Desliga o som
    printf("  [OK] Buzzer testado.\n");
}



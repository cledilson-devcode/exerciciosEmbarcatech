//
// semaforo_rgb.c
//
#include "pico/stdlib.h"
#include "semaforo_rgb.h"
#include "hardware/pwm.h" // NOVO: Biblioteca para controle PWM

// Inicializa os pinos de um LED RGB para serem controlados por PWM.
void rgb_init(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b) {
    const uint pins[] = {gpio_r, gpio_g, gpio_b};

    for (int i = 0; i < 3; ++i) {
        uint pin = pins[i];
        // 1. Informa ao GPIO que ele será controlado pela função PWM.
        gpio_set_function(pin, GPIO_FUNC_PWM);

        // 2. Encontra qual "slice" de PWM controla este pino.
        uint slice_num = pwm_gpio_to_slice_num(pin);

        // 3. Configura o período do PWM. Usamos 65535 (valor máximo de 16 bits)
        //    para ter uma alta resolução de brilho.
        pwm_config config = pwm_get_default_config();
        pwm_config_set_wrap(&config, 65535);
        pwm_init(slice_num, &config, true); // Carrega a config e habilita o PWM.

        // 4. Garante que o LED comece desligado (nível 0).
        pwm_set_gpio_level(pin, 0);
    }
}

// Define a cor e o brilho do LED RGB usando PWM.
void rgb_set_color(uint8_t gpio_r, uint8_t gpio_g, uint8_t gpio_b, 
                   uint8_t r_val, uint8_t g_val, uint8_t b_val) {
    // Para uma percepção de brilho mais linear (correção de gamma),
    // elevamos o valor de entrada ao quadrado. Isso faz com que a mudança
    // de brilho de 0 para 128 pareça mais suave e natural.
    // O valor de entrada (0-255) ao quadrado resulta em 0-65025, que se encaixa
    // perfeitamente no nosso período de 16 bits (0-65535).
    pwm_set_gpio_level(gpio_r, r_val * r_val);
    pwm_set_gpio_level(gpio_g, g_val * g_val);
    pwm_set_gpio_level(gpio_b, b_val * b_val);
}
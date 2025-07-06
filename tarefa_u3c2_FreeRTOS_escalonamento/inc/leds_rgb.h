#ifndef LEDS_RGB_H
#define LEDS_RGB_H

// ============================================================================
// Mapeamento dos pinos do kit BitDogLab para LEDs RGB
// Cada LED RGB tem um pino para os canais R, G e B
// ============================================================================

#define LED_A_R 13
#define LED_A_G 11
#define LED_A_B 12

#define LED_B_R 20
#define LED_B_G 19
#define LED_B_B 18

#define LED_C_R 8
#define LED_C_G 9
#define LED_C_B 4

// Prototipagem das funções de controle dos LEDs
void init_rgb_led(int r, int g, int b);
void clear_rgb(int r, int g, int b);
void test_rgb_sequence(int r, int g, int b);

#endif

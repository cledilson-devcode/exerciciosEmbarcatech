// inc/buzzer.h
#ifndef BUZZER_H
#define BUZZER_H

#include "pico/stdlib.h" // Para o tipo 'uint'

// Flag global para controlar a reprodução da música.
// 'volatile' é usado porque seu valor pode mudar de forma inesperada
// (por exemplo, por uma interrupção ou, neste caso, por outra parte do código no loop principal).
extern volatile bool g_play_music_flag;

// Declaração da função para inicializar o PWM para o buzzer
void buzzer_init(uint pin);

// Declaração da função para tocar a música Star Wars
void buzzer_play(uint pin);

#endif // BUZZER_MUSIC_H
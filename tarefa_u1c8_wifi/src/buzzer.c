// src/buzzer_music.c
#include "inc/buzzer.h" // Inclui o próprio cabeçalho
#include <stdio.h>
#include "hardware/pwm.h"
#include "hardware/clocks.h"

// Definição da flag global. Inicialmente, a música não está tocando.
volatile bool g_play_music_flag = false;


// --- NOVAS NOTAS E DURAÇÕES PARA SOM DE ALERTA ---
#define NOTE_DANGER_HIGH 1000 // Frequência em Hz para nota aguda do alerta
#define NOTE_DANGER_LOW  400  // Frequência em Hz para nota grave do alerta
#define DURATION_DANGER  150  // Duração de cada bip do alerta em ms

// Sequência de notas para o som de alerta
static const uint danger_alert_notes[] = {
    NOTE_DANGER_HIGH, NOTE_DANGER_LOW,
    NOTE_DANGER_HIGH, NOTE_DANGER_LOW,
    NOTE_DANGER_HIGH, NOTE_DANGER_LOW,
    NOTE_DANGER_HIGH, NOTE_DANGER_LOW,
    NOTE_DANGER_HIGH, NOTE_DANGER_LOW,
    0, // Uma pequena pausa no final do ciclo, se desejar
};

// Duração correspondente para cada nota do alerta
static const uint note_duration[] = {
    DURATION_DANGER, DURATION_DANGER,
    DURATION_DANGER, DURATION_DANGER,
    DURATION_DANGER, DURATION_DANGER,
    DURATION_DANGER, DURATION_DANGER,
    DURATION_DANGER, DURATION_DANGER,
    DURATION_DANGER * 0, // Duração da pausa (dobro da duração de um bip)
};


// // Notas musicais para a música tema de Star Wars (CORRIGIDO PARA 80 NOTAS)
// // !!! VERIFIQUE SE ESTAS NOTAS E DURAÇÕES CORRESPONDEM CORRETAMENTE APÓS O TRUNCAMENTO !!!
// static const uint star_wars_notes[] = {
//     330, 330, 330, 262, 392, 523, 330, 262, // 8
//     392, 523, 330, 659, 659, 659, 698, 523, // 16
//     415, 349, 330, 262, 392, 523, 330, 262, // 24
//     392, 523, 330, 659, 659, 659, 698, 523, // 32
//     415, 349, 330, 523, 494, 440, 392, 330, // 40
//     659, 784, 659, 523, 494, 440, 392, 330, // 48
//     659, 659, 330, 784, 880, 698, 784, 659, // 56
//     523, 494, 440, 392, 659, 784, 659, 523, // 64
//     494, 440, 392, 330, 659, 523, 659, 262, // 72
//     330, 294, 247, 262, 220, 262, 330, 262  // 80
// };

// // Duração das notas em milissegundos (80 durações)
// static const uint note_duration[] = {
//     500, 500, 500, 350, 150, 300, 500, 350,
//     150, 300, 500, 500, 500, 500, 350, 150,
//     300, 500, 500, 350, 150, 300, 500, 350,
//     150, 300, 650, 500, 150, 300, 500, 350,
//     150, 300, 500, 150, 300, 500, 350, 150,
//     300, 650, 500, 350, 150, 300, 500, 350,
//     150, 300, 500, 500, 500, 500, 350, 150,
//     300, 500, 500, 350, 150, 300, 500, 350,
//     150, 300, 500, 350, 150, 300, 500, 500,
//     350, 150, 300, 500, 500, 350, 150, 300,
// };

// Função auxiliar estática (interna a este módulo)
// Toca uma nota com a frequência e duração especificadas
static void play_tone(uint pin, uint frequency, uint duration_ms) {
    uint slice_num = pwm_gpio_to_slice_num(pin);
    uint32_t clock_freq = clock_get_hz(clk_sys);

    // Evita divisão por zero se a frequência for 0 (silêncio)
    if (frequency == 0) {
        pwm_set_gpio_level(pin, 0); // Desliga o PWM
        sleep_ms(duration_ms);
        return;
    }

    uint32_t top = clock_freq / frequency - 1;

    pwm_set_wrap(slice_num, top);
    pwm_set_gpio_level(pin, top / 2); // 50% de duty cycle

    sleep_ms(duration_ms);

    pwm_set_gpio_level(pin, 0); // Desliga o som após a duração
    sleep_ms(50); // Pausa entre notas
}

// Implementação da função pública para inicializar o PWM
void buzzer_init(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(pin);
    pwm_config config = pwm_get_default_config();
    // Ajuste o divisor de clock se necessário, 4.0f é um bom começo
    pwm_config_set_clkdiv(&config, 4.0f); 
    pwm_init(slice_num, &config, true);
    pwm_set_gpio_level(pin, 0); // Desliga o PWM inicialmente
}

// Implementação da função pública para tocar a música
void buzzer_play(uint pin) {
    // --- NOVO: Verificação no início da função ---
    if (!g_play_music_flag) {
        pwm_set_gpio_level(pin, 0); // Garante que o PWM está desligado
        return;
    }
    // --- FIM NOVO ---

    int num_notes = sizeof(note_duration) / sizeof(note_duration[0]);

    for (int i = 0; i < num_notes; i++) {
        // --- NOVO: Verificação antes de cada nota ---
        if (!g_play_music_flag) {
            pwm_set_gpio_level(pin, 0); // Desliga o som ao sair
            return; // Sai da função se a flag for falsa
        }
        // --- FIM NOVO ---

        if (i < (sizeof(danger_alert_notes) / sizeof(danger_alert_notes[0]))) {
            if (danger_alert_notes[i] == 0) {
                sleep_ms(note_duration[i]);
            } else {
                play_tone(pin, danger_alert_notes[i], note_duration[i]);
            }
        }
    }
    // --- NOVO: Se a música terminou e a flag ainda é true, podemos desabilitá-la
    // para que não comece automaticamente de novo, a menos que o main.c a reative.
    // Ou, se quisermos que ela toque continuamente enquanto a flag for true, não fazemos nada aqui.
    // Para "parar na hora que desejar", o comportamento de loop no main.c é mais adequado.
    // Se você quiser que ela toque apenas UMA VEZ por ativação da flag, descomente abaixo:
    // if (g_play_music_flag) {
    //     g_play_music_flag = false;
    // }
    // --- FIM NOVO ---
}
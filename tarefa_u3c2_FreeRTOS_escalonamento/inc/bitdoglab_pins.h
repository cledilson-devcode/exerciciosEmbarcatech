#ifndef BITDOGLAB_PINS_H
#define BITDOGLAB_PINS_H

// Mapeamento de Periféricos da Placa BitDogLab

// Tarefa 2: Alive Task
#define LED_ALIVE_PIN 13

// Tarefa 1: Self-Test - LEDs RGB
// Assumindo que o LED Vermelho (GPIO 13) também é o R do RGB
#define LED_RGB_R 13
#define LED_RGB_G 11
#define LED_RGB_B 12

// Tarefa 1: Self-Test & Tarefa 3: Alarme
#define BUZZER_PIN 21

// Tarefa 1: Self-Test - Botões
#define BTN_A_PIN 5
#define BTN_B_PIN 6
#define BTN_JOY_SW_PIN 22

// Tarefa 1 & 3: Joystick Analógico
#define JOY_X_ADC_PIN 27  // ADC1
#define JOY_Y_ADC_PIN 26  // ADC0

// Tarefa 1: Microfone
#define MIC_ADC_PIN 28    // ADC2

#endif
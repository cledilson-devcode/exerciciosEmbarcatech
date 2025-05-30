// Arquivo: funcoes_neopixel.h // Header para as funções de controle da fita NeoPixel.

#ifndef FUNCOES_NEOPIXEL_H // Guarda de inclusão.
#define FUNCOES_NEOPIXEL_H

#include <stdint.h>      // Para tipos inteiros de tamanho fixo como uint8_t.
#include "hardware/pio.h" // Para tipos e funções relacionadas ao PIO.
#include <time.h>        // Para time() (usado em inicializar_aleatorio, embora a chamada não esteja no main).
#include <stdlib.h>      // Para rand() e srand().
#include "hardware/adc.h" // Incluído, mas ADC não é usado diretamente nas funções NeoPixel.
#include "pico/types.h"   // Para tipos básicos do Pico SDK.

// ======= DEFINIÇÕES E CONSTANTES PARA NEOPIXEL =======

#define LED_COUNT 25      // Número total de LEDs NeoPixel na fita/matriz.
#define LED_PIN 7         // Pino GPIO ao qual o DADOS da fita NeoPixel está conectado.
#define NUM_COLUNAS 5     // Número de colunas na matriz de LEDs (se aplicável).
#define NUM_LINHAS 5      // Número de linhas na matriz de LEDs (se aplicável).

// Estrutura para representar a cor de um LED NeoPixel.
// A ordem (G, R, B) é importante para a forma como os dados são enviados pelo PIO.
typedef struct {
    uint8_t G, R, B;
} npLED_t;


// ======= DECLARAÇÕES DE VARIÁVEIS GLOBAIS EXTERNAS (para NeoPixel) =======

// Array que pode definir uma ordem de mapeamento customizada para os LEDs (definido em .c).
extern uint ordem[];
// Buffer principal que armazena o estado de cor de cada LED (definido em .c).
extern npLED_t leds[];

// Instância do PIO usada para controlar os NeoPixels (definida em .c).
extern PIO np_pio;
// Máquina de estados (SM) dentro do PIO usada (definida em .c).
extern uint sm;

// Índice do LED NeoPixel atualmente em foco, usado pela lógica principal da aplicação.
// Definido em Atividade_5.c, mas declarado extern aqui para ser acessível por funcoes_neopixel.c
// e outros arquivos que incluam este header (como funcao_atividade_.c).
extern volatile uint index_neo;


// ======= PROTÓTIPOS DAS FUNÇÕES NEOPIXEL =======
// Funções implementadas em funcoes_neopixel.c

// Inicializa o sistema NeoPixel (PIO, máquina de estados).
void npInit(uint pin);

// Define a cor de um LED específico no buffer 'leds'.
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b);

// Define a cor de todos os LEDs no buffer 'leds'.
void npSetAll(uint8_t r, uint8_t g, uint8_t b);

// Acende uma fileira de LEDs (para arranjos de matriz).
void acenderFileira(uint linha, uint8_t r, uint8_t g, uint8_t b, uint colunas);

// Acende uma coluna de LEDs e atualiza a fita (para arranjos de matriz).
void acender_coluna(uint8_t coluna, uint8_t r, uint8_t g, uint8_t b);

// Define todos os LEDs no buffer para preto (desligados).
void npClear();

// Envia os dados do buffer 'leds' para a fita NeoPixel física.
void npWrite();

// Define a cor de um LED específico e atualiza a fita imediatamente.
void npAcendeLED(uint index, uint8_t r, uint8_t g, uint8_t b);

// Inicializa o gerador de números aleatórios com uma semente baseada no tempo.
void inicializar_aleatorio();

// Gera um número aleatório dentro de um intervalo [min, max].
int numero_aleatorio(int min, int max);

#endif // FUNCOES_NEOPIXEL_H
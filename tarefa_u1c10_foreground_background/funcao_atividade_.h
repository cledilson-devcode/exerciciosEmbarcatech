// Arquivo: funcao_atividade_.h // Header com definições, constantes e protótipos para a lógica da atividade.

#ifndef FUNCAO_ATIVIDADE_3_H // Guarda de inclusão para evitar múltiplas definições.
#define FUNCAO_ATIVIDADE_3_H // O nome _3_H parece ser um resquício de uma versão anterior.

#include <stdio.h>          // Biblioteca padrão de I/O (para printf, etc.).
#include "pico/stdlib.h"     // Funções utilitárias principais do SDK do Pico.
#include "hardware/gpio.h"   // API para controle dos pinos GPIO.
#include "hardware/irq.h"    // API para gerenciamento de interrupções.
#include "hardware/sync.h"   // Para primitivas de sincronização, como __wfi().
#include "hardware/adc.h"    // Biblioteca para manipulação do ADC (não usada ativamente no código fornecido, mas incluída).
#include "pico/multicore.h"  // API para programação multicore no RP2040.


// ======= DEFINIÇÕES E CONSTANTES =======

#define DEBOUNCE_MS 40      // Tempo em milissegundos para o tratamento de debounce dos botões.
#define DELAY_MS 500        // Tempo de delay genérico em milissegundos (não usado ativamente no código fornecido).

// Definições dos pinos GPIO para os botões
#define BOTAO_A 5           // Botão A conectado ao GPIO 5.
#define BOTAO_B 6           // Botão B conectado ao GPIO 6.
#define BOTAO_JOYSTICK 22   // Botão do Joystick conectado ao GPIO 22.

// Definições dos pinos GPIO para os LEDs externos de status
#define LED_VERMELHO 13     // LED Vermelho conectado ao GPIO 13.
#define LED_AZUL 12         // LED Azul conectado ao GPIO 12.
#define LED_VERDE 11        // LED Verde conectado ao GPIO 11.

#define NUM_BOTOES 3        // Número total de botões sendo monitorados.

#define TAM_FILA 25         // Tamanho máximo da fila de eventos/tarefas.

// Constantes para ações (não usadas ativamente no código fornecido, mas definidas)
#define ACAO_INSERIR 1
#define ACAO_REMOVER 2

// ======= PROTÓTIPOS DAS FUNÇÕES =======
// Funções implementadas em funcao_atividade_.c

// Função de callback para tratar interrupções de GPIO.
void gpio_callback(uint gpio, uint32_t events);

// Função para inicializar um pino GPIO com direção e configurações de pull.
void inicializar_pino(uint pino, uint direcao, bool pull_up, bool pull_down);

// Função principal executada pelo Core 1 para tratar eventos e lógica dos LEDs/NeoPixel.
void tratar_eventos_leds();

// Função para imprimir o estado atual da fila no console.
void imprimir_fila();


// ======= DECLARAÇÕES DE VARIÁVEIS GLOBAIS EXTERNAS =======
// Estas variáveis são definidas em funcao_atividade_.c e acessadas por outros arquivos (como Atividade_5.c).

// Array constante com os pinos dos botões.
extern const uint BOTOES[NUM_BOTOES];
// Array constante com os pinos dos LEDs externos.
extern const uint LEDS[NUM_BOTOES];

// Flag volátil para sinalizar que o Core 1 terminou sua inicialização e está pronto.
extern volatile bool core1_pronto;

// Array volátil para indicar se há eventos pendentes para cada botão (não usado ativamente no fluxo principal atual).
extern volatile bool eventos_pendentes[NUM_BOTOES];
// Array volátil para armazenar o estado atual (ligado/desligado) dos LEDs externos.
extern volatile bool estado_leds[NUM_BOTOES];

#endif // FUNCAO_ATIVIDADE_3_H
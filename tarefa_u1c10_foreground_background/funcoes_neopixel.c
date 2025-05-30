// Arquivo: funcoes_neopixel.c // Implementação das funções de controle para a fita de LEDs WS2812B (NeoPixel).

#include "funcoes_neopixel.h" // Inclui definições, protótipos e estruturas relacionadas ao NeoPixel.
#include "ws2818b.pio.h"      // Inclui o header gerado pelo compilador PIO para o programa ws2818b.
#include "pico/stdlib.h"      // Funções utilitárias principais do SDK do Pico (para sleep_us, etc.).
#include "hardware/clocks.h"  // Para obter a frequência do clock do sistema (necessário para configurar o PIO).

// Array 'ordem': Define uma ordem customizada para mapear um índice linear para LEDs
// em um arranjo físico específico (ex: matriz serpentina).
// NOTA: Esta 'ordem' não é utilizada diretamente pela função 'npAcendeLED' no fluxo principal
// da Atividade_5, que usa o 'index_neo' diretamente como índice no buffer 'leds'.
// Poderia ser usada se a intenção fosse acender LEDs em uma sequência visual específica na matriz.
uint ordem[] = {
    4, 5, 14, 15, 24,
    3, 6, 13, 16, 23,
    2, 7, 12, 17, 22,
    1, 8, 11, 18, 21,
    0, 9, 10, 19, 20
};

// Buffer para armazenar o estado (cor) de cada LED NeoPixel.
// Cada elemento do array 'leds' corresponde a um LED na fita.
npLED_t leds[LED_COUNT]; // LED_COUNT é definido em funcoes_neopixel.h

// Variáveis globais para o PIO e a máquina de estados (SM) utilizada.
PIO np_pio; // Instância do PIO (pio0 ou pio1) a ser usada.
uint sm;    // Número da máquina de estados dentro do PIO selecionado.

// Função para inicializar o sistema NeoPixel.
void npInit(uint pin) {
    // Tenta adicionar o programa PIO (ws2818b_program) ao pio0.
    uint offset = pio_add_program(pio0, &ws2818b_program);
    np_pio = pio0; // Assume pio0 inicialmente.

    // Tenta requisitar uma máquina de estados não utilizada no pio0.
    sm = pio_claim_unused_sm(np_pio, false); // 'false' indica que não deve falhar se não houver SM livre.
    if (sm < 0) { // Se não houver SM livre no pio0...
        np_pio = pio1; // Tenta usar o pio1.
        sm = pio_claim_unused_sm(np_pio, true); // 'true' indica que deve falhar (assert) se não houver SM livre no pio1.
    }

    // Inicializa a máquina de estados com o programa PIO, configurando o pino e a frequência.
    // 800000.f é a frequência típica para LEDs WS2812B (800 KHz).
    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

    // Inicializa todos os LEDs no buffer para a cor preta (desligados).
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Define a cor de um LED específico no buffer.
// Não envia os dados para a fita; apenas atualiza o buffer interno.
void npSetLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < LED_COUNT) { // Verifica se o índice é válido.
        leds[index].R = r;
        leds[index].G = g;
        leds[index].B = b;
    }
}

// Define a cor de todos os LEDs no buffer para os mesmos valores RGB.
// Não envia os dados para a fita.
void npSetAll(uint8_t r, uint8_t g, uint8_t b) {
    for (uint i = 0; i < LED_COUNT; ++i) {
        leds[i].R = r;
        leds[i].G = g;
        leds[i].B = b;
    }
}

// Acende uma fileira de LEDs (assumindo um arranjo de matriz).
// 'colunas' define quantos LEDs compõem uma fileira.
// Não envia os dados para a fita.
void acenderFileira(uint linha, uint8_t r, uint8_t g, uint8_t b, uint colunas) {
    uint inicio = linha * colunas; // Calcula o índice do primeiro LED da fileira.
    for (uint i = 0; i < colunas; ++i) {
        uint index = inicio + i;
        if (index < LED_COUNT) { // Verifica se o índice é válido.
            npSetLED(index, r, g, b);
        }
    }
}

// Acende uma coluna de LEDs (assumindo um arranjo de matriz).
// Envia os dados para a fita imediatamente após definir as cores no buffer.
void acender_coluna(uint8_t coluna, uint8_t r, uint8_t g, uint8_t b) {
    // NUM_LINHAS e NUM_COLUNAS são definidos em funcoes_neopixel.h
    for (int linha = 0; linha < NUM_LINHAS; linha++) {
        uint index = linha * NUM_COLUNAS + coluna; // Calcula o índice do LED na coluna.
        // Acessa diretamente o buffer 'leds' e não chama npSetLED, o que é uma inconsistência de estilo.
        // Seria melhor usar npSetLED(index, r, g, b);
        if (index < LED_COUNT) { // Adicionada verificação para segurança
            leds[index].R = r;
            leds[index].G = g;
            leds[index].B = b;
        }
    }
    npWrite(); // Envia os dados atualizados para a fita.
}

// Define todos os LEDs no buffer para a cor preta (desligados).
// Não envia os dados para a fita.
void npClear() {
    for (uint i = 0; i < LED_COUNT; ++i)
        npSetLED(i, 0, 0, 0);
}

// Envia os dados de cor do buffer 'leds' para a fita NeoPixel.
// Usa a FIFO de transmissão da máquina de estados PIO.
// A ordem de envio (G, R, B) é comum para WS2812B, mas pode variar para outros chips.
void npWrite() {
    for (uint i = 0; i < LED_COUNT; ++i) {
        pio_sm_put_blocking(np_pio, sm, leds[i].G); // Envia componente Verde.
        pio_sm_put_blocking(np_pio, sm, leds[i].R); // Envia componente Vermelho.
        pio_sm_put_blocking(np_pio, sm, leds[i].B); // Envia componente Azul.
    }
    sleep_us(100); // Pequeno delay após a escrita, para garantir a latch dos dados pelos LEDs (alguns chips exigem).
}

// Função de conveniência: define a cor de um LED específico e envia imediatamente para a fita.
// Esta é a função principal usada pela lógica da Atividade_5 para controlar NeoPixels individualmente.
void npAcendeLED(uint index, uint8_t r, uint8_t g, uint8_t b) {
    if (index < LED_COUNT) { // Verifica se o índice é válido.
        npSetLED(index, r, g, b); // Define a cor no buffer.
        npWrite();                // Envia toda a configuração do buffer para a fita.
    }
}

// Inicializa o gerador de números aleatórios.
// Deve ser chamada uma vez no início do programa para garantir sequências diferentes a cada execução.
// NOTA: No projeto atual, esta função não é chamada explicitamente no `main`.
// Se não chamada, `rand()` pode produzir a mesma sequência de números a cada boot.
void inicializar_aleatorio() {
    srand(time(NULL)); // Usa o tempo atual como semente. Requer #include <time.h>.
}

// Gera um número aleatório dentro de um intervalo [min, max] (inclusivo).
int numero_aleatorio(int min, int max) {
    // Se inicializar_aleatorio() não for chamada, a semente para rand() será 1 por padrão.
    return rand() % (max - min + 1) + min;
}
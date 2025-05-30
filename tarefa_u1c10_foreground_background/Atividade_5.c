// Arquivo: Atividade_5.c // Ponto de entrada principal do programa, inicialização e gerenciamento dos núcleos.
#include "funcao_atividade_.h"   // Inclui definições de pinos, protótipos de funções de lógica e variáveis globais.
#include "funcoes_neopixel.h" // Inclui funções para controle da fita NeoPixel.

// Estrutura para definir uma cor RGB (não usada diretamente neste arquivo, mas definida no contexto do projeto)
typedef struct {
    uint8_t r, g, b;
} CorRGB;

// Array de cores pré-definidas (não usado diretamente neste arquivo, mas definido no contexto do projeto)
CorRGB cores[] = {
    {255, 0, 0}, {255, 64, 0}, {255, 128, 0}, {255, 192, 0},
    {255, 255, 0}, {192, 255, 0}, {128, 255, 0}, {0, 255, 0},
    {0, 255, 128}, {0, 255, 255}, {0, 128, 255}, {0, 0, 255},
    {128, 0, 255}, {255, 0, 255}, {255, 0, 128}, {255, 255, 255}
};

// Constante para o total de cores pré-definidas (não usada diretamente neste arquivo)
const size_t TOTAL_CORES = sizeof(cores) / sizeof(CorRGB);

// Variável global volátil para rastrear o índice do LED NeoPixel atual a ser manipulado.
// 'volatile' é usado porque esta variável pode ser modificada por diferentes contextos (ex: Core 1)
// e o compilador não deve otimizar o acesso a ela.
volatile uint index_neo = 0;

// Função principal do programa (executada no Core 0 por padrão)
int main() {
    // Inicializa a fita NeoPixel usando o pino definido em LED_PIN (de funcoes_neopixel.h)
    npInit(LED_PIN);
    npClear(); // Limpa todos os LEDs da fita (define para preto)
    npWrite(); // Envia o estado limpo para a fita

    // Inicializa todas as interfaces STDIO padrão (USB e/ou UART)
    stdio_init_all();

    // Inicializa os pinos dos LEDs externos (Vermelho, Azul, Verde)
    // Os pinos e o número de botões são definidos em funcao_atividade_.h
    for (int i = 0; i < NUM_BOTOES; i++) { // Reutiliza NUM_BOTOES para LEDs, assumindo um LED por botão/estado
        // Configura o pino do LED como saída (GPIO_OUT), sem pull-up/pull-down.
        inicializar_pino(LEDS[i], GPIO_OUT, false, false);
        gpio_put(LEDS[i], 0); // Desliga o LED inicialmente
        estado_leds[i] = false; // Atualiza o estado do LED no array de controle
    }

    // Inicializa os pinos dos botões (A, B, Joystick)
    for (int i = 0; i < NUM_BOTOES; i++) {
        // Configura o pino do botão como entrada (GPIO_IN) com resistor de pull-up interno habilitado.
        inicializar_pino(BOTOES[i], GPIO_IN, true, false);
    }

    // Lança a função 'tratar_eventos_leds' para ser executada no Core 1.
    // O Core 1 será responsável pela lógica principal da aplicação (tratar botões, atualizar NeoPixels).
    multicore_launch_core1(tratar_eventos_leds);

    // Aguarda até que o Core 1 sinalize que está pronto.
    // 'core1_pronto' é uma flag volátil definida em funcao_atividade_.c e modificada pelo Core 1.
    while (!core1_pronto);

    // Configurações de interrupção para o Core 0.
    // O Core 0 será responsável por detectar os pressionamentos de botões via interrupção.
    gpio_set_irq_callback(gpio_callback); // Define a função 'gpio_callback' como handler das interrupções de GPIO.
    irq_set_enabled(IO_IRQ_BANK0, true);  // Habilita as interrupções do banco GPIO.

    // Habilita interrupções para cada botão, disparando em borda de descida (GPIO_IRQ_EDGE_FALL).
    // Isso significa que a interrupção ocorre quando o botão é pressionado (nível lógico vai de ALTO para BAIXO devido ao pull-up).
    for (int i = 0; i < NUM_BOTOES; i++) {
        gpio_set_irq_enabled(BOTOES[i], GPIO_IRQ_EDGE_FALL, true);
    }

    // Loop principal do Core 0.
    // O Core 0 entra em modo de espera de baixa energia ('wait for interrupt').
    // Ele permanecerá neste estado até que uma interrupção (como o pressionar de um botão) ocorra.
    while (true) {
        __wfi(); // Entra em modo de espera para economizar energia.
    }
    // O programa nunca deve sair deste loop em operação normal.
    return 0; // Retorno padrão, embora inalcançável.
}
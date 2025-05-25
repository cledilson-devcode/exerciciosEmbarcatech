/**
 * Controla um semáforo de trânsito interativo com acionamento por pedestre.
 * Gerencia os estados do semáforo (vermelho, verde, amarelo) e o tempo de cada um
 * usando alarmes de hardware. Responde a botões de pedestre via interrupções GPIO.
 * O acionamento de pedestre só tem efeito se o sinal de carros estiver verde.
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"  

// --- Definições de Pinos ---
#define LED_VERMELHO_PIN 13
#define LED_VERDE_PIN    11
#define LED_AZUL_PIN     12 
#define BOTAO_A_PIN      5
#define BOTAO_B_PIN      6

// --- Tempos do Semáforo (em milissegundos) ---
#define TEMPO_VERMELHO_MS (10 * 1000)
#define TEMPO_VERDE_MS    (10 * 1000)
#define TEMPO_AMARELO_MS  (3 * 1000)

// --- Estados do Semáforo ---
typedef enum {
    ESTADO_VERMELHO,
    ESTADO_VERDE,
    ESTADO_AMARELO_CICLO,
    ESTADO_AMARELO_PEDESTRE
} estado_semaforo_t;

// --- Variáveis Globais ---
volatile estado_semaforo_t g_estado_atual = ESTADO_VERMELHO;
alarm_id_t g_alarme_id_atual = 0; // ID do alarme ativo, para cancelamento
volatile bool g_pedido_pedestre_pendente = false; // Sinaliza se um botão foi pressionado

// ==========================================================================
// --- Protótipos de Função ---
// ==========================================================================
void configurar_leds();
void acender_vermelho();
void acender_verde();
void acender_amarelo();
void apagar_todos_leds();

void iniciar_estado_vermelho();
void iniciar_estado_verde();
void iniciar_estado_amarelo_ciclo();
void iniciar_estado_amarelo_pedestre();

int64_t callback_para_verde(alarm_id_t id, void *user_data);
int64_t callback_para_amarelo_ciclo(alarm_id_t id, void *user_data);
int64_t callback_para_vermelho_apos_amarelo_ciclo(alarm_id_t id, void *user_data);
int64_t callback_para_vermelho_apos_amarelo_pedestre(alarm_id_t id, void *user_data);

void configurar_botoes_pedestre();
void gpio_callback_botoes_pedestre(uint gpio, uint32_t events);

// ==========================================================================
// --- Função Principal ---
// ==========================================================================
int main() {
    // Inicializa stdio para comunicação serial.
    stdio_init_all();
    // Pausa para permitir que o monitor serial seja conectado.
    sleep_ms(2000);

    printf("Semáforo Interativo - (Acionamento Condicional)\n");
    printf("Inicializando...\n");

    // Configura os pinos dos LEDs e botões.
    configurar_leds();
    configurar_botoes_pedestre();

    printf("Iniciando ciclo do semáforo...\n");
    // Define o estado inicial do semáforo como vermelho.
    iniciar_estado_vermelho();

    // Loop principal fica em espera por interrupções.
    // Toda a lógica do semáforo é tratada por alarmes e callbacks de GPIO.
    while (true) {
        __wfi(); // Entra em modo de baixo consumo até a próxima interrupção.
    }
    return 0; // Nunca alcançado.
}

// ==========================================================================
// --- Implementações das Funções ---
// ==========================================================================

// Configura os pinos dos LEDs como saída e os apaga inicialmente.
void configurar_leds() {
    gpio_init(LED_VERMELHO_PIN); gpio_set_dir(LED_VERMELHO_PIN, GPIO_OUT);
    gpio_init(LED_VERDE_PIN);    gpio_set_dir(LED_VERDE_PIN, GPIO_OUT);
    gpio_init(LED_AZUL_PIN);  gpio_set_dir(LED_AZUL_PIN, GPIO_OUT); // Pino para LED amarelo dedicado
    apagar_todos_leds();
}

// Acende o LED vermelho e apaga os outros. Imprime o estado no serial.
void acender_vermelho() {
    gpio_put(LED_VERMELHO_PIN, 1); gpio_put(LED_VERDE_PIN, 0); gpio_put(LED_AZUL_PIN, 0);
    printf("Sinal: Vermelho\n");
}

// Acende o LED verde e apaga os outros. Imprime o estado no serial.
void acender_verde() {
    gpio_put(LED_VERMELHO_PIN, 0); gpio_put(LED_VERDE_PIN, 1); gpio_put(LED_AZUL_PIN, 0);
    printf("Sinal: Verde\n");
}

// Acende o LED amarelo e apaga os outros. Imprime o estado no serial.
// (Nota: código original para BitDogLab usava V+R para amarelo, aqui usa LED dedicado).
void acender_amarelo() {
    gpio_put(LED_VERMELHO_PIN, 1); gpio_put(LED_VERDE_PIN, 1); gpio_put(LED_AZUL_PIN, 0);
    printf("Sinal: Amarelo\n");
}

// Apaga todos os LEDs do semáforo.
void apagar_todos_leds() {
    gpio_put(LED_VERMELHO_PIN, 0); gpio_put(LED_VERDE_PIN, 0); gpio_put(LED_AZUL_PIN, 0);
}


// Define o estado do semáforo como VERMELHO e agenda a próxima transição para VERDE.
// Limpa qualquer pedido de pedestre pendente.
void iniciar_estado_vermelho() {
    g_estado_atual = ESTADO_VERMELHO;
    acender_vermelho();
    if (g_alarme_id_atual > 0) cancel_alarm(g_alarme_id_atual);
    g_alarme_id_atual = add_alarm_in_ms(TEMPO_VERMELHO_MS, callback_para_verde, NULL, false);
    g_pedido_pedestre_pendente = false;
}

// Define o estado do semáforo como VERDE e agenda a próxima transição para AMARELO_CICLO.
void iniciar_estado_verde() {
    g_estado_atual = ESTADO_VERDE;
    acender_verde();
    if (g_alarme_id_atual > 0) cancel_alarm(g_alarme_id_atual);
    g_alarme_id_atual = add_alarm_in_ms(TEMPO_VERDE_MS, callback_para_amarelo_ciclo, NULL, false);
}

// Define o estado do semáforo como AMARELO_CICLO e agenda a próxima transição para VERMELHO.
void iniciar_estado_amarelo_ciclo() {
    g_estado_atual = ESTADO_AMARELO_CICLO;
    acender_amarelo();
    if (g_alarme_id_atual > 0) cancel_alarm(g_alarme_id_atual);
    g_alarme_id_atual = add_alarm_in_ms(TEMPO_AMARELO_MS, callback_para_vermelho_apos_amarelo_ciclo, NULL, false);
}

// Inicia a sequência de AMARELO para atender ao pedestre.
// Cancela o alarme do ciclo de tráfego atual e agenda a transição para VERMELHO após o amarelo.
void iniciar_estado_amarelo_pedestre() {
    g_estado_atual = ESTADO_AMARELO_PEDESTRE;
    acender_amarelo();
    if (g_alarme_id_atual > 0) cancel_alarm(g_alarme_id_atual);
    g_alarme_id_atual = add_alarm_in_ms(TEMPO_AMARELO_MS, callback_para_vermelho_apos_amarelo_pedestre, NULL, false);
}


// Callback de alarme: Transita do estado VERMELHO para VERDE.
int64_t callback_para_verde(alarm_id_t id, void *user_data) {
    g_alarme_id_atual = 0; // Alarme consumido.
    iniciar_estado_verde();
    return 0; // Não repetir alarme.
}

// Callback de alarme: Transita do estado VERDE para AMARELO_CICLO.
// Verifica se um pedestre solicitou passagem enquanto estava verde.
int64_t callback_para_amarelo_ciclo(alarm_id_t id, void *user_data) {
    g_alarme_id_atual = 0;
    if (g_pedido_pedestre_pendente) {
        // Se um botão foi pressionado muito perto do fim do verde, atende ao pedestre.
        printf("Pedido de pedestre atendido no final do verde.\n");
        g_pedido_pedestre_pendente = false;
        iniciar_estado_amarelo_pedestre();
    } else {
        iniciar_estado_amarelo_ciclo();
    }
    return 0;
}

// Callback de alarme: Transita do AMARELO_CICLO para VERMELHO, reiniciando o ciclo.
int64_t callback_para_vermelho_apos_amarelo_ciclo(alarm_id_t id, void *user_data) {
    g_alarme_id_atual = 0;
    iniciar_estado_vermelho();
    return 0;
}

// Callback de alarme: Transita do AMARELO_PEDESTRE para VERMELHO, reiniciando o ciclo.
int64_t callback_para_vermelho_apos_amarelo_pedestre(alarm_id_t id, void *user_data) {
    g_alarme_id_atual = 0;
    printf("Atendendo pedestre - Sinal Vermelho para carros.\n");
    iniciar_estado_vermelho();
    return 0;
}

// Configura os pinos dos botões de pedestre como entrada com pull-up
// e habilita interrupções na borda de descida.
void configurar_botoes_pedestre() {
    gpio_init(BOTAO_A_PIN); gpio_set_dir(BOTAO_A_PIN, GPIO_IN); gpio_pull_up(BOTAO_A_PIN);
    gpio_init(BOTAO_B_PIN); gpio_set_dir(BOTAO_B_PIN, GPIO_IN); gpio_pull_up(BOTAO_B_PIN);

    gpio_set_irq_enabled_with_callback(BOTAO_A_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_botoes_pedestre);
    gpio_set_irq_enabled_with_callback(BOTAO_B_PIN, GPIO_IRQ_EDGE_FALL, true, &gpio_callback_botoes_pedestre);
}

// Callback de interrupção para os botões de pedestre.
// Implementa debounce e aciona a sequência de pedestre se o sinal estiver VERDE.
void gpio_callback_botoes_pedestre(uint gpio, uint32_t events) {
    // Variáveis estáticas para debounce individual de cada botão.
    static uint32_t ultima_vez_botao_a_ms = 0;
    static uint32_t ultima_vez_botao_b_ms = 0;
    uint32_t agora_ms = to_ms_since_boot(get_absolute_time());
    uint32_t *p_ultima_vez_ms;

    // Seleciona a variável de tempo do botão correto.
    if (gpio == BOTAO_A_PIN) p_ultima_vez_ms = &ultima_vez_botao_a_ms;
    else if (gpio == BOTAO_B_PIN) p_ultima_vez_ms = &ultima_vez_botao_b_ms;
    else return; // Interrupção de pino não esperado.

    // Lógica de Debounce: ignora acionamentos muito rápidos.
    if (agora_ms - *p_ultima_vez_ms < 500) {
        return;
    }
    *p_ultima_vez_ms = agora_ms;

    printf("Botão de Pedestres (GPIO %u) acionado\n", gpio);

    // Ação do botão: só tem efeito se o semáforo de carros estiver VERDE.
    if (g_estado_atual == ESTADO_VERDE) {
        printf("Sinal estava VERDE. Iniciando ciclo para pedestre.\n");
        
        iniciar_estado_amarelo_pedestre(); // Inicia a sequência de amarelo para pedestre.
    } else {
        printf("Botão pressionado, mas sinal não estava VERDE (atual: %d). Pedido ignorado por agora.\n", g_estado_atual);
       
    }
}
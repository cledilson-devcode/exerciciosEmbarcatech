// funcao_atividade_.c

#include "funcao_atividade_.h"
#include "funcoes_neopixel.h" // Para npClear, npWrite e extern index_neo

// Variáveis da fila e contador de eventos
int fila[TAM_FILA];
int inicio = 0;
int fim = 0;
int quantidade = 0;
int contador = 0; // Contador de eventos/tarefas

// Timers para debounce (não modificado, mas presente no original)
absolute_time_t ultimo_toque[NUM_BOTOES];

// Mapeamento de botões e LEDs (não modificado)
const uint BOTOES[NUM_BOTOES] = {BOTAO_A, BOTAO_B, BOTAO_JOYSTICK};
const uint LEDS[NUM_BOTOES]   = {LED_VERMELHO, LED_AZUL, LED_VERDE};

// Flags de eventos e estados (não modificado)
volatile bool eventos_pendentes[NUM_BOTOES] = {false, false, false};
volatile bool estado_leds[NUM_BOTOES] = {false, false, false};
volatile bool core1_pronto = false;

// Callback de interrupção GPIO (não modificado)
void gpio_callback(uint gpio, uint32_t events) {
    for (int i = 0; i < NUM_BOTOES; i++) {
        if (gpio == BOTOES[i] && (events & GPIO_IRQ_EDGE_FALL)) {
            multicore_fifo_push_blocking(i);  // sem desabilitar a interrupção
        }
    }
}

// Inicialização de pino GPIO (não modificado)
void inicializar_pino(uint pino, uint direcao, bool pull_up, bool pull_down) {
    gpio_init(pino);
    gpio_set_dir(pino, direcao);

    if (direcao == GPIO_IN) {
        if (pull_up) {
            gpio_pull_up(pino);
        } else if (pull_down) {
            gpio_pull_down(pino);
        } else {
            gpio_disable_pulls(pino);
        }
    }
}

// Função principal do Core 1 para tratar eventos
void tratar_eventos_leds() {
    core1_pronto = true;

    while (true) {
        uint32_t id1 = multicore_fifo_pop_blocking();  // Aguarda botão pressionado

        sleep_ms(DEBOUNCE_MS); // Debounce

        // Confirma se ainda está pressionado
        if (!gpio_get(BOTOES[id1])) {
            // Verifica se outro botão também está pressionado → ignora se sim
            bool outro_pressionado = false;
            for (int i = 0; i < NUM_BOTOES; i++) {
                if (i != id1 && !gpio_get(BOTOES[i])) {
                    outro_pressionado = true;
                    break;
                }
            }
            if (outro_pressionado) {
                // Espera soltar ambos
                while (!gpio_get(BOTOES[id1])) tight_loop_contents();
                continue;
            }

            // Ações de incremento ou decremento para BOTÃO A e BOTÃO B
            if (id1 == 0 && index_neo < LED_COUNT) {  // BOTÃO A → incrementa
                uint8_t r = numero_aleatorio(1, 255);
                uint8_t g = numero_aleatorio(1, 255);
                uint8_t b = numero_aleatorio(1, 255);
                npAcendeLED(index_neo, r, g, b);
                index_neo++;

                if (quantidade < TAM_FILA) {
                    fila[fim] = contador++; // Usa contador e depois incrementa
                    fim = (fim + 1) % TAM_FILA;
                    quantidade++;
                    imprimir_fila();
                }

            } else if (id1 == 1 && index_neo > 0) {   // BOTÃO B → decrementa
                index_neo--;
                npAcendeLED(index_neo, 0, 0, 0);  // apaga o LED

                if (quantidade > 0) {
                    // int valor = fila[inicio]; // O valor não é usado, pode remover
                    inicio = (inicio + 1) % TAM_FILA;
                    quantidade--;
                    imprimir_fila();
                }
            }
            // --- INÍCIO DA MODIFICAÇÃO PARA A TAREFA ---
            else if (id1 == 2) { // BOTÃO JOYSTICK pressionado (BOTAO_JOYSTICK tem índice 2)
                printf("Botão Joystick Pressionado: Resetando estado.\n");

                // 1. Zerar o contador de eventos (tarefas)
                contador = 0;

                // 2. Apagar toda a Matriz de NeoPixel
                npClear(); // Limpa o buffer dos NeoPixels (define todos para 0,0,0)
                npWrite(); // Envia os dados do buffer para a fita NeoPixel

                // 3. Resetar o índice do NeoPixel
                // index_neo é global, definido em Atividade_5.c
                // e extern em funcoes_neopixel.h (incluído)
                index_neo = 0;

                // 4. Resetar os contadores da fila para simular uma fila vazia
                inicio = 0;
                fim = 0;
                quantidade = 0;

                printf("Sistema resetado: Contador=%d, Fila zerada, NeoPixels apagados.\n", contador);
                imprimir_fila(); // Para verificar o estado da fila
            }
            // --- FIM DA MODIFICAÇÃO PARA A TAREFA ---

            // Atualiza LEDs RGB externos
            // Esta lógica já deve refletir o estado correto após o reset do joystick
            gpio_put(LED_VERMELHO, (index_neo == LED_COUNT));     // todo aceso
            gpio_put(LED_AZUL,     (index_neo == 0));             // tudo apagado
            gpio_put(LED_VERDE,    0);                            // opcional

            // Espera botão ser solto
            while (!gpio_get(BOTOES[id1])) {
                tight_loop_contents();
            }
        }
    }
}

// Função para imprimir a fila (não modificado)
void imprimir_fila() {
    printf("Fila [tam=%d, contador_global=%d]: ", quantidade, contador); // Adicionado contador_global para depuração
    int i = inicio;
    for (int c = 0; c < quantidade; c++) {
        printf("%d ", fila[i]);
        i = (i + 1) % TAM_FILA;
    }
    printf("\n");
}
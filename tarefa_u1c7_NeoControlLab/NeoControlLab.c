#include <stdio.h>
#include "pico/stdlib.h"
#include "neopixel_driver.h"
#include "testes_cores.h"
#include "efeitos.h"
#include "efeito_curva_ar.h"
#include "numeros_neopixel.h"
#include <time.h>
#include <stdlib.h>
#include "pico/time.h"
#include "hardware/gpio.h"

// Define o pino do microcontrolador conectado ao botão A.
#define BOTAO_A 5 

// Variável global para comunicação entre a ISR e o loop principal.
// Funciona como uma "bandeira" (flag) que a interrupção levanta.
// A palavra-chave 'volatile' é ESSENCIAL: ela avisa ao compilador que o valor
// desta variável pode mudar a qualquer momento (pela ISR), impedindo que ele
// faça otimizações que poderiam quebrar a lógica do programa.
volatile bool botao_foi_pressionado = false;

// Função de inicialização do sistema. Executada uma única vez.
void setup() {
    // Inicializa a comunicação serial para depuração via printf.
    stdio_init_all();
    
    // Inicializa o driver da fita/matriz de LEDs NeoPixel.
    npInit(LED_PIN); 
    
    // Define a semente para o gerador de números aleatórios.
    srand(time_us_32()); 

    // Configura o pino do botão como entrada digital com resistor de pull-up.
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
}

// Gera e retorna um número inteiro aleatório dentro do intervalo [min, max].
int sorteia_entre(int min, int max) {
    return rand() % (max - min + 1) + min;
}

// Recebe um número e chama a função correspondente para exibi-lo na matriz NeoPixel.
void mostrar_numero_sorteado(int numero) {
    switch (numero) {
        case 1: mostrar_numero_1(); break;
        case 2: mostrar_numero_2(); break;
        case 3: mostrar_numero_3(); break;
        case 4: mostrar_numero_4(); break;
        case 5: mostrar_numero_5(); break;
        case 6: mostrar_numero_6(); break;
    }
}

// Rotina de Serviço de Interrupção (ISR) para o botão.
// Esta função agora é extremamente rápida, seguindo as boas práticas.
void isr_botao(uint gpio, uint32_t events) {
    // Verifica se a interrupção foi realmente gerada pelo BOTAO_A.
    if (gpio == BOTAO_A) {
        // Condição de guarda: só levanta a bandeira se a animação anterior já terminou.
        // Isso evita que o usuário acione a lógica múltiplas vezes com cliques rápidos.
        if (!botao_foi_pressionado) {
            // A única tarefa da ISR é sinalizar ao loop principal que o evento ocorreu.
            botao_foi_pressionado = true;
        }
    } 
}

// Função principal, ponto de entrada do programa.
int main() {
    // Executa as configurações iniciais de hardware e software.
    setup();

    // Configura a interrupção para o pino do botão.
    // A função 'isr_botao' será chamada sempre que ocorrer uma borda de descida.
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &isr_botao);

    // Loop principal do programa. Agora ele é responsável por processar a lógica.
    while (true) {
        // O loop verifica continuamente se a "bandeira" foi levantada pela ISR.
        if (botao_foi_pressionado) {
            // Passo 1: Imediatamente "abaixa a bandeira" (consome o evento).
            // Isso garante que o código dentro deste 'if' execute apenas uma vez por
            // pressionamento de botão.
            botao_foi_pressionado = false;

            // Passo 2: Executa a lógica longa e pesada aqui, no loop principal,
            // de forma segura e sem bloquear outras interrupções do sistema.
            int vezes = sorteia_entre(100, 500);
            printf("\nMostrando %d números aleatórios...\n", vezes);

            int n = sorteia_entre(1, 6);

            for (int i = 0; i < vezes; i++) {
                n = sorteia_entre(1, 6);
                printf("Número sorteado: %d\n", n);
                mostrar_numero_sorteado(n);
                sleep_ms(10);
            }
        }
        // O processador fica livre para executar outras tarefas aqui, se houvesse.
        // Em vez de ficar preso em um 'tight_loop_contents()', o loop agora tem um propósito.
    }
    // Este retorno nunca é alcançado em um sistema embarcado com loop infinito.
    return 0;
}
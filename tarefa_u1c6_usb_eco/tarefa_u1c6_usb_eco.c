/**
 * @file main.c
 * @brief Dispositivo CDC (USB) responsivo com identificação visual para BitDogLab.
 *        Tarefa Unidade 1, Capítulo 6.
 *        Recebe comandos de cor via serial USB, faz eco e acende o LED correspondente.
 */

// Inclusão de bibliotecas padrão e do Pico SDK
#include <stdio.h>      // Para funções de entrada/saída padrão como printf.
#include <string.h>     // Para funções de manipulação de strings como strcmp, strlen, strcspn.
#include "pico/stdlib.h" // Cabeçalho principal do SDK do Pico, inclui tipos básicos e funções utilitárias.
#include "tusb.h"       // Cabeçalho principal da biblioteca TinyUSB para funcionalidade USB.

// --- Definições de Pinos dos LEDs (Conforme BitDogLab e Tarefa) ---
// Define macros para os números dos pinos GPIO conectados aos LEDs.
// Isso torna o código mais legível e fácil de modificar se os pinos mudarem.
#define LED_PIN_VERMELHO 13 // GPIO 13 controla o LED vermelho.
#define LED_PIN_VERDE    11 // GPIO 11 controla o LED verde.
#define LED_PIN_AZUL     12 // GPIO 12 controla o LED azul.

// --- Comandos Esperados ---
// Define ponteiros constantes para strings que representam os comandos válidos.
// Usar ponteiros constantes para strings literais é eficiente.
const char *CMD_VERMELHO = "vermelho";
const char *CMD_VERDE    = "verde";
const char *CMD_AZUL     = "azul";




// ==========================================================================
// --- Protótipos de Função (Assinaturas) ---
// ==========================================================================
// Declarações antecipadas das funções definidas posteriormente no arquivo.
// Isso permite que as funções sejam chamadas antes de suas implementações completas
// serem encontradas pelo compilador, e ajuda na organização do código.

void configurar_hardware_inicial(); // Função para configurar pinos e stdio.
void processar_comando_cdc(char *buffer, uint32_t tamanho_buffer); // Função para tratar dados recebidos via USB CDC.
void controlar_led_por_comando(const char *comando); // Função para acender o LED com base no comando.
void apagar_todos_os_leds(); // Função para desligar todos os LEDs.



// ==========================================================================
// --- Função Principal ---
// ==========================================================================
int main() {
    // Chama a função para configurar stdio (para printf) e os pinos dos LEDs.
    configurar_hardware_inicial();
    
    
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }
    // Informa via terminal serial que a conexão foi detectada
    printf("USB conectado!\n");

    // Loop principal infinito da aplicação.
    while (true) {
        // tud_task() é a função principal do TinyUSB. Ela DEVE ser chamada
        // repetidamente dentro do loop principal. Ela processa eventos USB,
        // como recebimento de dados, envio de dados, enumeração, etc.
        tud_task();

        // Verifica duas condições:
        // 1. tud_cdc_connected(): Se a interface CDC USB está conectada e configurada pelo host (PC).
        // 2. tud_cdc_available(): Se há dados enviados pelo host e disponíveis para leitura na interface CDC.
        if (tud_cdc_connected() && tud_cdc_available()) {
            // Declara um buffer (array de caracteres) para armazenar os dados recebidos.
            // Tamanho 64 é arbitrário, mas suficiente para comandos curtos.
            char rx_buffer[64];
            // Lê os dados da FIFO de recepção do CDC para rx_buffer.
            // sizeof(rx_buffer) - 1 garante que haverá espaço para um terminador nulo '\0'.
            // 'count' armazena o número de bytes realmente lidos.
            uint32_t count = tud_cdc_read(rx_buffer, sizeof(rx_buffer) - 1);

            // Se algum dado foi lido (count > 0).
            if (count > 0) {
                // Chama a função para processar o comando recebido.
                processar_comando_cdc(rx_buffer, count);
            }
        }
    }
    // Esta linha nunca é alcançada em um programa embarcado típico com loop infinito.
    return 0;
}

// ==========================================================================
// --- Implementações das Funções Auxiliares ---
// ==========================================================================

/**
 * Configura os pinos GPIO dos LEDs como saídas e inicializa a comunicação serial (stdio).
 */
void configurar_hardware_inicial() {
    // Inicializa todas as interfaces stdio padrão (USB, UART se configurado).
    // É importante para printf e para o funcionamento correto do TinyUSB CDC.
    stdio_init_all();

    // Configura o pino do LED Vermelho.
    gpio_init(LED_PIN_VERMELHO);              // Inicializa o pino GPIO.
    gpio_set_dir(LED_PIN_VERMELHO, GPIO_OUT); // Define o pino como saída.

    // Configura o pino do LED Verde.
    gpio_init(LED_PIN_VERDE);
    gpio_set_dir(LED_PIN_VERDE, GPIO_OUT);

    // Configura o pino do LED Azul.
    gpio_init(LED_PIN_AZUL);
    gpio_set_dir(LED_PIN_AZUL, GPIO_OUT);

    // Garante que todos os LEDs comecem desligados.
    apagar_todos_os_leds();
    printf("Hardware (LEDs e stdio) configurado.\n");
}

/**
 * Desliga todos os LEDs do RGB (coloca os pinos em nível baixo).
 */
void apagar_todos_os_leds() {
    gpio_put(LED_PIN_VERMELHO, 0); // 0 para desligar.
    gpio_put(LED_PIN_VERDE, 0);
    gpio_put(LED_PIN_AZUL, 0);
}

/**
 * Acende um LED específico por 1 segundo com base no comando de cor recebido.
 * @param comando String contendo o comando de cor ("vermelho", "verde", "azul").
 */
void controlar_led_por_comando(const char *comando) {
    apagar_todos_os_leds(); // Primeiro, apaga qualquer LED que estivesse aceso.

    // Compara a string 'comando' com os comandos predefinidos.
    // strcmp retorna 0 se as strings forem iguais.
    if (strcmp(comando, CMD_VERMELHO) == 0) {
        printf("-> Acionando LED Vermelho\n");
        gpio_put(LED_PIN_VERMELHO, 1); // 1 para acender.
        sleep_ms(1000);                // Mantém o LED aceso por 1000 ms (1 segundo).
        gpio_put(LED_PIN_VERMELHO, 0); // Apaga o LED.
    } else if (strcmp(comando, CMD_VERDE) == 0) {
        printf("-> Acionando LED Verde\n");
        gpio_put(LED_PIN_VERDE, 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_VERDE, 0);
    } else if (strcmp(comando, CMD_AZUL) == 0) {
        printf("-> Acionando LED Azul\n");
        gpio_put(LED_PIN_AZUL, 1);
        sleep_ms(1000);
        gpio_put(LED_PIN_AZUL, 0);
    } else {
        // Se o comando não for reconhecido.
        printf("-> Comando de cor desconhecido: '%s'\n", comando);
    }
    // Após 1 segundo, o LED específico já foi apagado.
    // Não é estritamente necessário apagar todos novamente aqui, mas não prejudica.
}

/**
 * Processa os dados recebidos do host (PC) via interface CDC USB.
 * Limpa a string, identifica o comando, controla o LED e envia um eco de volta.
 * @param buffer Ponteiro para o buffer com os dados crus recebidos.
 * @param tamanho_buffer Número de bytes que foram lidos para o buffer.
 */
void processar_comando_cdc(char *buffer, uint32_t tamanho_buffer) {
    // Adiciona um terminador nulo ao final dos dados recebidos.
    // Isso transforma o buffer de bytes crus em uma string C válida,
    // permitindo o uso de funções de string como strcmp e strlen.
    buffer[tamanho_buffer] = '\0';

    // Remove caracteres de nova linha (\n) ou retorno de carro (\r) do final da string.
    // Terminais seriais frequentemente enviam esses caracteres quando você pressiona Enter.
    // strcspn retorna o comprimento da porção inicial de 'buffer' que NÃO contém
    // nenhum dos caracteres em "\r\n". Ao colocar '\0' nessa posição, cortamos a string ali.
    buffer[strcspn(buffer, "\r\n")] = 0;

    // Imprime o comando "limpo" que foi recebido.
    printf("Host enviou: '%s'\n", buffer);

    // Chama a função para acender o LED correspondente ao comando.
    controlar_led_por_comando(buffer);

    // Envia o eco de volta para o host.
    tud_cdc_write_str("Eco: "); // Envia o prefixo "Eco: ".
    // Envia o conteúdo do buffer (o comando limpo). strlen(buffer) calcula o tamanho da string limpa.
    tud_cdc_write(buffer, strlen(buffer));
    tud_cdc_write_str("\n"); // Envia uma nova linha para formatar a saída no terminal do host.
    // tud_cdc_write_flush() garante que os dados no buffer de escrita do TinyUSB sejam enviados
    // imediatamente para o host, em vez de esperar o buffer encher.
    tud_cdc_write_flush();
}




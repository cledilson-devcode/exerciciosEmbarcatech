/**
 * 
 *        Dispositivo CDC (USB) responsivo com identificação visual para BitDogLab.
 *        Tarefa Unidade 1, Capítulo 6.
 *        Recebe comandos de cor via serial USB, faz eco e acende o LED correspondente.
 */

// Inclusão de bibliotecas padrão e do Pico SDK
#include <stdio.h>      
#include <string.h>     
#include "pico/stdlib.h" 
#include "tusb.h"    

// --- Definições de Pinos dos LEDs 
#define LED_PIN_VERMELHO 13 // GPIO 13 controla o LED vermelho.
#define LED_PIN_VERDE    11 // GPIO 11 controla o LED verde.
#define LED_PIN_AZUL     12 // GPIO 12 controla o LED azul.


// Define ponteiros constantes para strings que representam os comandos válidos.
const char *CMD_VERMELHO = "vermelho";
const char *CMD_VERDE    = "verde";
const char *CMD_AZUL     = "azul";




// ==========================================================================
// --- Protótipos de Função (Assinaturas) ---
// ==========================================================================
void configurar_hardware_inicial(); // Função para configurar pinos e stdio.
void processar_comando_cdc(char *buffer, uint32_t tamanho_buffer); // Função para tratar dados recebidos via USB CDC.
void controlar_led_por_comando(char *comando); // Função para acender o LED com base no comando.
void apagar_todos_os_leds(); // Função para desligar todos os LEDs.




int main() {
    
    configurar_hardware_inicial();
    
    
    while (!tud_cdc_connected()) {
        sleep_ms(100);
    }
    // Informa via terminal serial que a conexão foi detectada
    printf("USB conectado!\n");

   
    while (true) {
       
        tud_task();

        // Verifica duas condições:
        // 1. tud_cdc_connected(): Se a interface CDC USB está conectada e configurada pelo host (PC).
        // 2. tud_cdc_available(): Se há dados enviados pelo host e disponíveis para leitura na interface CDC.
        if (tud_cdc_connected() && tud_cdc_available()) {
            // Declara um buffer (array de caracteres) para armazenar os dados recebidos.
            char rx_buffer[64];

            // Lê os dados da FIFO de recepção do CDC para rx_buffer.
            // sizeof(rx_buffer) - 1 garante que haverá espaço para um terminador nulo '\0'.
            // 'count' armazena o número de bytes realmente lidos.
            uint32_t count = tud_cdc_read(rx_buffer, sizeof(rx_buffer) - 1);

            // Se algum dado foi lido (count > 0).
            if (count > 0) {
                processar_comando_cdc(rx_buffer, count);
            }
        }
    }
    
    return 0;
}



void configurar_hardware_inicial() {
   
    stdio_init_all();

   
    gpio_init(LED_PIN_VERMELHO);             
    gpio_set_dir(LED_PIN_VERMELHO, GPIO_OUT); 

    
    gpio_init(LED_PIN_VERDE);
    gpio_set_dir(LED_PIN_VERDE, GPIO_OUT);

    
    gpio_init(LED_PIN_AZUL);
    gpio_set_dir(LED_PIN_AZUL, GPIO_OUT);

    // Garante que todos os LEDs comecem desligados.
    apagar_todos_os_leds();
}


void apagar_todos_os_leds() {
    gpio_put(LED_PIN_VERMELHO, 0); 
    gpio_put(LED_PIN_VERDE, 0);
    gpio_put(LED_PIN_AZUL, 0);
}

/**
 * Acende um LED específico por 1 segundo com base no comando de cor recebido.
 */
void controlar_led_por_comando(char *comando) {
    apagar_todos_os_leds(); 

    // Compara a string 'comando' com os comandos predefinidos.
    // strcmp retorna 0 se as strings forem iguais.
    if (strcmp(comando, CMD_VERMELHO) == 0) {
        printf("-> Acionando LED Vermelho\n");
        gpio_put(LED_PIN_VERMELHO, 1); // 1 para acender.
        sleep_ms(2000);                // Mantém o LED aceso por 1000 ms (1 segundo).
        gpio_put(LED_PIN_VERMELHO, 0); // Apaga o LED.
    } else if (strcmp(comando, CMD_VERDE) == 0) {
        printf("-> Acionando LED Verde\n");
        gpio_put(LED_PIN_VERDE, 1);
        sleep_ms(2000);
        gpio_put(LED_PIN_VERDE, 0);
    } else if (strcmp(comando, CMD_AZUL) == 0) {
        printf("-> Acionando LED Azul\n");
        gpio_put(LED_PIN_AZUL, 1);
        sleep_ms(2000);
        gpio_put(LED_PIN_AZUL, 0);
    } else {
        // Se o comando não for reconhecido.
        printf("-> Comando de cor desconhecido: '%s'\n", comando);
    }
    
}

/**
 * Processa os dados recebidos do host (PC) via interface CDC USB.
 * Limpa a string, identifica o comando, controla o LED e envia um eco de volta.
 * Ponteiro para o buffer com os dados crus recebidos.
 * Número de bytes que foram lidos para o buffer.
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




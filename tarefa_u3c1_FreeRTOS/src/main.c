//
// main.c
//
#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h" // Reintroduzido para usar filas
#include "fsm_semaforo.h"
#include "display_contador.h" // Reintroduzido para a tarefa do display

// Fila para comunicação entre a FSM do semáforo e o display
QueueHandle_t fila_oled;

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) sleep_ms(100);

    // Cria a fila para passar o tempo de contagem (2 itens de tamanho 'int')
    fila_oled = xQueueCreate(2, sizeof(int));

    // Tarefa do semáforo no núcleo 0 (para tempo real preciso)
    TaskHandle_t hFSM = NULL;
    xTaskCreate(tarefa_fsm_semaforo, "FSM_Semaforo", 512, NULL, 1, &hFSM);
    vTaskCoreAffinitySet(hFSM, (1 << 0));

    // Tarefa do display no núcleo 1 (para operações I2C que podem bloquear)
    TaskHandle_t hOLED = NULL;
    xTaskCreate(tarefa_display_contador, "OLED_Display", 1024, NULL, 1, &hOLED);
    vTaskCoreAffinitySet(hOLED, (1 << 1));

    // Início do escalonador
    vTaskStartScheduler();

    // O código nunca deve chegar aqui.
    while (1);
}
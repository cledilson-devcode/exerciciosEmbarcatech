#include "pico/stdlib.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "fsm_semaforo.h"
#include "display_contador.h"

QueueHandle_t fila_pedestre;
QueueHandle_t fila_oled;

int main() {
    stdio_init_all();
    while (!stdio_usb_connected()) sleep_ms(100);

    // Criação das filas
    fila_pedestre = xQueueCreate(4, sizeof(int));
    fila_oled = xQueueCreate(2, sizeof(int));

    // Tarefa do semáforo no núcleo 0
    TaskHandle_t hFSM = NULL;
    xTaskCreate(tarefa_fsm_semaforo, "FSM", 1024, NULL, 2, &hFSM);
    vTaskCoreAffinitySet(hFSM, (1 << 0));

    // Tarefa do display no núcleo 1
    TaskHandle_t hOLED = NULL;
    xTaskCreate(tarefa_display_contador, "OLED", 1024, NULL, 2, &hOLED);
    vTaskCoreAffinitySet(hOLED, (1 << 1));

    // Início do escalonador
    vTaskStartScheduler();
    while (1);
}

//---------------------------------------------------------------------------------------------//
// tarefas.h
//---------------------------------------------------------------------------------------------//
#ifndef TAREFAS_H
#define TAREFAS_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"

// --- ALTERAÇÃO: Definição da estrutura de dados para a fila ---
// Esta struct será usada para comunicar eventos do joystick e do botão.
typedef enum {
    JOYSTICK_DATA,
    BUTTON_CLICK
} event_type_t;

typedef struct {
    event_type_t type;
    uint16_t x;
    uint16_t y;
} queue_msg_t;

// --- ALTERAÇÃO: Handles para os novos recursos e tarefas ---
// Tornamos os handles globais para que possam ser acessados em main.c e tarefas.c
extern QueueHandle_t xQueueEventos;
extern SemaphoreHandle_t xMutexPrintf;
extern SemaphoreHandle_t xCountingSemBuzzer;

// --- ALTERAÇÃO: Protótipos das novas tarefas ---
void task_leitura_eixos(void *params);
void task_leitura_botao(void *params);
void task_processamento(void *params);
void task_controle_buzzer(void *params);

#endif
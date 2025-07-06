#ifndef TAREFAS_RGB_SMP_H
#define TAREFAS_RGB_SMP_H

#include "FreeRTOS.h"  // Corrige o erro de tipo desconhecido
#include "task.h"      // Define TaskHandle_t

// Prototipagem das tarefas FreeRTOS
void task_mensagens(void *params);
void task_autoteste_geral(void *params);
void task_botao_a(void *params);
void task_botao_b(void *params);
void task_botao_joystick(void *params);

// Declaração externa dos handles (visíveis em outros arquivos .c)
extern TaskHandle_t handle_botao_a;
extern TaskHandle_t handle_botao_b;
extern TaskHandle_t handle_botao_joystick;

#endif

#ifndef TAREFAS_H
#define TAREFAS_H

#include "FreeRTOS.h"
#include "task.h"

// Handle para a tarefa de autoteste
extern TaskHandle_t handle_self_test;

// Protótipo da tarefa
void task_self_test(void *params);

#endif
#ifndef TAREFAS_H
#define TAREFAS_H

#include "FreeRTOS.h"
#include "task.h"

// Handle para a tarefa de autoteste
extern TaskHandle_t handle_self_test;
extern TaskHandle_t handle_alive_task; 
extern TaskHandle_t handle_monitor_task;


// Protótipo da tarefa
void task_self_test(void *params);
void task_alive(void *params);
void task_monitor_joystick(void *params);


#endif
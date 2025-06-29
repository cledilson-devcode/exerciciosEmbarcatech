//
// fsm_semaforo.h
//
#ifndef FSM_SEMAFORO_H
#define FSM_SEMAFORO_H

// Pinos para o semáforo de carro
#define LED_VERDE_GPIO    11
#define LED_AZUL_GPIO     12
#define LED_VERMELHO_GPIO 13

// A única função que este módulo precisa declarar é a sua própria tarefa.
void tarefa_fsm_semaforo(void *p);

#endif
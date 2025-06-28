//
// fsm_semaforo.h
//
#ifndef FSM_SEMAFORO_H
#define FSM_SEMAFORO_H

// --- Pinos redefinidos para a tarefa do semáforo de carro ---
// Conforme solicitado: Verde=11, Azul=12 (para inicialização), Vermelho=13
#define LED_VERDE_GPIO    11
#define LED_AZUL_GPIO     12
#define LED_VERMELHO_GPIO 13

// A função da tarefa principal permanece a mesma
void tarefa_fsm_semaforo(void *p);

#endif
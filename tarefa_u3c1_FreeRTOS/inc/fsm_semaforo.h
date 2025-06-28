#ifndef FSM_SEMAFORO_H
#define FSM_SEMAFORO_H

// GPIOs definidos
#define AV_A_R 13
#define AV_A_G 11
#define AV_A_B 12
#define AV_B_R 16
#define AV_B_G 17
#define AV_B_B 28
#define PED_A_R 8
#define PED_A_G 9
#define PED_A_B 4
#define PED_B_R 20
#define PED_B_G 19
#define PED_B_B 18
#define BTN_PEDESTRE 5
#define BTN_B 6

void tarefa_fsm_semaforo(void *p);

#endif

#ifndef BOTOES_H
#define BOTOES_H

// ============================================================================
// Mapeamento dos botões físicos no kit BitDogLab
// Cada botão está ligado a um pino digital com pull-up interno
// ============================================================================
#define BTN_A   5
#define BTN_B   6
#define BTN_JOY 22

// Prototipagem da função de inicialização dos botões
void init_button(int gpio);

#endif

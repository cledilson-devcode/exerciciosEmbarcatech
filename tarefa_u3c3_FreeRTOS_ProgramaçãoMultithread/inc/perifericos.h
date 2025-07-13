#ifndef PERIFERICOS_H
#define PERIFERICOS_H

void init_perifericos();
void test_leds_rgb();
// void test_buzzer();
void test_botoes();
void test_joystick_analog();
void test_microfone();

// Funções para o buzzer com PWM
void buzzer_pwm_init();
void buzzer_set_alarm(bool on);
void test_buzzer_pwm();

#endif
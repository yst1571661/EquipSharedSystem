#ifndef _GPIO_EX_H
#define _GPIO_EX_H

#ifdef __cplusplus
extern "C" {
#endif

int init_gpio_e();
void close_gpio_e();
int ReadVol();
int SetLed(int val);
int TurnLedOn();
int TurnLedOff();

#ifdef __cplusplus
}
#endif
#endif

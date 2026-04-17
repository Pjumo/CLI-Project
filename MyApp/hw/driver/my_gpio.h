#ifndef _HW_DRIVER_MY_GPIO_H_
#define _HW_DRIVER_MY_GPIO_H_

#include "hw_def.h"

// port number : 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G, 7=H
bool gpioExtWrite(uint8_t port, uint8_t pin, uint8_t state);
int8_t gpioExtRead(uint8_t port, uint8_t pin);

#endif
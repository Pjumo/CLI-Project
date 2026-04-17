#ifndef __HW_DRIVER_LED_H_
#define __HW_DRIVER_LED_H_

#include "hw_def.h"

void ledInit(void);
void ledOn(void);
void ledOff(void);
void ledToggle(void);
bool getLedState(void);

#endif
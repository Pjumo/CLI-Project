#ifndef _HW_DRIVER_TEMP_H_
#define _HW_DRIVER_TEMP_H_

#include "hw_def.h"

bool tempInit(void);
float tempReadAuto(void);
float tempReadSingle(void);
void tempStartAuto(void);
void tempStopAuto(void);

#endif
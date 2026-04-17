#ifndef _HW_DRIVER_LOG_H_
#define _HW_DRIVER_LOG_H_

#include "hw_def.h"
#include "log_def.h"

void cliLog(uint8_t argc, char *argv[]);
bool logInit(void);
void logSetLevel(uint8_t level);
uint8_t logGetLevel(void);

#endif
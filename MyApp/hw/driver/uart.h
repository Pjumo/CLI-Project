#ifndef __HW_DRIVER_UART_H_
#define __HW_DRIVER_UART_H_

#include "hw_def.h"

bool uartInit(void);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
bool uartOpen(uint8_t ch, uint32_t baud);
bool uartClose(uint8_t ch);
uint32_t uartWrite(uint8_t ch, uint8_t *p_data, uint32_t length);
uint32_t uartAvailable(uint8_t ch);
uint8_t uartRead(uint8_t ch);
uint32_t uartPrintf(uint8_t ch, char *fmt, ...);

bool uartReadBlock(uint8_t ch, uint8_t *p_data, uint32_t timeout);
bool uartWriteBlock(uint8_t ch, uint8_t *p_data, uint32_t length);

#endif
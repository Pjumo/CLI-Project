#include "hw.h"

void hwInit(void)
{
    ledInit();
    uartInit();
    cliInit();
    buttonInit();
    tempInit();
    logInit();
}
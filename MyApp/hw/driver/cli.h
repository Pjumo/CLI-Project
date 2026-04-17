#ifndef __HW_DRIVER_CLI_H_
#define __HW_DRIVER_CLI_H_

#include "hw_def.h"
#include "bsp.h"

void cliInit(void);
void cliRunCommand(void);
void cliMain(void);
void cliPrintf(const char *fmt, ...);
void cliParseArgs(char *line_buf);
bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char *argv[]));

typedef void (*cli_callback_t)(void);
void cliSetCtrlCHandler(cli_callback_t handler);

#endif
#include "cli.h"

#define CLI_LINE_BUF_MAX 64
#define CLI_CMD_LIST_MAX 32
#define CLI_CMD_ARG_MAX 4

typedef struct {
    char cmd_str[16];
    void (*cmd_func)(uint8_t argc, char *argv[]);
} cli_cmd_t;

static cli_cmd_t cli_cmd_list[CLI_CMD_LIST_MAX];
static uint8_t cli_cmd_cnt = 0;

static uint8_t cli_argc = 0;
static char *cli_argv[CLI_CMD_ARG_MAX];
static char cli_line_buf[CLI_LINE_BUF_MAX];
static uint16_t cli_line_buf_idx = 0;

#define CLI_HISTORY_BUF_MAX 10
static char cli_history_buf[CLI_HISTORY_BUF_MAX][CLI_LINE_BUF_MAX];
static uint8_t cli_history_cnt = 0;
static uint8_t cli_history_idx = 0;
static uint8_t cli_history_depth = 0;

typedef enum { CLI_STATE_NORMAL = 0, CLI_STATE_ESC_RCVD, CLI_STATE_BRAKCET_RCVD } cli_input_state_t;

static cli_input_state_t cli_input_state = CLI_STATE_NORMAL;

static void handleEnterKey(void)
{
    cliPrintf("\r\n");
    cli_line_buf[cli_line_buf_idx] = '\0';

    strncpy(cli_history_buf[cli_history_cnt], cli_line_buf, CLI_LINE_BUF_MAX);
    cli_history_cnt = (cli_history_cnt + 1) % CLI_HISTORY_BUF_MAX;
    cli_history_idx = cli_history_cnt;
    cli_history_depth =
        cli_history_depth < CLI_HISTORY_BUF_MAX ? cli_history_depth + 1 : CLI_HISTORY_BUF_MAX;

    cliParseArgs(cli_line_buf);
    cliRunCommand();

    cli_line_buf_idx = 0;
    cliPrintf("CLI> ");
}

static void handleBackspace(void)
{
    if (cli_line_buf_idx > 0) {
        cli_line_buf_idx--;
        cliPrintf("\b \b");
    }
}

static void handleCharInsert(uint8_t c)
{
    cliPrintf("%c", c);
    cli_line_buf[cli_line_buf_idx++] = c;
    if (cli_line_buf_idx >= CLI_LINE_BUF_MAX) {
        cli_line_buf_idx = 0;
    }
}

static void handleArrowKeys(uint8_t rx_data)
{
    if (rx_data == 'A') {
        for (uint16_t i = 0; i < cli_line_buf_idx; i++) {
            cliPrintf("\b \b");
        }

        if (cli_history_idx == 0 && cli_history_depth == CLI_HISTORY_BUF_MAX) {
            cli_history_idx = CLI_HISTORY_BUF_MAX - 1;
        } else if (cli_history_idx != 0) {
            if (cli_history_idx != cli_history_cnt + 1) {
                cli_history_idx--;
            }
        }
        strncpy(cli_line_buf, cli_history_buf[cli_history_idx], CLI_LINE_BUF_MAX);

        cli_line_buf_idx = strlen(cli_line_buf);
        cliPrintf("%s", cli_line_buf);
    } else if (rx_data == 'B' && cli_history_idx != cli_history_cnt) {
        for (uint16_t i = 0; i < cli_line_buf_idx; i++) {
            cliPrintf("\b \b");
        }

        if (cli_history_idx == CLI_HISTORY_BUF_MAX - 1 &&
            cli_history_depth == CLI_HISTORY_BUF_MAX) {
            cli_history_idx = 0;
        } else if (cli_history_idx != CLI_HISTORY_BUF_MAX - 1) {
            if (cli_history_idx != cli_history_cnt - 1) {
                cli_history_idx++;
            }
        }
        strncpy(cli_line_buf, cli_history_buf[cli_history_idx], CLI_LINE_BUF_MAX);

        cli_line_buf_idx = strlen(cli_line_buf);
        cliPrintf("%s", cli_line_buf);
    }
}

static void processAnsiEscape(uint8_t rx_data)
{
    switch (rx_data) {
    case 0x1B:
        cli_input_state = CLI_STATE_ESC_RCVD;
        break;
    case '[':
        if (cli_input_state == CLI_STATE_ESC_RCVD) {
            cli_input_state = CLI_STATE_BRAKCET_RCVD;
        } else {
            cli_input_state = CLI_STATE_NORMAL;
        }
        break;
    case 'A':
    case 'B':
        if (cli_input_state == CLI_STATE_BRAKCET_RCVD) {
            handleArrowKeys(rx_data);
        }
        cli_input_state = CLI_STATE_NORMAL;
        break;
    default:
        cli_input_state = CLI_STATE_NORMAL;
        break;
    }
}

static bool isSafeAddress(uint32_t addr)
{
    // 1. f411 flash memory
    if (0x08000000 <= addr && addr <= 0x0807FFFF)
        return true;

    // 2. f411 sram
    if (0x20000000 <= addr && addr <= 0x2001FFFF)
        return true;

    // 3. system memory
    if (0x1FFF0000 <= addr && addr <= 0x1FFF7A1F)
        return true;

    // 4. peripheral
    if (0x40000000 <= addr && addr <= 0x5FFFFFFF)
        return true;

    return false;
}

static void cliMemoryDump(uint8_t argc, char *argv[])
{
    if (argc >= 2) {
        uint32_t addr = strtoul(argv[1], NULL, 16);
        if (!isSafeAddress(addr)) {
            cliPrintf("Invalid Address\r\n");
            return;
        }
        uint32_t length = 16;

        if (argc >= 3) {
            length = strtoul(argv[2], NULL, 10);
        }

        cliPrintf("Address: 0x%08X\r\n", addr);
        for (uint32_t i = 0; i < length; i++) {
            if (i % 16 == 0) {
                cliPrintf("\r\n0x%08X: ", addr + i);
            }
            cliPrintf("%02X ", *((uint8_t *)(addr + i)));
        }
        cliPrintf("  |  ");
        for (uint32_t i = 0; i < length; i++) {
            uint8_t val = *((volatile uint8_t *)(addr + i));
            cliPrintf("%c ", val > 32 && val < 127 ? val : '.');
        }
        cliPrintf("\r\n");
    } else {
        cliPrintf("Usage: md [addr(hex)]\r\n");
        cliPrintf("       md [addr(hex)] [length]\r\n");
    }
}

static void cliGpio(uint8_t argc, char *argv[])
{
    if (argc >= 3) {
        char port_char = tolower(argv[2][0]);
        uint8_t pin = atoi(&argv[2][1]);
        uint8_t port = port_char - 'a';

        if (strcmp(argv[1], "read") == 0) {
            int8_t state = gpioExtRead(port, pin);
            if (state < 0) {
                cliPrintf("Invalid Port or Pin (ex: a5, B12)\r\n");
            } else {
                cliPrintf("GPIO %c%d: %d\r\n", toupper(port_char), pin, state);
            }
        } else if (strcmp(argv[1], "write") == 0 && argc == 4) {
            int val = atoi(argv[3]);
            if (gpioExtWrite(port, pin, val) == true) {
                cliPrintf("GPIO %c%d set to %d\r\n", toupper(port_char), pin, val);
            } else {
                cliPrintf("Invalid Port or Pin (ex: a5, B12)\r\n");
            }
        } else {
            cliPrintf("Usage: gpio read [a-h][0-15]\r\n");
            cliPrintf("       gpio write [a-h][0-15] [0/1]\r\n");
        }
    } else {
        cliPrintf("Usage: gpio read [a-h][0-15]\r\n");
        cliPrintf("       gpio write [a-h][0-15] [0/1]\r\n");
    }
}

uint32_t temp_period = 0;

static void cliTemp(uint8_t argc, char **argv) {
  if (argc == 1) 
  {
   
    if(temp_period >0){
      tempStopAuto();
    }
    temp_period = 0;
    float t=tempReadSingle();
    cliPrintf("Current Temp: %.2f *C\r\n", t);
  } 
  else if (argc == 2) 
  {
    
    int period = atoi(argv[1]);
    if (period > 0) {
      tempStartAuto();
      temp_period = period;
      cliPrintf("Temperature Auto-Report Started (%d ms)\r\n", period);
    } else {
      tempStopAuto();
      cliPrintf("Invalid Period\r\n");
    }
  } else {
    tempStopAuto();
    cliPrintf("Usage: temp\r\n");
    cliPrintf("     : temp [period]\r\n");
  }
}

static void cliBtn(uint8_t argc, char *argv[])
{
    if (argc == 2) {
        if (strcmp(argv[1], "enable") == 0) {
            buttonEnable(true);
            cliPrintf("Button Enabled\r\n");
        } else if (strcmp(argv[1], "disable") == 0) {
            buttonEnable(false);
            cliPrintf("Button Disabled\r\n");
        } else {
            cliPrintf("Invalid Command\r\n");
        }
    } else {
        cliPrintf("Usage: btn [enable/disable]\r\n");
    }
}

uint32_t led_toggle_period = 0;

static void cliLed(uint8_t argc, char *argv[])
{
    if (argc >= 2) {
        if (strcmp(argv[1], "on") == 0) {
            led_toggle_period = 0;
            ledOn();
            cliPrintf("LED ON\r\n");
        } else if (strcmp(argv[1], "off") == 0) {
            led_toggle_period = 0;
            ledOff();
            cliPrintf("LED OFF\r\n");
        } else if (strcmp(argv[1], "toggle") == 0) {
            if (argc == 3) {
                led_toggle_period = atoi(argv[2]);
                cliPrintf("LED Toggle Period Set to %d ms\r\n", led_toggle_period);
            } else {
                led_toggle_period = 1000;
                cliPrintf("LED Toggle Period Set to Default (1000 ms)\r\n");
            }
        } else {
            cliPrintf("Invalid Command\r\n");
        }
    } else {
        cliPrintf("Usage: led [on/off]\r\n");
        cliPrintf("       led toggle [period_ms]\r\n");
    }
}

static void cliInfo(uint8_t argc, char *argv[])
{
    if (argc == 1) {
        cliPrintf("=====================================\r\n");
        cliPrintf("  HW Model    : STM32F411\r\n");
        cliPrintf("  FW Version  : V1.0.0\r\n");
        cliPrintf("  Build Date  : %s %s\r\n", __DATE__, __TIME__);

        uint32_t hal = HAL_GetHalVersion();
        uint32_t rev = HAL_GetREVID();
        uint32_t dev = HAL_GetDEVID();
        uint32_t uid0 = HAL_GetUIDw0();
        uint32_t uid1 = HAL_GetUIDw1();
        uint32_t uid2 = HAL_GetUIDw2();

        cliPrintf("  HAL Version : %d.%d.%d\r\n", (hal >> 24) & 0xFF, (hal >> 16) & 0xFF,
                  hal & 0xFFFF);
        cliPrintf("  Device ID   : %08X\r\n", dev);
        cliPrintf("  Revision ID : %08X\r\n", rev);
        cliPrintf("  Serial Num  : %08X-%08X-%08X\r\n", uid0, uid1, uid2);
        cliPrintf("=====================================\r\n");
    } else if (argc == 2 && strcmp(argv[1], "uptime") == 0) {
        cliPrintf("System Uptime: %d ms \r\n", millis());
    } else {
        cliPrintf("Usage: info [uptime]\r\n");
    }
}

static void cliSys(uint8_t argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "reset") == 0) {
        cliPrintf("System Resetting...\r\n");
        NVIC_SystemReset();
    } else {
        cliPrintf("Usage: sys [reset]\r\n");
    }
}

static void cliHelp(uint8_t argc, char *argv[])
{
    cliPrintf("----------CLI Commands----------\r\n");

    for (uint8_t i = 0; i < cli_cmd_cnt; i++) {
        cliPrintf("%s\r\n", cli_cmd_list[i].cmd_str);
    }
    cliPrintf("--------------------------------\r\n");
}

static void cliClear(uint8_t argc, char *argv[])
{
    cliPrintf("\x1B[2J\x1B[H");
}

void cliInit(void)
{
    cli_cmd_cnt = 0;
    cli_argc = 0;
    cli_line_buf_idx = 0;

    cliClear(0, NULL);

    LOG_INF("CLI Init");

    cliAdd("help", cliHelp);
    cliAdd("cls", cliClear);
    cliAdd("led", cliLed);
    cliAdd("info", cliInfo);
    cliAdd("sys", cliSys);
    cliAdd("gpio", cliGpio);
    cliAdd("md", cliMemoryDump);
    cliAdd("btn", cliBtn);
    cliAdd("temp", cliTemp);
    cliAdd("log", cliLog);
}

void cliRunCommand(void)
{
    if (cli_argc == 0) {
        return;
    }

    bool is_found = false;
    for (uint8_t i = 0; i < cli_cmd_cnt; i++) {
        if (strcmp(cli_argv[0], cli_cmd_list[i].cmd_str) == 0) {
            cli_cmd_list[i].cmd_func(cli_argc, cli_argv);
            is_found = true;
            break;
        }
    }

    if (!is_found) {
        cliPrintf("Command Not Found\r\n");
    }
}

void cliPrintf(const char *fmt, ...)
{
    char buf[CLI_LINE_BUF_MAX];
    uint32_t len;
    va_list args;

    va_start(args, fmt);

    len = vsnprintf(buf, sizeof(buf), fmt, args);

    va_end(args);
    uartWrite(0, (uint8_t *)buf, len);
}

void cliParseArgs(char *line_buf)
{
    char *token;
    cli_argc = 0;
    token = strtok(line_buf, " ");
    while (token != NULL && cli_argc < CLI_CMD_ARG_MAX) {
        cli_argv[cli_argc++] = token;
        token = strtok(NULL, " ");
    }
}

bool cliAdd(const char *cmd_str, void (*cmd_func)(uint8_t argc, char *argv[]))
{
    if (cli_cmd_cnt >= CLI_CMD_LIST_MAX) {
        return false;
    }

    strncpy(cli_cmd_list[cli_cmd_cnt].cmd_str, cmd_str, strlen(cmd_str) + 1);
    cli_cmd_list[cli_cmd_cnt].cmd_func = cmd_func;
    cli_cmd_cnt++;

    return true;
}

static cli_callback_t ctrl_c_handler = NULL;

void cliSetCtrlCHandler(cli_callback_t handler)
{
    ctrl_c_handler = handler;
}

void cliMain(void)
{
    uint8_t rx_data;
    if (uartReadBlock(0, &rx_data, osWaitForever) == false) {
        return;
    }

    if (cli_input_state != CLI_STATE_NORMAL) {
        processAnsiEscape(rx_data);
        return;
    }

    switch (rx_data) {
    case 0x03:
        ctrl_c_handler();
        cliPrintf("^c \r\nCLI>");
        cli_line_buf_idx=0;
        break;
    case 0x1B:
        cli_input_state = CLI_STATE_ESC_RCVD;
        break;
    case '\r':
    case '\n':
        handleEnterKey();
        break;
    case '\b':
    case 127:
        handleBackspace();
        break;
    default:
        if (32 <= rx_data && rx_data <= 126)
            handleCharInsert(rx_data);
        break;
    }
}
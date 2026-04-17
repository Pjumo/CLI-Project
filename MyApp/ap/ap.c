#include "ap.h"
#include "monitor.h"

extern uint32_t led_toggle_period;
extern uint32_t temp_period;


void ledSystemTask(void *arg)
{
    while (1) {
        if (led_toggle_period > 0) {
            ledToggle();

            bool led_state = getLedState();
            monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);

            osDelay(led_toggle_period);
        } else {
            if(isMonitoringOn()){
                bool led_state = getLedState();
                monitorUpdateValue(ID_OUT_LED_STATE, TYPE_BOOL, &led_state);
            }
            osDelay(50);
        }
    }
}

void tempSystemTask(void *arg)
{
    while (1) {
        if (temp_period > 0) {
            float temp = tempReadAuto();

            if(isMonitoringOn()){
                monitorUpdateValue(ID_ENV_TEMP, TYPE_FLOAT, &temp);
            } else {
                cliPrintf("Temperature: %.2f °C\r\n", temp);
            }

            osDelay(temp_period);
        } else {
            osDelay(50);
        }
    }
}

void monitorSystemTask(void *argument){
    while (1){
        if (isMonitoringOn()){
            monitorSendPacket();
        }
        osDelay(monitorGetPeriod());
    }
}

void StartDefaultTask(void *argument)
{
    apInit();
    cliPrintf("CLI> ");
    for (;;) {
        apMain();
    }
}

static void setPeriodTask(uint32_t period){
    led_toggle_period = period;
    temp_period = period;
}

static void apStopAutoTask(void){
  led_toggle_period=0;
  temp_period=0;
  tempStopAuto();
  ledOff();
}

void apInit(void)
{
    hwInit();
    monitorInit();
    cliSetCtrlCHandler(apStopAutoTask);
    monitorSetSyncHandler(setPeriodTask);
}

void apMain(void)
{
    cliMain();
}
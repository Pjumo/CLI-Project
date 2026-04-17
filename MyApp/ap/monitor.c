#include "monitor.h"

typedef enum { MONITOR_MODE_OFF = 0, MONITOR_MODE_BINARY, MONITOR_MODE_ASCII } monitor_mode_t;

static monitor_packet_t g_packet;
static osMutexId_t monitor_mtx = NULL;
static monitor_mode_t current_monitor_mode = MONITOR_MODE_OFF;
static monitor_sync_cb_t monitor_sync_handler = NULL;

uint32_t monitor_period = 1000;

static uint8_t calcChecksum(uint8_t *data, uint32_t len)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < len; i++) {
        sum ^= data[i];
    }
    return sum;
}

static void cliMonitor(uint8_t argc, char **argv)
{
    if (argc >= 2) {
        if (strcmp(argv[1], "on") == 0) {
            if (argc == 3) {
                uint32_t period = atoi(argv[2]);
                LOG_INF("Monitor Print Period Set to %d ms\r\n", period);
                monitorSetPeriod(period);
                monitor_sync_handler(period);
            } else {
                LOG_INF("Monitor Print Period Set to Default (1000 ms)\r\n");
                monitorSetPeriod(1000);
                monitor_sync_handler(1000);
            }
            current_monitor_mode = MONITOR_MODE_ASCII;
            LOG_INF("Monitoring Mode: ON (ASCII)\r\n");
            return;
        } else if (strcmp(argv[1], "off") == 0) {
            monitorOff();
            LOG_INF("Monitoring Mode OFF (Text Mode Restored)\r\n");
            return;
        }
    }

    cliPrintf("Usage: mon on [period]\r\n");
    cliPrintf("       mon off\r\n");
    if (current_monitor_mode == MONITOR_MODE_ASCII) {
        LOG_INF("Current Mode : ON (ASCII)\r\n");
    } else {
        LOG_INF("Current Mode: OFF\r\n");
    }
}

void monitorInit()
{
    memset(&g_packet, 0, sizeof(g_packet));

    if (monitor_mtx == NULL) {
        monitor_mtx = osMutexNew(NULL);
    }
    cliAdd("mon", cliMonitor);
}

bool isMonitoringOn()
{
    return (current_monitor_mode != MONITOR_MODE_OFF);
}

void monitorUpdateValue(SensorID id, DataType type, void *p_val)
{
    if (monitor_mtx == NULL)
        return;

    osMutexAcquire(monitor_mtx, osWaitForever);

    // sensor id chk
    int target_idx = -1;
    for (int i = 0; i < g_packet.count; i++) {
        if (g_packet.nodes[i].id == (uint8_t)id) {
            target_idx = i;
            break;
        }
    }

    // new sensor
    if (target_idx == -1) {
        if (g_packet.count < MAX_SENSOR_NODES) {
            target_idx = g_packet.count;
            g_packet.nodes[target_idx].id = (uint8_t)id;
            g_packet.count++;
        } else {
            osMutexRelease(monitor_mtx);
            return;
        }
    }

    // update
    g_packet.nodes[target_idx].type = (uint8_t)type;
    if (type == TYPE_UINT8 || type == TYPE_BOOL) {
        g_packet.nodes[target_idx].value.u8_val[0] = *(uint8_t *)p_val;
    } else {
        memcpy(&g_packet.nodes[target_idx].value, p_val, 4);
    }

    osMutexRelease(monitor_mtx);
}

void monitorSendPacket()
{
    if (current_monitor_mode == MONITOR_MODE_OFF || monitor_mtx == NULL)
        return; // c는 에러 가드를 만들어야함

    osMutexAcquire(monitor_mtx, osWaitForever); // 2개 사이가 가까워야 함

    char tx_buf[256] = {0};
    uint32_t len = 0;

    if (current_monitor_mode == MONITOR_MODE_ASCII) {
        // 시작 문자 $
        len = snprintf(tx_buf, sizeof(tx_buf), "$"); // tx_buf에 $ copy

        // 구분자 + 노드의 정보들
        for (int i = 0; i < g_packet.count; i++) {
            uint8_t id = g_packet.nodes[i].id;
            uint8_t type = g_packet.nodes[i].type;

            if (i > 0) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, ",");
            }

            // id, type
            len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%d:%d:", id, type);

            // value
            if (type == TYPE_UINT8 || type == TYPE_BOOL) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%u",
                                g_packet.nodes[i].value.u8_val[0]);
            } else if (type == TYPE_INT32) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%ld",
                                g_packet.nodes[i].value.i_val);
            } else if (type == TYPE_FLOAT) {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%.2f",
                                g_packet.nodes[i].value.f_val);
            } else {
                len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "%lu",
                                g_packet.nodes[i].value.u_val);
            }
        }

        // 종료#
        len += snprintf(tx_buf + len, sizeof(tx_buf) - len, "#\r\n");
    } else {
    }

    uartWrite(0, (uint8_t *)tx_buf, len);

    osMutexRelease(monitor_mtx);
}

void monitorOff(void)
{
    current_monitor_mode = MONITOR_MODE_OFF;
}

uint32_t monitorGetPeriod(void)
{
    return monitor_period;
}

void monitorSetPeriod(uint32_t period)
{
    monitor_period = period;
}

void monitorSetSyncHandler(monitor_sync_cb_t handler)
{
    monitor_sync_handler = handler;
}
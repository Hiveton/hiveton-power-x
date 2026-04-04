#include "app_controller.h"

#include "FreeRTOS.h"
#include "task.h"

static app_mode_t g_app_mode = APP_MODE_IDLE;

void app_controller_init(void)
{
    taskENTER_CRITICAL();
    g_app_mode = APP_MODE_MONITOR;
    taskEXIT_CRITICAL();
}

app_mode_t app_controller_get_mode(void)
{
    app_mode_t mode;

    taskENTER_CRITICAL();
    mode = g_app_mode;
    taskEXIT_CRITICAL();

    return mode;
}

void app_controller_set_mode(app_mode_t mode)
{
    if ((mode < APP_MODE_IDLE) || (mode > APP_MODE_ERROR))
    {
        return;
    }

    taskENTER_CRITICAL();
    g_app_mode = mode;
    taskEXIT_CRITICAL();
}

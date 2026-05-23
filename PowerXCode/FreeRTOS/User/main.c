#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_controller.h"
#include "app_tasks.h"
#include "bsp_board.h"
#include "bsp_board_config.h"
#include "bsp_lcd_st7735.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();

    bsp_board_init();
    app_controller_init();
    app_tasks_create();

    vTaskStartScheduler();

    bsp_lcd_fill_color(0x001FU);
    taskDISABLE_INTERRUPTS();
    for (;;)
    {
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;

    taskDISABLE_INTERRUPTS();
    bsp_lcd_fill_color(0xF81FU);
    for (;;)
    {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    bsp_lcd_fill_color(0xF800U);
    for (;;)
    {
    }
}


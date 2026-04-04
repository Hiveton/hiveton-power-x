#include "debug.h"
#include "FreeRTOS.h"
#include "task.h"
#include "app_controller.h"
#include "app_tasks.h"
#include "bsp_board.h"

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
    SystemCoreClockUpdate();
    Delay_Init();
    USART_Printf_Init(115200);
    printf("\r\n========================================\r\n");
    printf(" Hiveton PX1 boot\r\n");
    printf("========================================\r\n");
    printf("SystemClk:%d\r\n", SystemCoreClock);
    printf("ChipID:%08x\r\n", DBGMCU_GetCHIPID());
    printf("FreeRTOS Kernel Version:%s\r\n", tskKERNEL_VERSION_NUMBER);

    bsp_board_init();
    app_controller_init();
    app_tasks_create();

    vTaskStartScheduler();

    printf("PX1 fatal: scheduler failed to start\r\n");
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
    printf("PX1 fatal: stack overflow detected\r\n");
    for (;;)
    {
    }
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    printf("PX1 fatal: malloc failed\r\n");
    for (;;)
    {
    }
}


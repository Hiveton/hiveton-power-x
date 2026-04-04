#include "FreeRTOS.h"
#include "task.h"

#include "bsp_adc_dma.h"
#include "bsp_board_config.h"

#if defined(__riscv)
#include "debug.h"

#include "ch32l103_adc.h"
#include "ch32l103_dma.h"
#include "ch32l103_gpio.h"
#include "ch32l103_misc.h"
#include "ch32l103_rcc.h"
#endif

static bsp_adc_window_t g_adc_window;
static volatile uint8_t g_adc_window_ready;

#if defined(__riscv)
#define PX1_ADC_CHANNEL_COUNT 2U
#define PX1_ADC_DMA_RAW_COUNT (BSP_ADC_SAMPLE_COUNT * PX1_ADC_CHANNEL_COUNT * 2U)
#define PX1_ADC_HALF_RAW_COUNT (PX1_ADC_DMA_RAW_COUNT / 2U)
#define PX1_ADC_VOLTAGE_CHANNEL ADC_Channel_8
#define PX1_ADC_CURRENT_CHANNEL ADC_Channel_9

static volatile uint16_t g_adc_dma_raw[PX1_ADC_DMA_RAW_COUNT];
static int16_t g_adc_calibration;

static uint16_t bsp_adc_dma_apply_calibration(uint16_t raw)
{
    int32_t adjusted;

    adjusted = (int32_t)raw + (int32_t)g_adc_calibration;
    if (adjusted < 0)
    {
        adjusted = 0;
    }
    else if (adjusted > 4095)
    {
        adjusted = 4095;
    }

    return (uint16_t)adjusted;
}

static int32_t bsp_adc_dma_raw_to_voltage_mv(uint16_t raw)
{
    int64_t adc_mv;

    if ((PX1_BOARD_ADC_FULL_SCALE_COUNTS == 0) || (PX1_BOARD_ADC_VBUS_DEN == 0))
    {
        return 0;
    }

    adc_mv = ((int64_t)bsp_adc_dma_apply_calibration(raw) * PX1_BOARD_ADC_VREF_MV) /
             PX1_BOARD_ADC_FULL_SCALE_COUNTS;
    adc_mv = (adc_mv * PX1_BOARD_ADC_VBUS_NUM) / PX1_BOARD_ADC_VBUS_DEN;
    return (int32_t)adc_mv;
}

static int32_t bsp_adc_dma_raw_to_current_ma(uint16_t raw)
{
    int32_t adjusted_raw;
    int64_t current_ma;

    if (PX1_BOARD_ADC_CURRENT_NUMERATOR == 0)
    {
        return 0;
    }

    if (PX1_BOARD_ADC_CURRENT_DENOMINATOR == 0)
    {
        return 0;
    }

    adjusted_raw = (int32_t)bsp_adc_dma_apply_calibration(raw) - PX1_BOARD_ADC_CURRENT_ZERO_RAW;
    current_ma = ((int64_t)adjusted_raw * PX1_BOARD_ADC_CURRENT_NUMERATOR) /
                 PX1_BOARD_ADC_CURRENT_DENOMINATOR;
    return (int32_t)current_ma;
}

static void bsp_adc_dma_publish_window(uint16_t raw_offset)
{
    uint16_t index;

    for (index = 0U; index < BSP_ADC_SAMPLE_COUNT; ++index)
    {
        uint16_t sample_base;

        sample_base = raw_offset + (uint16_t)(index * PX1_ADC_CHANNEL_COUNT);
        g_adc_window.voltage[index] = bsp_adc_dma_raw_to_voltage_mv(g_adc_dma_raw[sample_base]);
        g_adc_window.current[index] = bsp_adc_dma_raw_to_current_ma(g_adc_dma_raw[sample_base + 1U]);
    }

    g_adc_window_ready = 1U;
}
#endif

void bsp_adc_dma_init(void)
{
    g_adc_window = (bsp_adc_window_t){ 0 };
    g_adc_window_ready = 0U;

#if defined(__riscv)
    {
        ADC_InitTypeDef adc_init = { 0 };
        DMA_InitTypeDef dma_init = { 0 };
        GPIO_InitTypeDef gpio_init = { 0 };
        NVIC_InitTypeDef nvic_init = { 0 };

        RCC_HBPeriphClockCmd(RCC_HBPeriph_DMA1, ENABLE);
        RCC_PB2PeriphClockCmd(PX1_ADC_GPIO_CLOCK | RCC_PB2Periph_ADC1, ENABLE);
        RCC_ADCCLKConfig(RCC_PCLK2_Div8);

        gpio_init.GPIO_Pin = PX1_ADC_VBUS_PIN | PX1_ADC_CURRENT_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_AIN;
        GPIO_Init(PX1_ADC_GPIO, &gpio_init);

        DMA_DeInit(DMA1_Channel1);
        dma_init.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->RDATAR;
        dma_init.DMA_MemoryBaseAddr = (uint32_t)g_adc_dma_raw;
        dma_init.DMA_DIR = DMA_DIR_PeripheralSRC;
        dma_init.DMA_BufferSize = PX1_ADC_DMA_RAW_COUNT;
        dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
        dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
        dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
        dma_init.DMA_Mode = DMA_Mode_Circular;
        dma_init.DMA_Priority = DMA_Priority_VeryHigh;
        dma_init.DMA_M2M = DMA_M2M_Disable;
        DMA_Init(DMA1_Channel1, &dma_init);
        DMA_ITConfig(DMA1_Channel1, DMA_IT_HT | DMA_IT_TC | DMA_IT_TE, ENABLE);
        DMA_Cmd(DMA1_Channel1, ENABLE);

        ADC_DeInit(ADC1);
        g_adc_calibration = Get_CalibrationValue(ADC1);

        adc_init.ADC_Mode = ADC_Mode_Independent;
        adc_init.ADC_ScanConvMode = ENABLE;
        adc_init.ADC_ContinuousConvMode = ENABLE;
        adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
        adc_init.ADC_DataAlign = ADC_DataAlign_Right;
        adc_init.ADC_NbrOfChannel = PX1_ADC_CHANNEL_COUNT;
        ADC_Init(ADC1, &adc_init);
        ADC_RegularChannelConfig(ADC1, PX1_ADC_VOLTAGE_CHANNEL, 1U, ADC_SampleTime_CyclesMode7);
        ADC_RegularChannelConfig(ADC1, PX1_ADC_CURRENT_CHANNEL, 2U, ADC_SampleTime_CyclesMode7);
        ADC_DMACmd(ADC1, ENABLE);
        ADC_Cmd(ADC1, ENABLE);
        ADC_FIFO_Cmd(ADC1, ENABLE);
        ADC_BufferCmd(ADC1, DISABLE);
        ADC_ResetCalibration(ADC1);
        while (ADC_GetResetCalibrationStatus(ADC1) != RESET)
        {
        }
        ADC_StartCalibration(ADC1);
        while (ADC_GetCalibrationStatus(ADC1) != RESET)
        {
        }

        nvic_init.NVIC_IRQChannel = DMA1_Channel1_IRQn;
        nvic_init.NVIC_IRQChannelPreemptionPriority = 1U;
        nvic_init.NVIC_IRQChannelSubPriority = 0U;
        nvic_init.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_init);

        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    }
#endif
}

int bsp_adc_dma_fetch_window(bsp_adc_window_t *window)
{
    if (window == NULL)
    {
        return 0;
    }

    taskENTER_CRITICAL();
    if (g_adc_window_ready == 0U)
    {
        taskEXIT_CRITICAL();
        return 0;
    }

    *window = g_adc_window;
    g_adc_window_ready = 0U;
    taskEXIT_CRITICAL();

    return 1;
}

uint8_t bsp_adc_dma_voltage_is_calibrated(void)
{
    if ((PX1_BOARD_ADC_FULL_SCALE_COUNTS == 0) || (PX1_BOARD_ADC_VBUS_DEN == 0))
    {
        return 0U;
    }

    return 1U;
}

uint8_t bsp_adc_dma_current_is_calibrated(void)
{
    if ((PX1_BOARD_ADC_CURRENT_NUMERATOR == 0) || (PX1_BOARD_ADC_CURRENT_DENOMINATOR == 0))
    {
        return 0U;
    }

    return 1U;
}

void bsp_adc_dma_irq_handler(void)
{
#if defined(__riscv)
    if (DMA_GetITStatus(DMA1_IT_TE1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TE1);
    }

    if (DMA_GetITStatus(DMA1_IT_HT1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_HT1);
        bsp_adc_dma_publish_window(0U);
    }

    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        bsp_adc_dma_publish_window(PX1_ADC_HALF_RAW_COUNT);
    }
#else
    g_adc_window_ready = 1U;
#endif
}

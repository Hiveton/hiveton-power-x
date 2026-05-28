#ifndef BSP_ADC_DMA_H
#define BSP_ADC_DMA_H

#include <stdint.h>

#include "bsp_board_config.h"

#define BSP_ADC_SAMPLE_COUNT 8U
#define BSP_INA226_BUS_RAW_MV_NUM 5
#define BSP_INA226_BUS_RAW_MV_DEN 4
#define BSP_INA226_SHUNT_RAW_UV_NUM 5
#define BSP_INA226_SHUNT_RAW_UV_DEN 2

typedef struct
{
    int32_t voltage[BSP_ADC_SAMPLE_COUNT];
    int32_t current[BSP_ADC_SAMPLE_COUNT];
    int32_t current_deci_ma[BSP_ADC_SAMPLE_COUNT];
} bsp_adc_window_t;

static inline int32_t bsp_adc_dma_ina226_bus_raw_to_voltage_mv(uint16_t raw)
{
    return ((int32_t)raw * BSP_INA226_BUS_RAW_MV_NUM) /
           BSP_INA226_BUS_RAW_MV_DEN;
}

static inline int32_t bsp_adc_dma_ina226_shunt_raw_to_current_ma(int16_t raw)
{
    int32_t numerator;
    int32_t denominator;

    numerator = (int32_t)raw *
                PX1_BOARD_INA226_CURRENT_SIGN *
                BSP_INA226_SHUNT_RAW_UV_NUM;
    denominator = PX1_BOARD_INA226_SHUNT_MILLIOHM *
                  BSP_INA226_SHUNT_RAW_UV_DEN;
    if (denominator == 0)
    {
        return 0;
    }

    return numerator / denominator;
}

static inline int32_t bsp_adc_dma_ina226_shunt_raw_to_current_deci_ma(int16_t raw)
{
    int32_t numerator;
    int32_t denominator;

    numerator = (int32_t)raw *
                PX1_BOARD_INA226_CURRENT_SIGN *
                BSP_INA226_SHUNT_RAW_UV_NUM *
                10;
    denominator = PX1_BOARD_INA226_SHUNT_MILLIOHM *
                  BSP_INA226_SHUNT_RAW_UV_DEN;
    if (denominator == 0)
    {
        return 0;
    }

    return numerator / denominator;
}

void bsp_adc_dma_init(void);
int bsp_adc_dma_fetch_window(bsp_adc_window_t *window);
int bsp_adc_dma_read_mcu_temp_deci_c(int32_t *temp_deci_c);
uint8_t bsp_adc_dma_voltage_is_calibrated(void);
uint8_t bsp_adc_dma_current_is_calibrated(void);
void bsp_adc_dma_irq_handler(void);

#endif /* BSP_ADC_DMA_H */

#ifndef BSP_ADC_DMA_H
#define BSP_ADC_DMA_H

#include <stdint.h>

#define BSP_ADC_SAMPLE_COUNT 64U

typedef struct
{
    int32_t voltage[BSP_ADC_SAMPLE_COUNT];
    int32_t current[BSP_ADC_SAMPLE_COUNT];
} bsp_adc_window_t;

void bsp_adc_dma_init(void);
int bsp_adc_dma_fetch_window(bsp_adc_window_t *window);
uint8_t bsp_adc_dma_voltage_is_calibrated(void);
uint8_t bsp_adc_dma_current_is_calibrated(void);
void bsp_adc_dma_irq_handler(void);

#endif /* BSP_ADC_DMA_H */

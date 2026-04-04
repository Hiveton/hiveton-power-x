#include <assert.h>

#include "bsp_adc_dma.h"

int main(void)
{
    assert(bsp_adc_dma_voltage_is_calibrated() == 1U);
    assert(bsp_adc_dma_current_is_calibrated() == 0U);
    return 0;
}

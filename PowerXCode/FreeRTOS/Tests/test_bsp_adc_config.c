#include <assert.h>

#include "bsp_adc_dma.h"

int main(void)
{
    assert(BSP_ADC_SAMPLE_COUNT == 4U);
    assert(PX1_BOARD_INA226_ADDRESS_7BIT == 0x40U);
    assert(PX1_BOARD_INA226_SHUNT_MILLIOHM == 5);
    assert(PX1_BOARD_INA226_CURRENT_SIGN == -1);
    assert(bsp_adc_dma_ina226_bus_raw_to_voltage_mv(8000U) == 10000);
    assert(bsp_adc_dma_ina226_shunt_raw_to_current_ma(-2000) == 1000);
    assert(bsp_adc_dma_ina226_shunt_raw_to_current_ma(2000) == -1000);
    return 0;
}

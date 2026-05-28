#include "bsp_adc_dma.h"

#if defined(__riscv)
#include "ch32l103_adc.h"
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#endif

#define PX1_INA226_REG_CONFIG 0x00U
#define PX1_INA226_REG_SHUNT_VOLTAGE 0x01U
#define PX1_INA226_REG_BUS_VOLTAGE 0x02U
#define PX1_INA226_REG_MASK_ENABLE 0x06U
#define PX1_INA226_MASK_CVRF 0x0008U
#define PX1_INA226_READY_POLL_LIMIT 12U
/*
 * AVG=1, VBUSCT=140us, VSHCT=140us, continuous shunt+bus.
 * The power monitor is the only VBUS measurement path on this board, so use
 * the fastest INA226 conversion mode for the ripple/scope pages.
 */
#define PX1_INA226_CONFIG_CONTINUOUS 0x4007U

#if defined(__riscv)
#define PX1_I2C_DELAY_LOOPS 48U
#define PX1_I2C_SCL_WAIT_LOOPS 2000U
#define PX1_ADC_CAL_WAIT_GUARD 100000UL
#define PX1_ADC_EOC_WAIT_GUARD 100000UL

static uint8_t g_mcu_temp_adc_ready;

static void px1_i2c_delay(void)
{
    volatile uint32_t guard;

    guard = PX1_I2C_DELAY_LOOPS;
    while (guard != 0U)
    {
        --guard;
    }
}

static void px1_i2c_sda_high(void)
{
    GPIO_SetBits(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SDA_PIN);
}

static void px1_i2c_sda_low(void)
{
    GPIO_ResetBits(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SDA_PIN);
}

static void px1_i2c_scl_low(void)
{
    GPIO_ResetBits(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SCL_PIN);
    px1_i2c_delay();
}

static uint8_t px1_i2c_scl_high(void)
{
    uint32_t guard;

    GPIO_SetBits(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SCL_PIN);
    guard = PX1_I2C_SCL_WAIT_LOOPS;
    while ((GPIO_ReadInputDataBit(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SCL_PIN) == 0U) &&
           (guard != 0U))
    {
        --guard;
    }
    px1_i2c_delay();

    return (guard != 0U) ? 1U : 0U;
}

static uint8_t px1_i2c_read_sda(void)
{
    return GPIO_ReadInputDataBit(PX1_INA226_I2C_GPIO, PX1_INA226_I2C_SDA_PIN);
}

static void px1_i2c_stop(void)
{
    px1_i2c_sda_low();
    px1_i2c_delay();
    (void)px1_i2c_scl_high();
    px1_i2c_sda_high();
    px1_i2c_delay();
}

static uint8_t px1_i2c_start(void)
{
    px1_i2c_sda_high();
    if (px1_i2c_scl_high() == 0U)
    {
        return 0U;
    }
    px1_i2c_sda_low();
    px1_i2c_delay();
    px1_i2c_scl_low();

    return 1U;
}

static uint8_t px1_i2c_write_byte(uint8_t value)
{
    uint8_t mask;
    uint8_t ack;

    for (mask = 0x80U; mask != 0U; mask >>= 1U)
    {
        if ((value & mask) != 0U)
        {
            px1_i2c_sda_high();
        }
        else
        {
            px1_i2c_sda_low();
        }

        px1_i2c_delay();
        if (px1_i2c_scl_high() == 0U)
        {
            return 0U;
        }
        px1_i2c_scl_low();
    }

    px1_i2c_sda_high();
    px1_i2c_delay();
    if (px1_i2c_scl_high() == 0U)
    {
        return 0U;
    }
    ack = (px1_i2c_read_sda() == 0U) ? 1U : 0U;
    px1_i2c_scl_low();

    return ack;
}

static uint8_t px1_i2c_read_byte(uint8_t ack, uint8_t *value)
{
    uint8_t data;
    uint8_t index;

    if (value == 0)
    {
        return 0U;
    }

    data = 0U;
    px1_i2c_sda_high();
    for (index = 0U; index < 8U; ++index)
    {
        data <<= 1U;
        if (px1_i2c_scl_high() == 0U)
        {
            return 0U;
        }
        if (px1_i2c_read_sda() != 0U)
        {
            data |= 0x01U;
        }
        px1_i2c_scl_low();
    }

    if (ack != 0U)
    {
        px1_i2c_sda_low();
    }
    else
    {
        px1_i2c_sda_high();
    }
    px1_i2c_delay();
    if (px1_i2c_scl_high() == 0U)
    {
        return 0U;
    }
    px1_i2c_scl_low();
    px1_i2c_sda_high();

    *value = data;
    return 1U;
}

static void px1_i2c_recover_bus(void)
{
    uint8_t index;

    px1_i2c_sda_high();
    for (index = 0U; index < 9U; ++index)
    {
        if (px1_i2c_read_sda() != 0U)
        {
            break;
        }

        if (px1_i2c_scl_high() == 0U)
        {
            break;
        }
        px1_i2c_scl_low();
    }
    px1_i2c_stop();
}

static void px1_i2c_gpio_init(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    RCC_PB2PeriphClockCmd(PX1_INA226_I2C_GPIO_CLOCK, ENABLE);
    gpio_init.GPIO_Pin = PX1_INA226_I2C_SCL_PIN | PX1_INA226_I2C_SDA_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_OD;
    gpio_init.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(PX1_INA226_I2C_GPIO, &gpio_init);

    px1_i2c_scl_low();
    px1_i2c_sda_high();
    (void)px1_i2c_scl_high();
    px1_i2c_recover_bus();
}

static uint8_t px1_ina226_write_register(uint8_t reg, uint16_t value)
{
    uint8_t address;

    address = (uint8_t)(PX1_BOARD_INA226_ADDRESS_7BIT << 1U);
    if (px1_i2c_start() == 0U)
    {
        return 0U;
    }
    if (px1_i2c_write_byte(address) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_write_byte(reg) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_write_byte((uint8_t)(value >> 8U)) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_write_byte((uint8_t)(value & 0xFFU)) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }

    px1_i2c_stop();
    return 1U;
}

static uint8_t px1_ina226_read_register(uint8_t reg, uint16_t *value)
{
    uint8_t address;
    uint8_t msb;
    uint8_t lsb;

    if (value == 0)
    {
        return 0U;
    }

    address = (uint8_t)(PX1_BOARD_INA226_ADDRESS_7BIT << 1U);
    if (px1_i2c_start() == 0U)
    {
        return 0U;
    }
    if (px1_i2c_write_byte(address) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_write_byte(reg) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_start() == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_write_byte((uint8_t)(address | 0x01U)) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_read_byte(1U, &msb) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }
    if (px1_i2c_read_byte(0U, &lsb) == 0U)
    {
        px1_i2c_stop();
        return 0U;
    }

    px1_i2c_stop();
    *value = (uint16_t)(((uint16_t)msb << 8U) | lsb);
    return 1U;
}

static uint8_t px1_ina226_wait_conversion_ready(void)
{
    uint8_t attempt;

    for (attempt = 0U; attempt < PX1_INA226_READY_POLL_LIMIT; ++attempt)
    {
        uint16_t mask_enable;

        if (px1_ina226_read_register(PX1_INA226_REG_MASK_ENABLE, &mask_enable) == 0U)
        {
            return 0U;
        }
        if ((mask_enable & PX1_INA226_MASK_CVRF) != 0U)
        {
            return 1U;
        }
    }

    return 0U;
}

static void px1_mcu_temp_adc_init(void)
{
    ADC_InitTypeDef adc_init = { 0 };
    uint32_t guard;

    RCC_PB2PeriphClockCmd(RCC_PB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);
    ADC_DeInit(ADC1);
    (void)Get_CalibrationValue(ADC1);
    adc_init.ADC_Mode = ADC_Mode_Independent;
    adc_init.ADC_ScanConvMode = DISABLE;
    adc_init.ADC_ContinuousConvMode = DISABLE;
    adc_init.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc_init.ADC_DataAlign = ADC_DataAlign_Right;
    adc_init.ADC_NbrOfChannel = 1;
    ADC_Init(ADC1, &adc_init);
    ADC_Cmd(ADC1, ENABLE);
    ADC_FIFO_Cmd(ADC1, ENABLE);
    ADC_BufferCmd(ADC1, DISABLE);
    ADC_ResetCalibration(ADC1);
    guard = PX1_ADC_CAL_WAIT_GUARD;
    while ((ADC_GetResetCalibrationStatus(ADC1)) &&
           (guard != 0UL))
    {
        --guard;
    }
    if (guard == 0UL)
    {
        g_mcu_temp_adc_ready = 0U;
        return;
    }
    ADC_StartCalibration(ADC1);
    guard = PX1_ADC_CAL_WAIT_GUARD;
    while ((ADC_GetCalibrationStatus(ADC1)) &&
           (guard != 0UL))
    {
        --guard;
    }
    if (guard == 0UL)
    {
        g_mcu_temp_adc_ready = 0U;
        return;
    }
    ADC_TempSensorVrefintCmd(ENABLE);
    g_mcu_temp_adc_ready = 1U;
}
#else
static bsp_adc_window_t g_adc_window;
static volatile uint8_t g_adc_window_ready;
#endif

void bsp_adc_dma_init(void)
{
#if defined(__riscv)
    px1_i2c_gpio_init();
    g_mcu_temp_adc_ready = 0U;
    px1_mcu_temp_adc_init();
    (void)px1_ina226_write_register(PX1_INA226_REG_CONFIG, PX1_INA226_CONFIG_CONTINUOUS);
#else
    g_adc_window = (bsp_adc_window_t){ 0 };
    g_adc_window_ready = 0U;
#endif
}

int bsp_adc_dma_fetch_window(bsp_adc_window_t *window)
{
    if (window == 0)
    {
        return 0;
    }

#if defined(__riscv)
    {
        uint16_t index;
        uint16_t bus_raw;
        uint16_t shunt_raw;

        for (index = 0U; index < BSP_ADC_SAMPLE_COUNT; ++index)
        {
            if (px1_ina226_wait_conversion_ready() == 0U)
            {
                return 0;
            }
            if (px1_ina226_read_register(PX1_INA226_REG_BUS_VOLTAGE, &bus_raw) == 0U)
            {
                return 0;
            }
            if (px1_ina226_read_register(PX1_INA226_REG_SHUNT_VOLTAGE, &shunt_raw) == 0U)
            {
                return 0;
            }

            window->voltage[index] = bsp_adc_dma_ina226_bus_raw_to_voltage_mv(bus_raw);
            window->current[index] = bsp_adc_dma_ina226_shunt_raw_to_current_ma((int16_t)shunt_raw);
            window->current_deci_ma[index] = bsp_adc_dma_ina226_shunt_raw_to_current_deci_ma((int16_t)shunt_raw);
        }
    }
#else
    if (g_adc_window_ready == 0U)
    {
        return 0;
    }

    *window = g_adc_window;
    g_adc_window_ready = 0U;
#endif

    return 1;
}

int bsp_adc_dma_read_mcu_temp_deci_c(int32_t *temp_deci_c)
{
    if (temp_deci_c == 0)
    {
        return 0;
    }

#if defined(__riscv)
    {
        uint32_t guard;
        uint16_t raw;
        int32_t mv;

        if (g_mcu_temp_adc_ready == 0U)
        {
            px1_mcu_temp_adc_init();
            if (g_mcu_temp_adc_ready == 0U)
            {
                return 0;
            }
        }
        ADC_TempSensorVrefintCmd(ENABLE);
        ADC_RegularChannelConfig(ADC1, ADC_Channel_TempSensor, 1U, ADC_SampleTime_CyclesMode7);
        ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
        ADC_SoftwareStartConvCmd(ADC1, ENABLE);
        guard = PX1_ADC_EOC_WAIT_GUARD;
        while ((ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) &&
               (guard != 0UL))
        {
            --guard;
        }
        if (guard == 0UL)
        {
            return 0;
        }
        raw = ADC_GetConversionValue(ADC1);
        mv = ((int32_t)raw * PX1_BOARD_ADC_VREF_MV) / PX1_BOARD_ADC_FULL_SCALE_COUNTS;
        *temp_deci_c = TempSensor_Volt_To_Temper(mv);
        return 1;
    }
#else
    *temp_deci_c = 324;
    return 1;
#endif
}

uint8_t bsp_adc_dma_voltage_is_calibrated(void)
{
    return 1U;
}

uint8_t bsp_adc_dma_current_is_calibrated(void)
{
    return (PX1_BOARD_INA226_SHUNT_MILLIOHM != 0) ? 1U : 0U;
}

void bsp_adc_dma_irq_handler(void)
{
#if !defined(__riscv)
    g_adc_window_ready = 1U;
#endif
}

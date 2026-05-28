#include "bsp_dpdm.h"
#include "bsp_board_config.h"

#if defined(__riscv)
#include "debug.h"

#include "ch32l103.h"
#include "ch32l103_adc.h"
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#endif

#define BSP_DPDM_QC3_PULSE_US 1500U
#define BSP_DPDM_QC3_SETTLE_US 1250U

static bsp_dpdm_mode_t g_dpdm_mode = BSP_DPDM_MODE_HIZ;
static bsp_dpdm_level_t g_dp_level = BSP_DPDM_LEVEL_HIZ;
static bsp_dpdm_level_t g_dm_level = BSP_DPDM_LEVEL_HIZ;
#if defined(PX1_HOST_TEST)
static bsp_dpdm_sample_t g_mock_sample = { 0U, 0U, 0U, 0, 0 };
static int g_mock_qc3_offset;
static int g_mock_qc3_pulse_count;
static unsigned char g_mock_bc_source_mask;
#endif

#if defined(__riscv)
#define PX1_DPDM_ADC_CAL_WAIT_GUARD 100000UL
#define PX1_DPDM_ADC_EOC_WAIT_GUARD 100000UL

static uint8_t g_dpdm_adc_ready;

static void bsp_dpdm_gpio_to_floating(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    gpio_init.GPIO_Pin = PX1_USB_DM_PIN | PX1_USB_DP_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PX1_DPDM_GPIO, &gpio_init);
}

static void bsp_dpdm_gpio_apply_levels(bsp_dpdm_level_t dp_level, bsp_dpdm_level_t dm_level)
{
    GPIO_InitTypeDef gpio_init = { 0 };
    uint16_t output_pins;
    uint16_t float_pins;

    output_pins = 0U;
    float_pins = 0U;

    if ((dp_level == BSP_DPDM_LEVEL_LOW) || (dp_level == BSP_DPDM_LEVEL_3300MV))
    {
        output_pins |= PX1_USB_DP_PIN;
    }
    else
    {
        float_pins |= PX1_USB_DP_PIN;
    }

    if ((dm_level == BSP_DPDM_LEVEL_LOW) || (dm_level == BSP_DPDM_LEVEL_3300MV))
    {
        output_pins |= PX1_USB_DM_PIN;
    }
    else
    {
        float_pins |= PX1_USB_DM_PIN;
    }

    if (float_pins != 0U)
    {
        gpio_init.GPIO_Pin = float_pins;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PX1_DPDM_GPIO, &gpio_init);
    }

    if (output_pins != 0U)
    {
        gpio_init.GPIO_Pin = output_pins;
        gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PX1_DPDM_GPIO, &gpio_init);

        if (dp_level == BSP_DPDM_LEVEL_LOW)
        {
            GPIO_ResetBits(PX1_DPDM_GPIO, PX1_USB_DP_PIN);
        }
        else if (dp_level == BSP_DPDM_LEVEL_3300MV)
        {
            GPIO_SetBits(PX1_DPDM_GPIO, PX1_USB_DP_PIN);
        }

        if (dm_level == BSP_DPDM_LEVEL_LOW)
        {
            GPIO_ResetBits(PX1_DPDM_GPIO, PX1_USB_DM_PIN);
        }
        else if (dm_level == BSP_DPDM_LEVEL_3300MV)
        {
            GPIO_SetBits(PX1_DPDM_GPIO, PX1_USB_DM_PIN);
        }
    }
}

static void bsp_dpdm_apply_bc_source(uint8_t dp_source_enable, uint8_t dm_source_enable)
{
    uint32_t afio_cr;

    afio_cr = AFIO->CR;
    afio_cr &= ~(AFIO_CR_UPD_BC_VSRC | AFIO_CR_UDM_BC_VSRC |
                 AFIO_CR_UPD_BC_CMPE | AFIO_CR_UDM_BC_CMPE);

    if (dp_source_enable != 0U)
    {
        afio_cr |= AFIO_CR_UPD_BC_VSRC;
    }
    if (dm_source_enable != 0U)
    {
        afio_cr |= AFIO_CR_UDM_BC_VSRC;
    }

    AFIO->CR = afio_cr;
}

static void bsp_dpdm_adc_init(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };
    ADC_InitTypeDef adc_init = { 0 };
    uint32_t guard;

#if (PX1_BOARD_HAS_DPDM_ADC_SENSE != 0)
    RCC_PB2PeriphClockCmd(PX1_ADC_GPIO_CLOCK | RCC_PB2Periph_ADC1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div8);

    gpio_init.GPIO_Pin = PX1_DP_ADC_PIN | PX1_DM_ADC_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(PX1_ADC_GPIO, &gpio_init);

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
    guard = PX1_DPDM_ADC_CAL_WAIT_GUARD;
    while ((ADC_GetResetCalibrationStatus(ADC1)) &&
           (guard != 0UL))
    {
        --guard;
    }
    if (guard == 0UL)
    {
        g_dpdm_adc_ready = 0U;
        return;
    }
    ADC_StartCalibration(ADC1);
    guard = PX1_DPDM_ADC_CAL_WAIT_GUARD;
    while ((ADC_GetCalibrationStatus(ADC1)) &&
           (guard != 0UL))
    {
        --guard;
    }
    if (guard == 0UL)
    {
        g_dpdm_adc_ready = 0U;
        return;
    }
    g_dpdm_adc_ready = 1U;
#else
    (void)gpio_init;
    (void)adc_init;
    g_dpdm_adc_ready = 0U;
#endif
}

static uint16_t bsp_dpdm_adc_read_raw(uint8_t channel)
{
    uint32_t guard;

    ADC_RegularChannelConfig(ADC1, channel, 1U, ADC_SampleTime_CyclesMode7);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    guard = PX1_DPDM_ADC_EOC_WAIT_GUARD;
    while ((ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET) &&
           (guard != 0UL))
    {
        --guard;
    }
    if (guard == 0UL)
    {
        return 0U;
    }

    return ADC_GetConversionValue(ADC1);
}

static int bsp_dpdm_adc_raw_to_mv(uint16_t raw)
{
    return ((int)raw * PX1_BOARD_ADC_VREF_MV) / PX1_BOARD_ADC_FULL_SCALE_COUNTS;
}
#endif

#if defined(__riscv)
static void bsp_dpdm_restore_bc_source_from_levels(void)
{
    bsp_dpdm_apply_bc_source((g_dp_level == BSP_DPDM_LEVEL_600MV) ? 1U : 0U,
                             (g_dm_level == BSP_DPDM_LEVEL_600MV) ? 1U : 0U);
}
#elif defined(PX1_HOST_TEST)
static void bsp_dpdm_mock_update_bc_source_mask(void)
{
    g_mock_bc_source_mask = 0U;
    if (g_dp_level == BSP_DPDM_LEVEL_600MV)
    {
        g_mock_bc_source_mask |= 0x01U;
    }
    if (g_dm_level == BSP_DPDM_LEVEL_600MV)
    {
        g_mock_bc_source_mask |= 0x02U;
    }
}
#endif

void bsp_dpdm_init(void)
{
    g_dpdm_mode = BSP_DPDM_MODE_HIZ;
    g_dp_level = BSP_DPDM_LEVEL_HIZ;
    g_dm_level = BSP_DPDM_LEVEL_HIZ;

#if defined(__riscv)
    RCC_PB2PeriphClockCmd(PX1_DPDM_GPIO_CLOCK | PX1_USBPD_AFIO_CLOCK, ENABLE);
    bsp_dpdm_gpio_to_floating();
    bsp_dpdm_apply_bc_source(0U, 0U);
    bsp_dpdm_adc_init();
#elif defined(PX1_HOST_TEST)
    g_mock_sample = (bsp_dpdm_sample_t){ 0U, 0U, 0U, 0, 0 };
    g_mock_qc3_offset = 0;
    g_mock_qc3_pulse_count = 0;
    g_mock_bc_source_mask = 0U;
#else
#error "bsp_dpdm requires __riscv for product firmware or PX1_HOST_TEST for host tests."
#endif
}

void bsp_dpdm_set_mode(bsp_dpdm_mode_t mode)
{
    g_dpdm_mode = mode;

    switch (mode)
    {
        case BSP_DPDM_MODE_DP:
            bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_HIZ);
            break;
        case BSP_DPDM_MODE_DM:
            bsp_dpdm_set_levels(BSP_DPDM_LEVEL_HIZ, BSP_DPDM_LEVEL_600MV);
            break;
        case BSP_DPDM_MODE_BOTH:
            bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
            break;
        case BSP_DPDM_MODE_HIZ:
        default:
            bsp_dpdm_set_levels(BSP_DPDM_LEVEL_HIZ, BSP_DPDM_LEVEL_HIZ);
            break;
    }
}

bsp_dpdm_mode_t bsp_dpdm_get_mode(void)
{
    return g_dpdm_mode;
}

void bsp_dpdm_set_levels(bsp_dpdm_level_t dp_level, bsp_dpdm_level_t dm_level)
{
    g_dp_level = dp_level;
    g_dm_level = dm_level;

    if ((dp_level == BSP_DPDM_LEVEL_HIZ) && (dm_level == BSP_DPDM_LEVEL_HIZ))
    {
        g_dpdm_mode = BSP_DPDM_MODE_HIZ;
    }
    else if ((dp_level == BSP_DPDM_LEVEL_600MV) && (dm_level == BSP_DPDM_LEVEL_HIZ))
    {
        g_dpdm_mode = BSP_DPDM_MODE_DP;
    }
    else if ((dp_level == BSP_DPDM_LEVEL_HIZ) && (dm_level == BSP_DPDM_LEVEL_600MV))
    {
        g_dpdm_mode = BSP_DPDM_MODE_DM;
    }
    else if ((dp_level == BSP_DPDM_LEVEL_600MV) && (dm_level == BSP_DPDM_LEVEL_600MV))
    {
        g_dpdm_mode = BSP_DPDM_MODE_BOTH;
    }
    else
    {
        g_dpdm_mode = BSP_DPDM_MODE_BOTH;
    }

#if defined(__riscv)
    bsp_dpdm_apply_bc_source(0U, 0U);
    bsp_dpdm_gpio_apply_levels(dp_level, dm_level);
    bsp_dpdm_restore_bc_source_from_levels();
#elif defined(PX1_HOST_TEST)
    bsp_dpdm_mock_update_bc_source_mask();
#else
#error "bsp_dpdm_set_levels requires __riscv or PX1_HOST_TEST."
#endif
}

void bsp_dpdm_get_levels(bsp_dpdm_level_t *dp_level, bsp_dpdm_level_t *dm_level)
{
    if (dp_level != 0)
    {
        *dp_level = g_dp_level;
    }
    if (dm_level != 0)
    {
        *dm_level = g_dm_level;
    }
}

void bsp_dpdm_apply_qc2_voltage_mv(int target_mv)
{
    if (target_mv >= 19000)
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_3300MV, BSP_DPDM_LEVEL_3300MV);
    }
    else if (target_mv >= 11500)
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
    }
    else if (target_mv >= 8500)
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_3300MV, BSP_DPDM_LEVEL_600MV);
    }
    else
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_LOW);
    }
}

void bsp_dpdm_apply_qc3_pulse(int step_delta)
{
    if (step_delta == 0)
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
        return;
    }

#if defined(__riscv)
    bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
    Delay_Us(BSP_DPDM_QC3_SETTLE_US);
    if (step_delta > 0)
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_3300MV, BSP_DPDM_LEVEL_600MV);
    }
    else
    {
        bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_3300MV);
    }
    Delay_Us(BSP_DPDM_QC3_PULSE_US);
    bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
    Delay_Us(BSP_DPDM_QC3_SETTLE_US);
#elif defined(PX1_HOST_TEST)
    g_mock_qc3_offset += (step_delta > 0) ? 1 : -1;
    g_mock_qc3_pulse_count++;
    bsp_dpdm_set_levels(BSP_DPDM_LEVEL_600MV, BSP_DPDM_LEVEL_600MV);
#else
#error "bsp_dpdm_apply_qc3_pulse requires __riscv or PX1_HOST_TEST."
#endif
}

unsigned char bsp_dpdm_sample_lines(bsp_dpdm_sample_t *sample)
{
    if (sample == 0)
    {
        return 0U;
    }

#if defined(__riscv)
    {
        int dp_mv;
        int dm_mv;

        sample->voltage_valid = 0U;
        sample->dp_high = 0U;
        sample->dm_high = 0U;
        sample->dp_mv = 0;
        sample->dm_mv = 0;
        if (g_dpdm_adc_ready != 0U)
        {
            dp_mv = bsp_dpdm_adc_raw_to_mv(bsp_dpdm_adc_read_raw(ADC_Channel_8));
            dm_mv = bsp_dpdm_adc_raw_to_mv(bsp_dpdm_adc_read_raw(ADC_Channel_9));
            sample->dp_mv = dp_mv;
            sample->dm_mv = dm_mv;
            sample->dp_high = (dp_mv >= 300) ? 1U : 0U;
            sample->dm_high = (dm_mv >= 300) ? 1U : 0U;
            sample->voltage_valid = 1U;
        }
    }
#elif defined(PX1_HOST_TEST)
    *sample = g_mock_sample;
#else
#error "bsp_dpdm_sample_lines requires __riscv or PX1_HOST_TEST."
#endif

    return 1U;
}

#if defined(PX1_HOST_TEST)
void bsp_dpdm_mock_set_sample(unsigned char dp_high, unsigned char dm_high)
{
    g_mock_sample.dp_high = (dp_high != 0U) ? 1U : 0U;
    g_mock_sample.dm_high = (dm_high != 0U) ? 1U : 0U;
    g_mock_sample.voltage_valid = 1U;
    g_mock_sample.dp_mv = (dp_high != 0U) ? 600 : 0;
    g_mock_sample.dm_mv = (dm_high != 0U) ? 600 : 0;
}

void bsp_dpdm_mock_set_voltage_mv(int dp_mv, int dm_mv)
{
    g_mock_sample.voltage_valid = 1U;
    g_mock_sample.dp_mv = dp_mv;
    g_mock_sample.dm_mv = dm_mv;
    g_mock_sample.dp_high = (dp_mv >= 300) ? 1U : 0U;
    g_mock_sample.dm_high = (dm_mv >= 300) ? 1U : 0U;
}

void bsp_dpdm_mock_set_adc_unavailable(void)
{
    g_mock_sample.dp_high = 0U;
    g_mock_sample.dm_high = 0U;
    g_mock_sample.voltage_valid = 0U;
    g_mock_sample.dp_mv = 0;
    g_mock_sample.dm_mv = 0;
}

int bsp_dpdm_mock_get_qc3_offset(void)
{
    return g_mock_qc3_offset;
}

int bsp_dpdm_mock_get_qc3_pulse_count(void)
{
    return g_mock_qc3_pulse_count;
}

unsigned char bsp_dpdm_mock_get_bc_source_mask(void)
{
    return g_mock_bc_source_mask;
}
#endif

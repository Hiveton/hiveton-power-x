#include "bsp_cc_ext_rd.h"

#include "bsp_board_config.h"

#if defined(__riscv)
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#endif

static uint8_t g_cc_ext_rd_enabled;

void bsp_cc_ext_rd_init(void)
{
    g_cc_ext_rd_enabled = 0U;

#if defined(__riscv)
    {
        GPIO_InitTypeDef gpio_init = { 0 };

        RCC_PB2PeriphClockCmd(PX1_CC1_EXT_RD_CTL_GPIO_CLOCK, ENABLE);

        gpio_init.GPIO_Pin = PX1_CC1_EXT_RD_CTL_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PX1_CC1_EXT_RD_CTL_GPIO, &gpio_init);

        GPIO_ResetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
#endif
}

void bsp_cc_ext_rd_set(uint8_t enabled)
{
    g_cc_ext_rd_enabled = (enabled != 0U) ? 1U : 0U;

#if defined(__riscv)
    if (g_cc_ext_rd_enabled != 0U)
    {
        GPIO_SetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
    else
    {
        GPIO_ResetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
#endif
}

uint8_t bsp_cc_ext_rd_get(void)
{
    return g_cc_ext_rd_enabled;
}

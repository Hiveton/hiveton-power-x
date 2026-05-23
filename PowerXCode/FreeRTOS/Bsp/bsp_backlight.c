#include "bsp_backlight.h"

#include "bsp_board_config.h"

static uint8_t g_backlight_percent;

#if defined(__riscv)
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#include "ch32l103_tim.h"
#include "system_ch32l103.h"

#define PX1_BACKLIGHT_PWM_PERIOD 999U

static void bsp_backlight_apply_percent(uint8_t percent)
{
#if PX1_BOARD_HAS_CONFIRMED_BACKLIGHT_PWM
    uint32_t pulse;

    pulse = ((uint32_t)PX1_BACKLIGHT_PWM_PERIOD * percent) / 100U;
    if ((percent != 0U) && (pulse == 0U))
    {
        pulse = 1U;
    }

    TIM_SetCompare1(PX1_BACKLIGHT_TIM, pulse);
#else
    (void)percent;
#endif
}
#endif

void bsp_backlight_init(void)
{
    g_backlight_percent = 0U;

#if defined(__riscv)
#if PX1_BOARD_HAS_CONFIRMED_BACKLIGHT_PWM
    {
        GPIO_InitTypeDef gpio_init = { 0 };
        TIM_OCInitTypeDef oc_init = { 0 };
        TIM_TimeBaseInitTypeDef timebase_init = { 0 };
        uint32_t timer_clock_hz;
        uint16_t prescaler;

        timer_clock_hz = SystemCoreClock;
        if (timer_clock_hz == 0U)
        {
            timer_clock_hz = 48000000U;
        }

        prescaler = (uint16_t)((timer_clock_hz / 1000000U) - 1U);

        RCC_PB2PeriphClockCmd(PX1_BACKLIGHT_GPIO_CLOCK | PX1_BACKLIGHT_TIM_CLOCK, ENABLE);

        gpio_init.GPIO_Pin = PX1_BACKLIGHT_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        GPIO_Init(PX1_BACKLIGHT_GPIO, &gpio_init);

        timebase_init.TIM_Period = PX1_BACKLIGHT_PWM_PERIOD;
        timebase_init.TIM_Prescaler = prescaler;
        timebase_init.TIM_ClockDivision = TIM_CKD_DIV1;
        timebase_init.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(PX1_BACKLIGHT_TIM, &timebase_init);

        oc_init.TIM_OCMode = TIM_OCMode_PWM1;
        oc_init.TIM_OutputState = TIM_OutputState_Enable;
        oc_init.TIM_Pulse = 0U;
        oc_init.TIM_OCPolarity = TIM_OCPolarity_High;
        TIM_OC1Init(PX1_BACKLIGHT_TIM, &oc_init);

        TIM_CtrlPWMOutputs(PX1_BACKLIGHT_TIM, ENABLE);
        TIM_OC1PreloadConfig(PX1_BACKLIGHT_TIM, TIM_OCPreload_Disable);
        TIM_ARRPreloadConfig(PX1_BACKLIGHT_TIM, ENABLE);
        TIM_Cmd(PX1_BACKLIGHT_TIM, ENABLE);
        bsp_backlight_apply_percent(g_backlight_percent);
    }
#endif
#endif
}

void bsp_backlight_set(uint8_t percent)
{
    if (percent > 100U)
    {
        percent = 100U;
    }

    g_backlight_percent = percent;

#if defined(__riscv)
    bsp_backlight_apply_percent(percent);
#endif
}

uint8_t bsp_backlight_get_percent(void)
{
    return g_backlight_percent;
}

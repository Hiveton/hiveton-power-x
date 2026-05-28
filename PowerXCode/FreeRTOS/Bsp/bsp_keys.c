#include "bsp_keys.h"

#include <string.h>

#include "bsp_board_config.h"

#if defined(__riscv)
#include "ch32l103_exti.h"
#include "ch32l103_gpio.h"
#include "ch32l103_misc.h"
#include "ch32l103_rcc.h"
#endif

typedef struct
{
    uint8_t stable_level;
    uint8_t debounce_ticks;
    uint16_t hold_ticks;
    uint8_t short_reported;
    uint8_t long_reported;
} bsp_key_state_t;

#define BSP_KEYS_COUNT 3U
#define BSP_KEYS_DEBOUNCE_TICKS 1U
#define BSP_KEYS_LONG_PRESS_TICKS 16U
#define BSP_KEYS_MASK_BTN1 0x01U
#define BSP_KEYS_MASK_BTN2 0x02U
#define BSP_KEYS_MASK_BTN3 0x04U

static bsp_key_state_t g_key_state[BSP_KEYS_COUNT];
static volatile uint8_t g_key_irq_mask;
static volatile uint8_t g_key_irq_seen_mask;
static volatile uint16_t g_key_irq_count[BSP_KEYS_COUNT];
#if defined(PX1_HOST_TEST)
static uint8_t g_key_mock_raw_high_mask = BSP_KEYS_MASK_BTN1 | BSP_KEYS_MASK_BTN3;
#endif

static void bsp_keys_emit_short_event(bsp_keys_event_t *event, uint8_t index);

uint8_t bsp_keys_active_mask_from_levels(uint8_t raw_high_mask)
{
    uint8_t pressed_mask;

    pressed_mask = 0U;
    if ((raw_high_mask & BSP_KEYS_MASK_BTN1) == 0U)
    {
        pressed_mask |= BSP_KEYS_MASK_BTN1;
    }

    if ((raw_high_mask & BSP_KEYS_MASK_BTN2) != 0U)
    {
        pressed_mask |= BSP_KEYS_MASK_BTN2;
    }

    if ((raw_high_mask & BSP_KEYS_MASK_BTN3) == 0U)
    {
        pressed_mask |= BSP_KEYS_MASK_BTN3;
    }

    return pressed_mask;
}

void bsp_keys_record_irq_mask(uint8_t pressed_mask)
{
    uint8_t mask;
    uint8_t index;

    mask = (uint8_t)(pressed_mask & (BSP_KEYS_MASK_BTN1 | BSP_KEYS_MASK_BTN2 | BSP_KEYS_MASK_BTN3));
    g_key_irq_mask |= mask;
    g_key_irq_seen_mask |= mask;

    for (index = 0U; index < BSP_KEYS_COUNT; ++index)
    {
        if (((mask >> index) & 0x01U) != 0U)
        {
            ++g_key_irq_count[index];
        }
    }
}

uint8_t bsp_keys_consume_irq_mask(void)
{
    uint8_t pressed_mask;

#if defined(__riscv)
    __disable_irq();
#endif
    pressed_mask = g_key_irq_mask;
    g_key_irq_mask = 0U;
#if defined(__riscv)
    __enable_irq();
#endif

    return pressed_mask;
}

static uint8_t bsp_keys_read_raw_high_mask(void)
{
#if defined(__riscv)
#if PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    uint8_t raw_high_mask;

    raw_high_mask = 0U;
    if (GPIO_ReadInputDataBit(PX1_KEY1_GPIO, PX1_KEY1_PIN) != Bit_RESET)
    {
        raw_high_mask |= BSP_KEYS_MASK_BTN1;
    }

    if (GPIO_ReadInputDataBit(PX1_KEY2_GPIO, PX1_KEY2_PIN) != Bit_RESET)
    {
        raw_high_mask |= BSP_KEYS_MASK_BTN2;
    }

    if (GPIO_ReadInputDataBit(PX1_KEY3_GPIO, PX1_KEY3_PIN) != Bit_RESET)
    {
        raw_high_mask |= BSP_KEYS_MASK_BTN3;
    }

    return raw_high_mask;
#else
    return 0U;
#endif
#elif defined(PX1_HOST_TEST)
    return g_key_mock_raw_high_mask;
#else
#error "bsp_keys requires __riscv for product firmware or PX1_HOST_TEST for host tests."
#endif
}

static uint8_t bsp_keys_read_mask(void)
{
    return bsp_keys_active_mask_from_levels(bsp_keys_read_raw_high_mask());
}

uint8_t bsp_keys_boot_probe_btn3(void)
{
#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    uint8_t pressed;
    GPIO_InitTypeDef gpio_init = { 0 };

    RCC_PB2PeriphClockCmd(PX1_KEY1_GPIO_CLOCK |
                          PX1_KEY2_GPIO_CLOCK |
                          PX1_KEY3_GPIO_CLOCK |
                          RCC_PB2Periph_AFIO,
                          ENABLE);
    GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    gpio_init.GPIO_Pin = PX1_KEY1_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(PX1_KEY1_GPIO, &gpio_init);

    gpio_init.GPIO_Pin = PX1_KEY2_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_Init(PX1_KEY2_GPIO, &gpio_init);

    gpio_init.GPIO_Pin = PX1_KEY3_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(PX1_KEY3_GPIO, &gpio_init);

    pressed = (uint8_t)((bsp_keys_read_mask() & BSP_KEYS_MASK_BTN3) != 0U);
    if (pressed == 0U)
    {
        return 0U;
    }

    RCC_PB2PeriphClockCmd(PX1_CC1_EXT_RD_CTL_GPIO_CLOCK, ENABLE);
    gpio_init.GPIO_Pin = PX1_CC1_EXT_RD_CTL_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PX1_CC1_EXT_RD_CTL_GPIO, &gpio_init);
    GPIO_SetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    {
        volatile uint32_t guard;

        for (guard = 0U; guard < 1000U; ++guard)
        {
        }
    }

    pressed = (uint8_t)((bsp_keys_read_mask() & BSP_KEYS_MASK_BTN3) != 0U);
    if (pressed != 0U)
    {
        GPIO_ResetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
    return pressed;
#elif defined(PX1_HOST_TEST)
    return (uint8_t)((bsp_keys_read_mask() & BSP_KEYS_MASK_BTN3) != 0U);
#else
    return 0U;
#endif
}

#if (defined(PX1_ENABLE_KEY_DEBUG_STATE) && (PX1_ENABLE_KEY_DEBUG_STATE != 0)) || defined(PX1_HOST_TEST)
void bsp_keys_get_debug_state(bsp_keys_debug_t *state)
{
    if (state == 0)
    {
        return;
    }

    memset(state, 0, sizeof(*state));
    state->raw_high_mask = bsp_keys_read_raw_high_mask();
    state->active_mask = bsp_keys_active_mask_from_levels(state->raw_high_mask);

#if defined(__riscv)
    __disable_irq();
#endif
    state->pending_irq_mask = g_key_irq_mask;
    state->seen_irq_mask = g_key_irq_seen_mask;
    state->irq_count_btn1 = g_key_irq_count[0];
    state->irq_count_btn2 = g_key_irq_count[1];
    state->irq_count_btn3 = g_key_irq_count[2];
#if defined(__riscv)
    __enable_irq();
#endif
}
#endif

static void bsp_keys_emit_short_event(bsp_keys_event_t *event, uint8_t index)
{
    switch (index)
    {
        case 0U:
            event->btn1_short = 1U;
            break;
        case 1U:
            event->btn2_short = 1U;
            break;
        case 2U:
            event->btn3_short = 1U;
            break;
        default:
            break;
    }
}

static void bsp_keys_emit_long_event(bsp_keys_event_t *event, uint8_t index)
{
    switch (index)
    {
        case 0U:
            event->btn1_long = 1U;
            break;
        case 1U:
            event->btn2_long = 1U;
            break;
        case 2U:
            event->btn3_long = 1U;
            break;
        default:
            break;
    }
}

#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
static void bsp_keys_config_exti_line(uint8_t port_source,
                                      uint8_t pin_source,
                                      uint32_t line,
                                      EXTITrigger_TypeDef trigger,
                                      uint8_t irq_channel,
                                      uint8_t sub_priority)
{
    EXTI_InitTypeDef exti_init = { 0 };
    NVIC_InitTypeDef nvic_init = { 0 };

    GPIO_EXTILineConfig(port_source, pin_source);

    exti_init.EXTI_Line = line;
    exti_init.EXTI_Mode = EXTI_Mode_Interrupt;
    exti_init.EXTI_Trigger = trigger;
    exti_init.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti_init);
    EXTI_ClearITPendingBit(line);

    nvic_init.NVIC_IRQChannel = irq_channel;
    nvic_init.NVIC_IRQChannelPreemptionPriority = 1U;
    nvic_init.NVIC_IRQChannelSubPriority = sub_priority;
    nvic_init.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_init);
}
#endif

void bsp_keys_init(void)
{
    memset(g_key_state, 0, sizeof(g_key_state));
    g_key_irq_mask = 0U;
    g_key_irq_seen_mask = 0U;
    memset((void *)g_key_irq_count, 0, sizeof(g_key_irq_count));
#if defined(PX1_HOST_TEST)
    g_key_mock_raw_high_mask = BSP_KEYS_MASK_BTN1 | BSP_KEYS_MASK_BTN3;
#endif

#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    {
        GPIO_InitTypeDef gpio_init = { 0 };

        RCC_PB2PeriphClockCmd(PX1_KEY1_GPIO_CLOCK | PX1_KEY3_GPIO_CLOCK | RCC_PB2Periph_AFIO, ENABLE);
        GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);

        gpio_init.GPIO_Pin = PX1_KEY1_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_IPU;
        gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PX1_KEY1_GPIO, &gpio_init);

        gpio_init.GPIO_Pin = PX1_KEY2_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_IPD;
        GPIO_Init(PX1_KEY2_GPIO, &gpio_init);

        gpio_init.GPIO_Pin = PX1_KEY3_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_IPU;
        GPIO_Init(PX1_KEY3_GPIO, &gpio_init);

        bsp_keys_config_exti_line(GPIO_PortSourceGPIOB,
                                  GPIO_PinSource9,
                                  EXTI_Line9,
                                  EXTI_Trigger_Rising,
                                  EXTI9_5_IRQn,
                                  2U);
        /*
         * BTN1 is PB15 and BTN3 is PA15, so both share EXTI line 15. The AFIO
         * line source can only point at one port at a time; keep BTN3 on EXTI
         * and let the 20 ms scanner cover BTN1.
         */
        bsp_keys_config_exti_line(GPIO_PortSourceGPIOA,
                                  GPIO_PinSource15,
                                  EXTI_Line15,
                                  EXTI_Trigger_Falling,
                                  EXTI15_10_IRQn,
                                  3U);
    }
#endif
}

#if defined(PX1_HOST_TEST)
void bsp_keys_mock_set_raw_high_mask(uint8_t raw_high_mask)
{
    g_key_mock_raw_high_mask = raw_high_mask;
}
#endif

void bsp_keys_exti9_5_irq_handler(void)
{
#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    if (EXTI_GetITStatus(EXTI_Line9) != RESET)
    {
        bsp_keys_record_irq_mask(BSP_KEYS_MASK_BTN2);
        EXTI_ClearITPendingBit(EXTI_Line9);
    }
#endif
}

void bsp_keys_exti15_10_irq_handler(void)
{
#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    if (EXTI_GetITStatus(EXTI_Line15) != RESET)
    {
        bsp_keys_record_irq_mask(BSP_KEYS_MASK_BTN3);
        EXTI_ClearITPendingBit(EXTI_Line15);
    }
#endif
}

void bsp_keys_poll(bsp_keys_event_t *event)
{
    uint8_t pressed_mask;
    uint8_t irq_mask;
    uint8_t index;

    if (event == 0)
    {
        return;
    }

    memset(event, 0, sizeof(*event));
    irq_mask = bsp_keys_consume_irq_mask();
    pressed_mask = bsp_keys_read_mask();

    for (index = 0U; index < BSP_KEYS_COUNT; ++index)
    {
        bsp_key_state_t *state;
        uint8_t level;
        uint8_t irq_level;

        state = &g_key_state[index];
        level = (pressed_mask >> index) & 0x01U;
        irq_level = (irq_mask >> index) & 0x01U;

        if (irq_level != 0U)
        {
            state->debounce_ticks = 0U;
            if (level != 0U)
            {
                state->stable_level = 1U;
                state->hold_ticks = 0U;
                state->short_reported = 0U;
                state->long_reported = 0U;
            }
            else
            {
                bsp_keys_emit_short_event(event, index);
                state->stable_level = 0U;
                state->hold_ticks = 0U;
                state->short_reported = 0U;
                state->long_reported = 0U;
            }
            continue;
        }

        if (level == state->stable_level)
        {
            state->debounce_ticks = 0U;
        }
        else if (++state->debounce_ticks >= BSP_KEYS_DEBOUNCE_TICKS)
        {
            state->debounce_ticks = 0U;
            if ((state->stable_level != 0U) &&
                (level == 0U) &&
                (state->short_reported == 0U) &&
                (state->long_reported == 0U))
            {
                bsp_keys_emit_short_event(event, index);
                state->short_reported = 1U;
            }

            state->stable_level = level;
            state->hold_ticks = 0U;
            state->short_reported = 0U;
            state->long_reported = 0U;
        }

        if (state->stable_level != 0U)
        {
            if (state->hold_ticks < 0xFFFFU)
            {
                ++state->hold_ticks;
            }

            if ((state->long_reported == 0U) && (state->hold_ticks >= BSP_KEYS_LONG_PRESS_TICKS))
            {
                state->long_reported = 1U;
                bsp_keys_emit_long_event(event, index);
            }
        }
    }
}

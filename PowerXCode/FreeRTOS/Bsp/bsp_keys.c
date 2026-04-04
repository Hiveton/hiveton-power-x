#include "bsp_keys.h"

#include <string.h>

#include "bsp_board_config.h"

#if defined(__riscv)
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#endif

typedef struct
{
    uint8_t stable_level;
    uint8_t debounce_ticks;
    uint16_t hold_ticks;
    uint8_t long_reported;
} bsp_key_state_t;

#define BSP_KEYS_COUNT 3U
#define BSP_KEYS_DEBOUNCE_TICKS 2U
#define BSP_KEYS_LONG_PRESS_TICKS 12U

static bsp_key_state_t g_key_state[BSP_KEYS_COUNT];

static uint8_t bsp_keys_read_mask(void)
{
#if defined(__riscv)
#if PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    uint8_t mask;

    mask = 0U;
    if (GPIO_ReadInputDataBit(PX1_KEY1_GPIO, PX1_KEY1_PIN) == Bit_RESET)
    {
        mask |= 0x01U;
    }

    if (GPIO_ReadInputDataBit(PX1_KEY2_GPIO, PX1_KEY2_PIN) == Bit_RESET)
    {
        mask |= 0x02U;
    }

    if (GPIO_ReadInputDataBit(PX1_KEY3_GPIO, PX1_KEY3_PIN) == Bit_RESET)
    {
        mask |= 0x04U;
    }

    return mask;
#else
    return 0U;
#endif
#else
    return 0U;
#endif
}

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

void bsp_keys_init(void)
{
    memset(g_key_state, 0, sizeof(g_key_state));

#if defined(__riscv) && PX1_BOARD_HAS_CONFIRMED_KEY_PINS
    {
        GPIO_InitTypeDef gpio_init = { 0 };

        RCC_PB2PeriphClockCmd(PX1_KEY1_GPIO_CLOCK | PX1_KEY3_GPIO_CLOCK, ENABLE);

        gpio_init.GPIO_Pin = PX1_KEY1_PIN | PX1_KEY2_PIN;
        gpio_init.GPIO_Mode = GPIO_Mode_IPU;
        gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
        GPIO_Init(PX1_KEY1_GPIO, &gpio_init);

        gpio_init.GPIO_Pin = PX1_KEY3_PIN;
        GPIO_Init(PX1_KEY3_GPIO, &gpio_init);
    }
#endif
}

void bsp_keys_poll(bsp_keys_event_t *event)
{
    uint8_t pressed_mask;
    uint8_t index;

    if (event == 0)
    {
        return;
    }

    memset(event, 0, sizeof(*event));
    pressed_mask = bsp_keys_read_mask();

    for (index = 0U; index < BSP_KEYS_COUNT; ++index)
    {
        bsp_key_state_t *state;
        uint8_t level;

        state = &g_key_state[index];
        level = (pressed_mask >> index) & 0x01U;

        if (level == state->stable_level)
        {
            state->debounce_ticks = 0U;
        }
        else if (++state->debounce_ticks >= BSP_KEYS_DEBOUNCE_TICKS)
        {
            state->debounce_ticks = 0U;
            state->stable_level = level;

            if (state->stable_level != 0U)
            {
                state->hold_ticks = 0U;
                state->long_reported = 0U;
            }
            else if (state->long_reported == 0U)
            {
                bsp_keys_emit_short_event(event, index);
            }
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

#ifndef BSP_KEYS_H
#define BSP_KEYS_H

#include <stdint.h>

typedef struct
{
    uint8_t btn1_short;
    uint8_t btn2_short;
    uint8_t btn3_short;
    uint8_t btn1_long;
    uint8_t btn2_long;
    uint8_t btn3_long;
} bsp_keys_event_t;

#if (defined(PX1_ENABLE_KEY_DEBUG_STATE) && (PX1_ENABLE_KEY_DEBUG_STATE != 0)) || defined(PX1_HOST_TEST)
typedef struct
{
    uint8_t raw_high_mask;
    uint8_t active_mask;
    uint8_t pending_irq_mask;
    uint8_t seen_irq_mask;
    uint16_t irq_count_btn1;
    uint16_t irq_count_btn2;
    uint16_t irq_count_btn3;
} bsp_keys_debug_t;
#endif

void bsp_keys_init(void);
void bsp_keys_poll(bsp_keys_event_t *event);
uint8_t bsp_keys_active_mask_from_levels(uint8_t raw_high_mask);
void bsp_keys_record_irq_mask(uint8_t pressed_mask);
uint8_t bsp_keys_consume_irq_mask(void);
#if (defined(PX1_ENABLE_KEY_DEBUG_STATE) && (PX1_ENABLE_KEY_DEBUG_STATE != 0)) || defined(PX1_HOST_TEST)
void bsp_keys_get_debug_state(bsp_keys_debug_t *state);
#endif
void bsp_keys_exti9_5_irq_handler(void);
void bsp_keys_exti15_10_irq_handler(void);
#if defined(PX1_HOST_TEST)
void bsp_keys_mock_set_raw_high_mask(uint8_t raw_high_mask);
#endif

#endif /* BSP_KEYS_H */

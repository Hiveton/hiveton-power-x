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

void bsp_keys_init(void);
void bsp_keys_poll(bsp_keys_event_t *event);

#endif /* BSP_KEYS_H */

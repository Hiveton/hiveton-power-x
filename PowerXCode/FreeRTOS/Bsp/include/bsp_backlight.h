#ifndef BSP_BACKLIGHT_H
#define BSP_BACKLIGHT_H

#include <stdint.h>

void bsp_backlight_init(void);
void bsp_backlight_set(uint8_t percent);
uint8_t bsp_backlight_get_percent(void);

#endif /* BSP_BACKLIGHT_H */

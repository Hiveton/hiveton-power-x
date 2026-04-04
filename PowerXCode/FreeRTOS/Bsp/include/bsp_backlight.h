#ifndef BSP_BACKLIGHT_H
#define BSP_BACKLIGHT_H

#include <stdint.h>

void bsp_backlight_init(void);
void bsp_backlight_set(uint8_t percent);

#endif /* BSP_BACKLIGHT_H */

#include "bsp_board.h"

#include "bsp_backlight.h"
#include "bsp_lcd_st7735.h"

void bsp_board_init(void)
{
    bsp_backlight_init();
    bsp_backlight_set(100U);
    bsp_lcd_init();
}

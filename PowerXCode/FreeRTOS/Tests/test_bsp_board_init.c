#include <assert.h>

#include "bsp_backlight.h"
#include "bsp_board.h"
#include "bsp_lcd_st7735.h"

int main(void)
{
    bsp_lcd_window_info_t window;

    bsp_backlight_init();
    bsp_backlight_set(0U);
    bsp_lcd_set_window(12U, 10U, 20U, 15U);
    bsp_lcd_set_rotation(180U);

    bsp_board_init();

    assert(bsp_backlight_get_percent() == 0U);
    assert(bsp_lcd_get_rotation_degrees() == 0U);

    bsp_lcd_get_window(&window);
    assert(window.x == 0U);
    assert(window.y == 0U);
    assert(window.width == LCD_WIDTH);
    assert(window.height == LCD_HEIGHT);

    return 0;
}

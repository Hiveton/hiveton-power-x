#include <assert.h>

#include "bsp_lcd_st7735.h"

int main(void)
{
    bsp_lcd_window_info_t window;

    bsp_lcd_init();
    bsp_lcd_get_window(&window);
    assert(bsp_lcd_get_rotation_degrees() == 0U);
    assert(window.x == 0U);
    assert(window.y == 0U);
    assert(window.width == LCD_WIDTH);
    assert(window.height == LCD_HEIGHT);

    bsp_lcd_set_window(150U, 70U, 30U, 20U);
    bsp_lcd_get_window(&window);
    assert(window.x == 150U);
    assert(window.y == 70U);
    assert(window.width == 10U);
    assert(window.height == 10U);

    bsp_lcd_set_rotation(180U);
    bsp_lcd_get_window(&window);
    assert(bsp_lcd_get_rotation_degrees() == 180U);
    assert(window.x == 150U);
    assert(window.y == 70U);
    assert(window.width == 10U);
    assert(window.height == 10U);

    bsp_lcd_set_rotation(90U);
    assert(bsp_lcd_get_rotation_degrees() == 0U);

    return 0;
}

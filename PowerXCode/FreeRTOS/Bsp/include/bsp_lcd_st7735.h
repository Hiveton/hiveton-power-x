#ifndef BSP_LCD_ST7735_H
#define BSP_LCD_ST7735_H

#include <stdint.h>

#define LCD_WIDTH 160U
#define LCD_HEIGHT 80U

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} bsp_lcd_window_info_t;

void bsp_lcd_init(void);
void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void bsp_lcd_get_window(bsp_lcd_window_info_t *window);
void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count);
void bsp_lcd_fill_color(uint16_t color);
void bsp_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color);
void bsp_lcd_draw_test_pattern(void);
void bsp_lcd_set_rotation(uint16_t degrees);
uint16_t bsp_lcd_get_rotation_degrees(void);

#endif /* BSP_LCD_ST7735_H */

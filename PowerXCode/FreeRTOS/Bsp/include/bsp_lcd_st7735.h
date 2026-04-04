#ifndef BSP_LCD_ST7735_H
#define BSP_LCD_ST7735_H

#include <stdint.h>

#define LCD_WIDTH 160U
#define LCD_HEIGHT 80U

void bsp_lcd_init(void);
void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count);

#endif /* BSP_LCD_ST7735_H */

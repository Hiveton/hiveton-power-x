#include "ui_renderer.h"

#include <stdint.h>

#include "bsp_backlight.h"
#include "bsp_lcd_st7735.h"
#include "ui_widgets.h"

static uint16_t g_line_buffer[LCD_WIDTH];

static void ui_renderer_push_line(uint16_t y, uint16_t background, uint16_t accent, uint16_t fill_width)
{
    uint16_t x;
    uint16_t width;

    width = LCD_WIDTH;
    ui_widgets_fill_line(g_line_buffer, width, background);

    if (fill_width > width)
    {
        fill_width = width;
    }

    for (x = 0U; x < fill_width; ++x)
    {
        g_line_buffer[x] = accent;
    }

    bsp_lcd_set_window(0U, y, width, 1U);
    bsp_lcd_push_pixels(g_line_buffer, width);
}

static void ui_renderer_push_placeholder_line(uint16_t y,
                                              uint16_t background,
                                              uint16_t accent,
                                              uint16_t segment_width,
                                              uint16_t gap_width)
{
    uint16_t width;
    uint16_t x;

    width = LCD_WIDTH;
    ui_widgets_fill_line(g_line_buffer, width, background);

    if (segment_width == 0U)
    {
        segment_width = 12U;
    }

    for (x = 0U; x < width; x = (uint16_t)(x + segment_width + gap_width))
    {
        uint16_t end;
        uint16_t fill_x;

        end = (uint16_t)(x + segment_width);
        if (end > width)
        {
            end = width;
        }

        for (fill_x = x; fill_x < end; ++fill_x)
        {
            g_line_buffer[fill_x] = accent;
        }
    }

    bsp_lcd_set_window(0U, y, width, 1U);
    bsp_lcd_push_pixels(g_line_buffer, width);
}

void ui_renderer_init(void)
{
    bsp_lcd_init();
    bsp_backlight_init();
    bsp_backlight_set(80U);
}

void ui_renderer_draw_boot_screen(void)
{
    uint16_t y;

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        ui_renderer_push_line(y, 0U, 0U, 0U);
    }
}

void ui_renderer_draw_placeholder_page(ui_page_t page)
{
    uint16_t y;
    uint16_t accent;

    switch (page)
    {
        case UI_PAGE_PROTOCOL:
            accent = 0x07E0U;
            break;
        case UI_PAGE_TRIGGER:
            accent = 0xFFE0U;
            break;
        case UI_PAGE_STATS:
            accent = 0xF81FU;
            break;
        case UI_PAGE_MAIN:
        default:
            accent = 0x4208U;
            break;
    }

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        if (y < 8U)
        {
            ui_renderer_push_line(y, 0x0000U, accent, LCD_WIDTH);
        }
        else
        {
            ui_renderer_push_line(y, 0x0000U, 0x0000U, 0U);
        }
    }
}

void ui_renderer_draw_main_page(const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    uint16_t protocol_color;
    uint16_t voltage_fill;
    uint16_t current_fill;
    uint16_t power_fill;
    uint16_t voltage_color;
    uint16_t current_color;
    uint16_t power_color;
    uint16_t y;

    protocol_color = ui_widgets_protocol_color((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE,
                                               (protocol != NULL) ? protocol->emark_present : 0U);
    voltage_fill = ui_widgets_scale_u16((measure != NULL) ? measure->voltage_avg_mv : 0,
                                        0,
                                        20000,
                                        LCD_WIDTH);
    current_fill = ui_widgets_scale_u16((measure != NULL) ? measure->current_avg_ma : 0,
                                        0,
                                        5000,
                                        LCD_WIDTH);
    power_fill = ui_widgets_scale_u16((measure != NULL) ? measure->power_mw : 0,
                                      0,
                                      40000,
                                      LCD_WIDTH);
    voltage_color = ((measure != NULL) && (measure->voltage_valid != 0U)) ? 0x07E0U : 0x4208U;
    current_color = ((measure != NULL) && (measure->current_valid != 0U)) ? 0xFFE0U : 0x4208U;
    power_color = ((measure != NULL) && (measure->power_valid != 0U)) ? 0xF800U : 0x4208U;

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        if (y < 16U)
        {
            ui_renderer_push_line(y, 0x0841U, protocol_color, LCD_WIDTH);
        }
        else if (y < 36U)
        {
            if ((measure != NULL) && (measure->voltage_valid != 0U))
            {
                ui_renderer_push_line(y,
                                      0x0000U,
                                      voltage_color,
                                      voltage_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y,
                                                  0x0000U,
                                                  voltage_color,
                                                  18U,
                                                  10U);
            }
        }
        else if (y < 56U)
        {
            if ((measure != NULL) && (measure->current_valid != 0U))
            {
                ui_renderer_push_line(y,
                                      0x0000U,
                                      current_color,
                                      current_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y,
                                                  0x0000U,
                                                  current_color,
                                                  12U,
                                                  8U);
            }
        }
        else
        {
            if ((measure != NULL) && (measure->power_valid != 0U))
            {
                ui_renderer_push_line(y,
                                      0x0000U,
                                      power_color,
                                      power_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y,
                                                  0x0000U,
                                                  power_color,
                                                  8U,
                                                  6U);
            }
        }
    }
}

void ui_renderer_draw_protocol_page(const protocol_snapshot_t *protocol)
{
    uint16_t protocol_color;
    uint16_t voltage_fill;
    uint16_t current_fill;
    uint16_t step_fill;
    uint16_t step_color;
    uint16_t y;
    int32_t step_abs;

    protocol_color = ui_widgets_protocol_color((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE,
                                               (protocol != NULL) ? protocol->emark_present : 0U);
    voltage_fill = ui_widgets_scale_u16((protocol != NULL) ? protocol->contract_mv : 0,
                                        0,
                                        20000,
                                        LCD_WIDTH);
    current_fill = ui_widgets_scale_u16((protocol != NULL) ? protocol->contract_ma : 0,
                                        0,
                                        5000,
                                        LCD_WIDTH);
    step_abs = (protocol != NULL) ? protocol->legacy_step_offset : 0;
    if (step_abs < 0)
    {
        step_abs = -step_abs;
    }
    step_fill = ui_widgets_scale_u16(step_abs, 0, 20, LCD_WIDTH);
    step_color = ((protocol != NULL) && (protocol->legacy_step_offset >= 0)) ? 0x07FFU : 0xF81FU;

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        if (y < 16U)
        {
            ui_renderer_push_line(y, 0x0000U, protocol_color, LCD_WIDTH);
        }
        else if (y < 32U)
        {
            if ((protocol != NULL) && (protocol->kind != PROTOCOL_KIND_NONE))
            {
                ui_renderer_push_line(y, 0x0000U, 0x07E0U, voltage_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y, 0x0000U, 0x4208U, 18U, 10U);
            }
        }
        else if (y < 48U)
        {
            if ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_PD) && (protocol->contract_ma > 0))
            {
                ui_renderer_push_line(y, 0x0000U, 0xFFE0U, current_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y, 0x0000U, 0x3186U, 10U, 8U);
            }
        }
        else if (y < 64U)
        {
            if ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_QC))
            {
                ui_renderer_push_line(y, 0x0000U, step_color, step_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y, 0x0000U, 0x2104U, 6U, 6U);
            }
        }
        else
        {
            if ((protocol != NULL) && (protocol->emark_present != 0U))
            {
                ui_renderer_push_line(y, 0x0000U, 0x05A0U, LCD_WIDTH);
            }
            else
            {
                ui_renderer_push_line(y, 0x0000U, 0x0000U, 0U);
            }
        }
    }
}

void ui_renderer_draw_trigger_page(const ui_model_state_t *state,
                                   const protocol_snapshot_t *protocol)
{
    uint16_t protocol_color;
    uint16_t selected_fill;
    uint16_t actual_fill;
    uint16_t step_fill;
    uint16_t request_color;
    uint16_t y;
    int32_t step_abs;
    uint8_t has_request;
    int32_t selected_mv;

    protocol_color = ui_widgets_protocol_color((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE,
                                               (protocol != NULL) ? protocol->emark_present : 0U);
    selected_mv = ui_model_trigger_selected_mv(state);
    selected_fill = ui_widgets_scale_u16(selected_mv,
                                         0,
                                         20000,
                                         LCD_WIDTH);
    actual_fill = ui_widgets_scale_u16((protocol != NULL) ? protocol->contract_mv : 0,
                                        0,
                                        20000,
                                        LCD_WIDTH);
    step_abs = (protocol != NULL) ? protocol->legacy_step_offset : 0;
    if (step_abs < 0)
    {
        step_abs = -step_abs;
    }
    step_fill = ui_widgets_scale_u16(step_abs, 0, 20, LCD_WIDTH);
    has_request = ((protocol != NULL) && (protocol->kind != PROTOCOL_KIND_NONE)) ? 1U : 0U;
    request_color = ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_PD)) ? 0x07E0U : 0xFFE0U;

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        if (y < 16U)
        {
            ui_renderer_push_line(y, 0x0000U, protocol_color, LCD_WIDTH);
        }
        else if (y < 36U)
        {
            ui_renderer_push_line(y, 0x0000U, request_color, selected_fill);
        }
        else if (y < 56U)
        {
            if (has_request != 0U)
            {
                ui_renderer_push_line(y, 0x0000U, 0x07E0U, actual_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y, 0x0000U, 0x4208U, 16U, 10U);
            }
        }
        else
        {
            if ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_QC))
            {
                ui_renderer_push_line(y, 0x0000U, (protocol->emark_present != 0U) ? 0x05A0U : 0x3186U, LCD_WIDTH);
            }
            else if ((protocol != NULL) && (protocol->kind != PROTOCOL_KIND_NONE))
            {
                ui_renderer_push_line(y,
                                      0x0000U,
                                      (protocol->legacy_step_offset >= 0) ? 0x07FFU : 0xF81FU,
                                      step_fill);
            }
            else
            {
                ui_renderer_push_placeholder_line(y, 0x0000U, 0x2104U, 8U, 6U);
            }
        }
    }
}

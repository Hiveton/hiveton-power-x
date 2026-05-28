#include "ui_renderer.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include "bsp_lcd_st7735.h"
#include "bsp_usbpd_port.h"
#include "font_meter_24.h"
#include "font_mono_9.h"
#include "font_mono_12.h"
#include "font_mono_15.h"
#include "font_zh_15.h"
#include "font_zh_24.h"
#include "service_pd.h"
#include "ui_scope_model.h"
#include "ui_value_format.h"
#include "ui_widgets.h"

#define UI_COLOR_BLACK 0x0000U
#define UI_COLOR_PANEL 0x0841U
#define UI_COLOR_SCOPE_GRID 0x05F7U
#define UI_COLOR_SCOPE_VOLTAGE UI_COLOR_MAIN_AMBER
#define UI_COLOR_SCOPE_CURRENT UI_COLOR_MAIN_GREEN
#define UI_COLOR_SCOPE_POWER UI_COLOR_MAGENTA
#define UI_SCOPE_HISTORY_CAPACITY 160U
#define UI_SCOPE_GRAPH_TOP_Y 4U
#define UI_SCOPE_GRAPH_HEIGHT 62U
#define UI_COLOR_HEADER 0x0882U
#define UI_COLOR_DIM 0xFFFFU
#define UI_COLOR_TEXT 0xFFFFU
#define UI_COLOR_CYAN 0x07FFU
#define UI_COLOR_GREEN 0x07E0U
#define UI_COLOR_AMBER 0xFFE0U
#define UI_COLOR_RED 0xF800U
#define UI_COLOR_MAGENTA 0xF81FU
#define UI_COLOR_FRAME 0x1987U
#define UI_COLOR_HEADER_INSET 0x00A1U
#define UI_COLOR_MAIN_GREEN 0x2FADU
#define UI_COLOR_MAIN_AMBER 0xFE89U
#define UI_COLOR_MAIN_CYAN 0x071FU
#define UI_COLOR_MAIN_PINK 0xFD7FU
#define UI_COLOR_MAIN_PURPLE 0x8A7FU
#define UI_COLOR_DPDM_TOP_BG 0xD360U
#define UI_COLOR_DPDM_BOTTOM_BG 0x0560U
#define UI_COLOR_STATS_TOP_BG 0x301BU
#define UI_COLOR_STATS_BOTTOM_BG 0x04B1U
#define UI_COLOR_DPDM_LINE 0xFFFFU
#define UI_COLOR_DPDM_PROTOCOL 0xFFFFU
#define UI_COLOR_DPDM_ARROW 0xFFFFU
#define UI_COLOR_CC_ACTIVE 0x2FADU
#define UI_COLOR_CC_ACTIVE_DARK 0x1569U
#define UI_COLOR_CC_DIM 0x7C31U
#define UI_COLOR_CC_DARK 0x10C4U

static uint16_t g_line_buffer[LCD_WIDTH];
static uint8_t g_frame_streaming;
static uint16_t g_frame_next_row;

typedef struct
{
    int16_t voltage_mv[UI_SCOPE_HISTORY_CAPACITY];
    int16_t current_ma[UI_SCOPE_HISTORY_CAPACITY];
    uint16_t count;
    uint16_t interval_s;
    uint32_t last_sample_s;
    uint8_t initialized;
} ui_scope_history_t;

static ui_scope_history_t g_scope_history;

typedef struct
{
    int32_t voltage_mv[UI_SCOPE_HISTORY_CAPACITY];
    uint16_t count;
    uint16_t next_index;
} ui_ripple_history_t;

static ui_ripple_history_t g_ripple_history;

#if !defined(__riscv)
__attribute__((weak)) void service_pd_copy_source_caps(service_pd_source_caps_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    *snapshot = (service_pd_source_caps_snapshot_t){
        .count = 5U,
        .pdos = {
            { 5000U, 5000U, 3000U, 150U, 1U, SERVICE_PD_SOURCE_PDO_FIXED },
            { 9000U, 9000U, 3000U, 270U, 2U, SERVICE_PD_SOURCE_PDO_FIXED },
            { 12000U, 12000U, 3000U, 360U, 3U, SERVICE_PD_SOURCE_PDO_FIXED },
            { 20000U, 20000U, 3000U, 600U, 4U, SERVICE_PD_SOURCE_PDO_FIXED },
            { 3300U, 11000U, 3000U, 330U, 5U, SERVICE_PD_SOURCE_PDO_PPS },
        },
    };
}

__attribute__((weak)) void bsp_usbpd_port_copy_diag(bsp_usbpd_port_diag_t *diag)
{
    if (diag != NULL)
    {
        *diag = (bsp_usbpd_port_diag_t){ 0 };
    }
}
#endif

static uint16_t ui_draw_mono_9_on_line(uint16_t row,
                                       uint16_t x,
                                       uint16_t y,
                                       const char *text,
                                       uint16_t color);
static uint16_t ui_mono_9_text_width(const char *text);
static void ui_begin_row(uint16_t row);
static void ui_end_row(uint16_t row);
static int32_t ui_measure_voltage_mv(const measure_snapshot_t *measure);

static const uint16_t g_label_voltage[] = { 0x7535U, 0x538BU };
static const uint16_t g_label_current[] = { 0x7535U, 0x6D41U };
static const uint16_t g_label_power[] = { 0x529FU, 0x7387U };
static const uint16_t g_label_max[] = { 0x6700U, 0x5927U };
static const uint16_t g_label_average[] = { 0x5E73U, 0x5747U };
static const uint16_t g_label_stat_time[] = { 0x7EDFU, 0x8BA1U, 0x65F6U, 0x95F4U };
static const uint16_t g_label_report[] = { 0x62A5U, 0x6587U };
static const uint16_t g_label_protocol_detect[] = { 0x534FU, 0x8BAEU, 0x68C0U, 0x6D4BU };
static const uint16_t g_label_read[] = { 0x8BFBU, 0x53D6U };
static const uint16_t g_label_emulate[] = { 0x6A21U, 0x62DFU };
static const uint16_t g_label_kelvin[] = { 0x5F00U, 0x5C14U, 0x6587U, 0x7EBFU, 0x963BU };
static const uint16_t g_label_settings_zh[] = { 0x8BBEU, 0x7F6EU };
static const uint16_t g_label_edit_zh[] = { 0x7F16U, 0x8F91U };
static const uint16_t g_label_view_zh[] = { 0x67E5U, 0x770BU };
static const uint16_t g_label_brightness_zh[] = { 0x4EAEU, 0x5EA6U };
static const uint16_t g_label_rotation_zh[] = { 0x65CBU, 0x8F6CU };
static const uint16_t g_label_remove_load_zh[] = { 0x79FBU, 0x9664U, 0x8D1FU, 0x8F7DU };
static const uint16_t g_label_confirm_zh[] = { 0x786EU, 0x8BA4U };
static const uint16_t g_label_cancel_zh[] = { 0x53D6U, 0x6D88U };

#if (defined(PX1_ENABLE_KEY_DEBUG_RENDERER) && (PX1_ENABLE_KEY_DEBUG_RENDERER != 0)) || defined(PX1_HOST_TEST)
static const uint8_t *ui_ascii_glyph(char ch)
{
    static const uint8_t glyph_space[7] = { 0, 0, 0, 0, 0, 0, 0 };
    static const uint8_t glyph_unknown[7] = { 0x0EU, 0x11U, 0x01U, 0x06U, 0x04U, 0x00U, 0x04U };
    static const uint8_t glyphs[][7] = {
        { 0x0EU, 0x11U, 0x13U, 0x15U, 0x19U, 0x11U, 0x0EU },
        { 0x04U, 0x0CU, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU },
        { 0x0EU, 0x11U, 0x01U, 0x02U, 0x04U, 0x08U, 0x1FU },
        { 0x1EU, 0x01U, 0x01U, 0x0EU, 0x01U, 0x01U, 0x1EU },
        { 0x02U, 0x06U, 0x0AU, 0x12U, 0x1FU, 0x02U, 0x02U },
        { 0x1FU, 0x10U, 0x1EU, 0x01U, 0x01U, 0x11U, 0x0EU },
        { 0x06U, 0x08U, 0x10U, 0x1EU, 0x11U, 0x11U, 0x0EU },
        { 0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x08U, 0x08U },
        { 0x0EU, 0x11U, 0x11U, 0x0EU, 0x11U, 0x11U, 0x0EU },
        { 0x0EU, 0x11U, 0x11U, 0x0FU, 0x01U, 0x02U, 0x0CU },
        { 0x0EU, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U },
        { 0x1EU, 0x11U, 0x11U, 0x1EU, 0x11U, 0x11U, 0x1EU },
        { 0x0EU, 0x11U, 0x10U, 0x10U, 0x10U, 0x11U, 0x0EU },
        { 0x1EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x1EU },
        { 0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x1FU },
        { 0x1FU, 0x10U, 0x10U, 0x1EU, 0x10U, 0x10U, 0x10U },
        { 0x0EU, 0x11U, 0x10U, 0x17U, 0x11U, 0x11U, 0x0FU },
        { 0x11U, 0x11U, 0x11U, 0x1FU, 0x11U, 0x11U, 0x11U },
        { 0x0EU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x0EU },
        { 0x01U, 0x01U, 0x01U, 0x01U, 0x11U, 0x11U, 0x0EU },
        { 0x11U, 0x12U, 0x14U, 0x18U, 0x14U, 0x12U, 0x11U },
        { 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x10U, 0x1FU },
        { 0x11U, 0x1BU, 0x15U, 0x15U, 0x11U, 0x11U, 0x11U },
        { 0x11U, 0x19U, 0x15U, 0x13U, 0x11U, 0x11U, 0x11U },
        { 0x0EU, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU },
        { 0x1EU, 0x11U, 0x11U, 0x1EU, 0x10U, 0x10U, 0x10U },
        { 0x0EU, 0x11U, 0x11U, 0x11U, 0x15U, 0x12U, 0x0DU },
        { 0x1EU, 0x11U, 0x11U, 0x1EU, 0x14U, 0x12U, 0x11U },
        { 0x0FU, 0x10U, 0x10U, 0x0EU, 0x01U, 0x01U, 0x1EU },
        { 0x1FU, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U, 0x04U },
        { 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0EU },
        { 0x11U, 0x11U, 0x11U, 0x11U, 0x11U, 0x0AU, 0x04U },
        { 0x11U, 0x11U, 0x11U, 0x15U, 0x15U, 0x15U, 0x0AU },
        { 0x11U, 0x11U, 0x0AU, 0x04U, 0x0AU, 0x11U, 0x11U },
        { 0x11U, 0x11U, 0x0AU, 0x04U, 0x04U, 0x04U, 0x04U },
        { 0x1FU, 0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x1FU },
    };
    static const uint8_t glyph_dot[7] = { 0, 0, 0, 0, 0, 0x0CU, 0x0CU };
    static const uint8_t glyph_dash[7] = { 0, 0, 0, 0x1FU, 0, 0, 0 };
    static const uint8_t glyph_slash[7] = { 0x01U, 0x02U, 0x02U, 0x04U, 0x08U, 0x08U, 0x10U };
    static const uint8_t glyph_percent[7] = { 0x19U, 0x1AU, 0x02U, 0x04U, 0x08U, 0x0BU, 0x13U };
    static const uint8_t glyph_colon[7] = { 0, 0x0CU, 0x0CU, 0, 0x0CU, 0x0CU, 0 };
    static const uint8_t glyph_lt[7] = { 0x02U, 0x04U, 0x08U, 0x10U, 0x08U, 0x04U, 0x02U };
    static const uint8_t glyph_gt[7] = { 0x08U, 0x04U, 0x02U, 0x01U, 0x02U, 0x04U, 0x08U };

    if (ch == ' ')
    {
        return glyph_space;
    }
    if (ch == '.')
    {
        return glyph_dot;
    }
    if (ch == '-')
    {
        return glyph_dash;
    }
    if (ch == '/')
    {
        return glyph_slash;
    }
    if (ch == '%')
    {
        return glyph_percent;
    }
    if (ch == ':')
    {
        return glyph_colon;
    }
    if (ch == '<')
    {
        return glyph_lt;
    }
    if (ch == '>')
    {
        return glyph_gt;
    }
    if ((ch >= '0') && (ch <= '9'))
    {
        return glyphs[ch - '0'];
    }
    if ((ch >= 'A') && (ch <= 'Z'))
    {
        return glyphs[10 + ch - 'A'];
    }
    return glyph_unknown;
}
#endif

static const font_zh_15_glyph_t *ui_zh_15_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_ZH_15_GLYPH_COUNT; ++index)
    {
        if (g_font_zh_15_glyphs[index].codepoint == codepoint)
        {
            return &g_font_zh_15_glyphs[index];
        }
    }

    return NULL;
}

static const font_meter_24_glyph_t *ui_meter_24_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_METER_24_GLYPH_COUNT; ++index)
    {
        if (g_font_meter_24_glyphs[index].codepoint == codepoint)
        {
            return &g_font_meter_24_glyphs[index];
        }
    }

    return NULL;
}

static const font_mono_15_glyph_t *ui_mono_15_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_MONO_15_GLYPH_COUNT; ++index)
    {
        if (g_font_mono_15_glyphs[index].codepoint == codepoint)
        {
            return &g_font_mono_15_glyphs[index];
        }
    }

    return ui_mono_15_glyph((uint16_t)' ');
}

static const font_mono_12_glyph_t *ui_mono_12_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_MONO_12_GLYPH_COUNT; ++index)
    {
        if (g_font_mono_12_glyphs[index].codepoint == codepoint)
        {
            return &g_font_mono_12_glyphs[index];
        }
    }

    return ui_mono_12_glyph((uint16_t)'0');
}

static const font_mono_9_glyph_t *ui_mono_9_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_MONO_9_GLYPH_COUNT; ++index)
    {
        if (g_font_mono_9_glyphs[index].codepoint == codepoint)
        {
            return &g_font_mono_9_glyphs[index];
        }
    }

    return ui_mono_9_glyph((uint16_t)'0');
}

static const font_zh_24_glyph_t *ui_zh_24_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_ZH_24_GLYPH_COUNT; ++index)
    {
        if (g_font_zh_24_glyphs[index].codepoint == codepoint)
        {
            return &g_font_zh_24_glyphs[index];
        }
    }

    return NULL;
}

static void ui_clear_line(uint16_t color)
{
    uint16_t x;

    for (x = 0U; x < LCD_WIDTH; ++x)
    {
        g_line_buffer[x] = color;
    }
}

static void ui_flush_line(uint16_t y)
{
    if ((g_frame_streaming != 0U) && (y == g_frame_next_row))
    {
        bsp_lcd_push_pixels(g_line_buffer, LCD_WIDTH);
        g_frame_next_row++;
        if (g_frame_next_row >= LCD_HEIGHT)
        {
            g_frame_streaming = 0U;
        }
        return;
    }

    bsp_lcd_set_window(0U, y, LCD_WIDTH, 1U);
    bsp_lcd_push_pixels(g_line_buffer, LCD_WIDTH);
}

static void ui_put_pixel(uint16_t x, uint16_t color)
{
    if (x < LCD_WIDTH)
    {
        g_line_buffer[x] = color;
    }
}

static uint16_t ui_blend_rgb565(uint16_t bg, uint16_t fg, uint8_t alpha)
{
    uint16_t rb;
    uint16_t gb;
    uint16_t bb;
    uint16_t rf;
    uint16_t gf;
    uint16_t bf;

    if (alpha == 0U)
    {
        return bg;
    }
    if (alpha >= 15U)
    {
        return fg;
    }

    rb = (uint16_t)((bg >> 11) & 0x1FU);
    gb = (uint16_t)((bg >> 5) & 0x3FU);
    bb = (uint16_t)(bg & 0x1FU);
    rf = (uint16_t)((fg >> 11) & 0x1FU);
    gf = (uint16_t)((fg >> 5) & 0x3FU);
    bf = (uint16_t)(fg & 0x1FU);

    rb = (uint16_t)((rb * (15U - alpha) + rf * alpha + 7U) / 15U);
    gb = (uint16_t)((gb * (15U - alpha) + gf * alpha + 7U) / 15U);
    bb = (uint16_t)((bb * (15U - alpha) + bf * alpha + 7U) / 15U);
    return (uint16_t)((rb << 11) | (gb << 5) | bb);
}

static void ui_draw_rect_on_line(uint16_t row,
                                 uint16_t x,
                                 uint16_t y,
                                 uint16_t width,
                                 uint16_t height,
                                 uint16_t color)
{
    uint16_t col;

    if ((row < y) || (row >= (uint16_t)(y + height)) || (x >= LCD_WIDTH))
    {
        return;
    }

    if ((uint32_t)x + width > LCD_WIDTH)
    {
        width = LCD_WIDTH - x;
    }

    for (col = x; col < (uint16_t)(x + width); ++col)
    {
        g_line_buffer[col] = color;
    }
}

static void ui_draw_box_on_line(uint16_t row,
                                uint16_t x,
                                uint16_t y,
                                uint16_t width,
                                uint16_t height,
                                uint16_t color)
{
    if ((width < 2U) || (height < 2U))
    {
        ui_draw_rect_on_line(row, x, y, width, height, color);
        return;
    }

    ui_draw_rect_on_line(row, x, y, width, 1U, color);
    ui_draw_rect_on_line(row, x, (uint16_t)(y + height - 1U), width, 1U, color);
    ui_draw_rect_on_line(row, x, y, 1U, height, color);
    ui_draw_rect_on_line(row, (uint16_t)(x + width - 1U), y, 1U, height, color);
}

static void ui_draw_frame_on_line(uint16_t row, uint16_t color)
{
    ui_draw_rect_on_line(row, 0U, 0U, LCD_WIDTH, 1U, color);
    ui_draw_rect_on_line(row, 0U, (uint16_t)(LCD_HEIGHT - 1U), LCD_WIDTH, 1U, color);
    ui_draw_rect_on_line(row, 0U, 0U, 1U, LCD_HEIGHT, color);
    ui_draw_rect_on_line(row, (uint16_t)(LCD_WIDTH - 1U), 0U, 1U, LCD_HEIGHT, color);
}

#if (defined(PX1_ENABLE_KEY_DEBUG_RENDERER) && (PX1_ENABLE_KEY_DEBUG_RENDERER != 0)) || defined(PX1_HOST_TEST)
static void ui_draw_ascii_on_line(uint16_t row,
                                  uint16_t x,
                                  uint16_t y,
                                  const char *text,
                                  uint8_t scale,
                                  uint16_t color);
#endif
static void ui_draw_zh_15_on_line(uint16_t row,
                                  uint16_t x,
                                  uint16_t y,
                                  const uint16_t *codes,
                                  uint8_t count,
                                  uint16_t color);
static uint16_t ui_draw_mono_12_on_line(uint16_t row,
                                        uint16_t x,
                                        uint16_t y,
                                        const char *text,
                                        uint16_t color);
static uint16_t ui_draw_mono_15_baseline_on_line(uint16_t row,
                                                 uint16_t x,
                                                 uint16_t y,
                                                 const char *text,
                                                 uint16_t color);
static uint16_t ui_draw_meter_text_on_line(uint16_t row,
                                           uint16_t x,
                                           uint16_t y,
                                           const char *text,
                                           char unit,
                                           uint16_t color);

static void ui_draw_mono_9_chip_on_line(uint16_t row,
                                        uint16_t x,
                                        uint16_t y,
                                        const char *label,
                                        uint16_t width,
                                        uint16_t color)
{
    uint16_t text_width;
    uint16_t text_x;

    ui_draw_rect_on_line(row, x, y, width, 11U, color);
    ui_draw_box_on_line(row, x, y, width, 11U, color);
    text_width = ui_mono_9_text_width(label);
    text_x = (text_width < width) ? (uint16_t)(x + ((width - text_width) / 2U)) : (uint16_t)(x + 1U);
    ui_draw_mono_9_on_line(row, text_x, (uint16_t)(y + 1U), label, UI_COLOR_TEXT);
}

static void ui_draw_row_label_chip_on_line(uint16_t row,
                                           uint16_t x,
                                           uint16_t y,
                                           const uint16_t *label,
                                           uint8_t label_count,
                                           uint16_t color)
{
    uint16_t width;
    uint8_t index;
    uint16_t cursor;

    width = (uint16_t)(label_count * 26U + 3U);
    ui_draw_rect_on_line(row, x, y, width, 26U, color);
    cursor = (uint16_t)(x + 3U);
    for (index = 0U; index < label_count; ++index)
    {
        const font_zh_24_glyph_t *glyph;

        glyph = ui_zh_24_glyph(label[index]);
        if (glyph != NULL)
        {
            uint16_t local_row;
            uint16_t byte_width;
            uint16_t col;

            if (row <= y)
            {
                cursor = (uint16_t)(cursor + glyph->advance);
                continue;
            }
            local_row = (uint16_t)(row - y - 1U);
            byte_width = (uint16_t)((glyph->width + 1U) / 2U);
            if ((row >= y) && (local_row < glyph->height))
            {
                for (col = 0U; col < glyph->width; ++col)
                {
                    uint8_t packed;
                    uint8_t alpha;

                    packed = glyph->data[(uint16_t)(local_row * byte_width + col / 2U)];
                    alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                    if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                    {
                        uint16_t px;

                        px = (uint16_t)(cursor + col);
                        g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], UI_COLOR_TEXT, alpha);
                    }
                }
            }
            cursor = (uint16_t)(cursor + glyph->advance);
        }
    }
}

#if (defined(PX1_ENABLE_KEY_DEBUG_RENDERER) && (PX1_ENABLE_KEY_DEBUG_RENDERER != 0)) || defined(PX1_HOST_TEST)
static void ui_draw_ascii_on_line(uint16_t row,
                                  uint16_t x,
                                  uint16_t y,
                                  const char *text,
                                  uint8_t scale,
                                  uint16_t color)
{
    uint16_t cursor;

    if ((text == NULL) || (scale == 0U) || (row < y))
    {
        return;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const uint8_t *glyph;
        uint16_t glyph_row;
        uint16_t sy;
        uint16_t col;
        uint16_t sx;

        if ((row >= y) && (row < (uint16_t)(y + 7U * scale)))
        {
            glyph = ui_ascii_glyph(*text);
            glyph_row = (uint16_t)((row - y) / scale);
            sy = (uint16_t)((row - y) % scale);
            (void)sy;
            for (col = 0U; col < 5U; ++col)
            {
                if ((glyph[glyph_row] & (uint8_t)(1U << (4U - col))) != 0U)
                {
                    for (sx = 0U; sx < scale; ++sx)
                    {
                        uint16_t px;

                        px = (uint16_t)(cursor + col * scale + sx);
                        if (px < LCD_WIDTH)
                        {
                            g_line_buffer[px] = color;
                        }
                    }
                }
            }
        }

        cursor = (uint16_t)(cursor + 6U * scale);
        ++text;
    }
}
#endif

static void ui_draw_zh_15_on_line(uint16_t row,
                                  uint16_t x,
                                  uint16_t y,
                                  const uint16_t *codes,
                                  uint8_t count,
                                  uint16_t color)
{
    uint8_t index;

    if ((codes == NULL) || (row < y) || (row >= (uint16_t)(y + 15U)))
    {
        return;
    }

    for (index = 0U; index < count; ++index)
    {
        const font_zh_15_glyph_t *glyph;
        uint16_t col;
        uint16_t draw_x;
        uint16_t glyph_row;
        uint16_t byte_width;

        glyph = ui_zh_15_glyph(codes[index]);
        if (glyph == NULL)
        {
            continue;
        }

        draw_x = x;
        {
            uint8_t prev;

            for (prev = 0U; prev < index; ++prev)
            {
                const font_zh_15_glyph_t *prev_glyph;

                prev_glyph = ui_zh_15_glyph(codes[prev]);
                if (prev_glyph != NULL)
                {
                    draw_x = (uint16_t)(draw_x + prev_glyph->advance);
                }
            }
        }
        glyph_row = (uint16_t)(row - y);
        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        for (col = 0U; col < glyph->width; ++col)
        {
            uint8_t packed;
            uint8_t alpha;

            packed = glyph->data[(uint16_t)(glyph_row * byte_width + col / 2U)];
            alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
            if (alpha != 0U)
            {
                uint16_t px;

                px = (uint16_t)(draw_x + col);
                if (px < LCD_WIDTH)
                {
                    g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                }
            }
        }
    }
}

static uint16_t ui_draw_mono_12_on_line(uint16_t row,
                                        uint16_t x,
                                        uint16_t y,
                                        const char *text,
                                        uint16_t color)
{
    uint16_t cursor;

    if ((text == NULL) || (row < y))
    {
        return x;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const font_mono_12_glyph_t *glyph;
        uint16_t local_row;
        uint16_t byte_width;
        uint16_t col;

        glyph = ui_mono_12_glyph((uint16_t)(uint8_t)*text);
        local_row = (uint16_t)(row - y);
        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        if (local_row < glyph->height)
        {
            for (col = 0U; col < glyph->width; ++col)
            {
                uint8_t packed;
                uint8_t alpha;

                packed = glyph->data[(uint16_t)(local_row * byte_width + col / 2U)];
                alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                {
                    uint16_t px;

                    px = (uint16_t)(cursor + col);
                    g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                }
            }
        }
        cursor = (uint16_t)(cursor + glyph->advance);
        ++text;
    }

    return cursor;
}

static uint16_t ui_draw_mono_9_on_line(uint16_t row,
                                       uint16_t x,
                                       uint16_t y,
                                       const char *text,
                                       uint16_t color)
{
    uint16_t cursor;

    if ((text == NULL) || (row < y))
    {
        return x;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const font_mono_9_glyph_t *glyph;
        uint16_t local_row;
        uint16_t byte_width;
        uint16_t col;

        glyph = ui_mono_9_glyph((uint16_t)(uint8_t)*text);
        local_row = (uint16_t)(row - y);
        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        if (local_row < glyph->height)
        {
            for (col = 0U; col < glyph->width; ++col)
            {
                uint8_t packed;
                uint8_t alpha;

                packed = glyph->data[(uint16_t)(local_row * byte_width + col / 2U)];
                alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                {
                    uint16_t px;

                    px = (uint16_t)(cursor + col);
                    g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                }
            }
        }
        cursor = (uint16_t)(cursor + glyph->advance);
        ++text;
    }

    return cursor;
}

static uint8_t ui_mono_15_bottom_row(const font_mono_15_glyph_t *glyph)
{
    int row;

    if (glyph == NULL)
    {
        return 0U;
    }

    for (row = (int)glyph->height - 1; row >= 0; --row)
    {
        uint16_t byte_width;
        uint16_t col;

        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        for (col = 0U; col < glyph->width; ++col)
        {
            uint8_t packed;
            uint8_t alpha;

            packed = glyph->data[(uint16_t)((uint16_t)row * byte_width + col / 2U)];
            alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
            if (alpha > 3U)
            {
                return (uint8_t)row;
            }
        }
    }

    return 0U;
}

static uint16_t ui_draw_mono_15_baseline_on_line(uint16_t row,
                                                 uint16_t x,
                                                 uint16_t y,
                                                 const char *text,
                                                 uint16_t color)
{
    uint16_t cursor;

    if ((text == NULL) || (row < y))
    {
        return x;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const font_mono_15_glyph_t *glyph;
        uint8_t bottom;
        int16_t source_row;
        uint16_t byte_width;
        uint16_t col;

        glyph = ui_mono_15_glyph((uint16_t)(uint8_t)*text);
        bottom = ui_mono_15_bottom_row(glyph);
        source_row = (int16_t)(row - y) - (int16_t)(14U - bottom);
        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        if ((source_row >= 0) && (source_row < (int16_t)glyph->height))
        {
            for (col = 0U; col < glyph->width; ++col)
            {
                uint8_t packed;
                uint8_t alpha;

                packed = glyph->data[(uint16_t)((uint16_t)source_row * byte_width + col / 2U)];
                alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                {
                    uint16_t px;

                    px = (uint16_t)(cursor + col);
                    g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                }
            }
        }
        cursor = (uint16_t)(cursor + glyph->advance);
        ++text;
    }

    return cursor;
}

static uint16_t ui_draw_meter_text_on_line(uint16_t row,
                                           uint16_t x,
                                           uint16_t y,
                                           const char *text,
                                           char unit,
                                           uint16_t color)
{
    uint16_t cursor;
    char unit_text[2];

    if (text == NULL)
    {
        return x;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const font_meter_24_glyph_t *glyph;

        glyph = ui_meter_24_glyph((uint16_t)(uint8_t)*text);
        if (glyph != NULL)
        {
            uint16_t local_row;
            uint16_t byte_width;
            uint16_t col;

            local_row = (uint16_t)(row - y);
            byte_width = (uint16_t)((glyph->width + 1U) / 2U);
            if ((row >= y) && (local_row < glyph->height))
            {
                for (col = 0U; col < glyph->width; ++col)
                {
                    uint8_t packed;
                    uint8_t alpha;

                    packed = glyph->data[(uint16_t)(local_row * byte_width + col / 2U)];
                    alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                    if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                    {
                        uint16_t px;

                        px = (uint16_t)(cursor + col);
                        g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                    }
                }
            }
            cursor = (uint16_t)(cursor + glyph->advance);
        }
        ++text;
    }

    if (unit != '\0')
    {
        unit_text[0] = unit;
        unit_text[1] = '\0';
        return ui_draw_meter_text_on_line(row, cursor, y, unit_text, '\0', color);
    }
    return cursor;
}

static uint16_t ui_draw_mono_15_on_line(uint16_t row,
                                        uint16_t x,
                                        uint16_t y,
                                        const char *text,
                                        uint16_t color)
{
    uint16_t cursor;

    if ((text == NULL) || (row < y))
    {
        return x;
    }

    cursor = x;
    while ((*text != '\0') && (cursor < LCD_WIDTH))
    {
        const font_mono_15_glyph_t *glyph;
        uint16_t local_row;
        uint16_t byte_width;
        uint16_t col;

        glyph = ui_mono_15_glyph((uint16_t)(uint8_t)*text);
        local_row = (uint16_t)(row - y);
        byte_width = (uint16_t)((glyph->width + 1U) / 2U);
        if (local_row < glyph->height)
        {
            for (col = 0U; col < glyph->width; ++col)
            {
                uint8_t packed;
                uint8_t alpha;

                packed = glyph->data[(uint16_t)(local_row * byte_width + col / 2U)];
                alpha = ((col & 1U) == 0U) ? (uint8_t)(packed >> 4) : (uint8_t)(packed & 0x0FU);
                if ((alpha != 0U) && ((uint16_t)(cursor + col) < LCD_WIDTH))
                {
                    uint16_t px;

                    px = (uint16_t)(cursor + col);
                    g_line_buffer[px] = ui_blend_rgb565(g_line_buffer[px], color, alpha);
                }
            }
        }
        cursor = (uint16_t)(cursor + glyph->advance);
        ++text;
    }

    return cursor;
}

static void ui_append_uint(char *out, uint8_t *pos, uint8_t limit, uint32_t value, uint8_t min_digits)
{
    char temp[10];
    uint8_t count;

    count = 0U;
    do
    {
        temp[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (count < sizeof(temp)));

    while ((count < min_digits) && (count < sizeof(temp)))
    {
        temp[count++] = '0';
    }

    while ((count != 0U) && (*pos < limit))
    {
        out[(*pos)++] = temp[--count];
    }
}

static uint32_t ui_abs_to_u32(int32_t value)
{
    if (value >= 0)
    {
        return (uint32_t)value;
    }

    return (uint32_t)(-(value + 1)) + 1U;
}

static void ui_format_meter_2dec(char *out, uint8_t limit, int32_t milli_value)
{
    uint8_t pos;
    uint32_t centi;
    uint32_t value;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    value = ui_abs_to_u32(milli_value);
    centi = (value + 5U) / 10U;
    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi / 100U, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi % 100U, 2U);
    out[pos] = '\0';
}

static void ui_append_unit_char(char *out, uint8_t limit, char unit)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    while ((pos < (uint8_t)(limit - 1U)) && (out[pos] != '\0'))
    {
        ++pos;
    }
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = unit;
    }
    out[pos] = '\0';
}

static int32_t ui_scope_abs_i32(int32_t value)
{
    if (value == INT32_MIN)
    {
        return INT32_MAX;
    }

    return (value < 0) ? -value : value;
}

static int16_t ui_scope_clamp_i16(int32_t value)
{
    if (value > INT16_MAX)
    {
        return INT16_MAX;
    }
    if (value < INT16_MIN)
    {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static int32_t ui_scope_range_padding(int32_t value)
{
    int32_t abs_value;
    int32_t padding;

    abs_value = ui_scope_abs_i32(value);
    padding = abs_value / 20;
    if (padding < 10)
    {
        padding = 10;
    }
    return padding;
}

static void ui_scope_expand_range(int32_t *min_value, int32_t *max_value)
{
    int32_t padding;

    if ((min_value == NULL) || (max_value == NULL))
    {
        return;
    }

    if (*min_value == *max_value)
    {
        padding = ui_scope_range_padding(*max_value);
        *min_value = (*min_value > padding) ? (*min_value - padding) : 0;
        *max_value += padding;
    }

    if (*max_value <= *min_value)
    {
        *max_value = *min_value + 1;
    }
}

static void ui_scope_history_range(uint8_t current,
                                   int32_t fallback_value,
                                   int32_t *min_value,
                                   int32_t *max_value)
{
    uint16_t index;
    int32_t value;

    if ((min_value == NULL) || (max_value == NULL))
    {
        return;
    }

    value = (current != 0U) ? ui_scope_abs_i32(fallback_value) : fallback_value;
    *min_value = value;
    *max_value = value;

    if (g_scope_history.count >= 2U)
    {
        for (index = 0U; index < g_scope_history.count; ++index)
        {
            value = (current != 0U) ?
                    ui_scope_abs_i32(g_scope_history.current_ma[index]) :
                    g_scope_history.voltage_mv[index];
            if (value < *min_value)
            {
                *min_value = value;
            }
            if (value > *max_value)
            {
                *max_value = value;
            }
        }
    }

    ui_scope_expand_range(min_value, max_value);
}

static uint16_t ui_scope_value_to_y_region(int32_t value,
                                           int32_t min_value,
                                           int32_t max_value,
                                           uint16_t top_y,
                                           uint16_t height)
{
    int32_t clamped;
    uint32_t range;
    uint16_t span;

    clamped = value;
    if (clamped < min_value)
    {
        clamped = min_value;
    }
    if (clamped > max_value)
    {
        clamped = max_value;
    }

    range = (uint32_t)(max_value - min_value);
    span = (uint16_t)(height - 1U);
    return (uint16_t)(top_y +
                      ((uint32_t)(max_value - clamped) * span) / range);
}

static uint16_t ui_scope_value_to_y(int32_t value, int32_t min_value, int32_t max_value)
{
    return ui_scope_value_to_y_region(value,
                                      min_value,
                                      max_value,
                                      UI_SCOPE_GRAPH_TOP_Y,
                                      UI_SCOPE_GRAPH_HEIGHT);
}

static void ui_format_uint_suffix(char *out,
                                  uint8_t limit,
                                  uint32_t value,
                                  const char *suffix)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), value, 1U);
    while ((suffix != NULL) && (*suffix != '\0') && (pos < (uint8_t)(limit - 1U)))
    {
        out[pos++] = *suffix++;
    }
    out[pos] = '\0';
}

static void ui_format_frequency_short(char *out, uint8_t limit, uint32_t hz)
{
    if (hz >= 1000000U)
    {
        ui_format_uint_suffix(out, limit, hz / 1000000U, "M");
    }
    else if (hz >= 1000U)
    {
        ui_format_uint_suffix(out, limit, hz / 1000U, "K");
    }
    else
    {
        ui_format_uint_suffix(out, limit, hz, "H");
    }
}

static void ui_scope_history_decimate(void)
{
    uint16_t index;

    for (index = 0U; index < (UI_SCOPE_HISTORY_CAPACITY / 2U); ++index)
    {
        g_scope_history.voltage_mv[index] = g_scope_history.voltage_mv[(uint16_t)(index * 2U)];
        g_scope_history.current_ma[index] = g_scope_history.current_ma[(uint16_t)(index * 2U)];
    }
    g_scope_history.count = UI_SCOPE_HISTORY_CAPACITY / 2U;
    if (g_scope_history.interval_s < 32768U)
    {
        g_scope_history.interval_s = (uint16_t)(g_scope_history.interval_s * 2U);
    }
}

static void ui_scope_history_push(int32_t voltage_mv, int32_t current_ma)
{
    if (g_scope_history.count >= UI_SCOPE_HISTORY_CAPACITY)
    {
        ui_scope_history_decimate();
    }

    g_scope_history.voltage_mv[g_scope_history.count] = ui_scope_clamp_i16(voltage_mv);
    g_scope_history.current_ma[g_scope_history.count] = ui_scope_clamp_i16(current_ma);
    ++g_scope_history.count;
}

void ui_renderer_update_scope_history(const measure_snapshot_t *measure)
{
    uint32_t elapsed_s;
    int32_t voltage_mv;
    int32_t current_ma;

    if ((measure == NULL) || (measure->voltage_valid == 0U) || (measure->current_valid == 0U))
    {
        return;
    }

    elapsed_s = measure->stat_elapsed_s;
    voltage_mv = measure->voltage_avg_mv;
    current_ma = measure->current_avg_ma;

    if ((g_scope_history.initialized == 0U) ||
        (elapsed_s < g_scope_history.last_sample_s))
    {
        g_scope_history = (ui_scope_history_t){ 0 };
        g_scope_history.interval_s = 1U;
        g_scope_history.last_sample_s = elapsed_s;
        g_scope_history.initialized = 1U;
        ui_scope_history_push(voltage_mv, current_ma);
        return;
    }

    if ((elapsed_s - g_scope_history.last_sample_s) >= g_scope_history.interval_s)
    {
        g_scope_history.last_sample_s = elapsed_s;
        ui_scope_history_push(voltage_mv, current_ma);
    }
}

static void ui_ripple_history_push(int32_t voltage_mv)
{
    g_ripple_history.voltage_mv[g_ripple_history.next_index] = voltage_mv;
    g_ripple_history.next_index = (uint16_t)((g_ripple_history.next_index + 1U) % UI_SCOPE_HISTORY_CAPACITY);
    if (g_ripple_history.count < UI_SCOPE_HISTORY_CAPACITY)
    {
        ++g_ripple_history.count;
    }
}

void ui_renderer_update_ripple_history(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure)
{
    uint8_t index;

    if ((measure == NULL) || (ui_model_ripple_paused(state) != 0U))
    {
        return;
    }

    if (measure->ripple_sample_count != 0U)
    {
        for (index = 0U; index < measure->ripple_sample_count; ++index)
        {
            ui_ripple_history_push(measure->ripple_sample_mv[index]);
        }
    }
    else
    {
        ui_ripple_history_push(ui_measure_voltage_mv(measure));
    }
}

static void ui_format_meter_4digits(char *out, uint8_t limit, int32_t milli_value)
{
    uint8_t pos;
    uint32_t value;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    value = ui_abs_to_u32(milli_value);

    pos = 0U;
    if (value < 10000U)
    {
        uint32_t milli;

        milli = (uint32_t)value;
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), milli / 1000U, 1U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), milli % 1000U, 3U);
    }
    else
    {
        uint32_t centi;

        centi = (value + 5U) / 10U;
        if (centi > 9999U)
        {
            centi = 9999U;
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi / 100U, 2U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi % 100U, 2U);
    }

    out[pos] = '\0';
}

static void ui_format_meter_5digits(char *out, uint8_t limit, int32_t milli_value)
{
    uint8_t pos;
    uint8_t decimals;
    uint32_t integer;
    uint32_t frac;
    uint32_t value;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    value = ui_abs_to_u32(milli_value);

    integer = value / 1000U;
    decimals = (integer < 10U) ? 4U : 3U;
    if (decimals == 4U)
    {
        frac = (value % 1000U) * 10U;
    }
    else
    {
        frac = value % 1000U;
    }

    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), integer, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), frac, decimals);
    out[pos] = '\0';
}

static void ui_format_current_5digits(char *out, uint8_t limit, int32_t deci_ma_value)
{
    uint8_t pos;
    uint8_t decimals;
    uint32_t integer;
    uint32_t frac;
    uint32_t value;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    value = ui_abs_to_u32(deci_ma_value);

    integer = value / 10000U;
    decimals = (integer < 10U) ? 4U : 3U;
    if (decimals == 4U)
    {
        frac = value % 10000U;
    }
    else
    {
        frac = (value % 10000U) / 10U;
    }

    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), integer, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), frac, decimals);
    out[pos] = '\0';
}

static void ui_format_current_4digits(char *out, uint8_t limit, int32_t deci_ma_value)
{
    uint8_t pos;
    uint32_t value;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    value = ui_abs_to_u32(deci_ma_value);

    pos = 0U;
    if (value < 100000U)
    {
        uint32_t milli;

        milli = (uint32_t)((value + 5) / 10);
        if (milli > 9999U)
        {
            milli = 9999U;
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), milli / 1000U, 1U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), milli % 1000U, 3U);
    }
    else
    {
        uint32_t centi;

        centi = (value + 50U) / 100U;
        if (centi > 9999U)
        {
            centi = 9999U;
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi / 100U, 2U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi % 100U, 2U);
    }

    out[pos] = '\0';
}

static void ui_format_wh(char *out, uint8_t limit, uint32_t mwh)
{
    uint8_t pos;
    uint32_t centi_wh;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    centi_wh = (mwh + 5U) / 10U;
    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_wh / 100U, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_wh % 100U, 2U);
    out[pos] = '\0';
}

static void ui_format_time(char *out, uint8_t limit, uint32_t seconds)
{
    uint8_t pos;
    uint32_t hours;
    uint32_t minutes;

    if ((out == NULL) || (limit < 9U))
    {
        return;
    }

    hours = seconds / 3600U;
    minutes = (seconds / 60U) % 60U;
    seconds %= 60U;
    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), hours, 2U);
    out[pos++] = ':';
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), minutes, 2U);
    out[pos++] = ':';
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), seconds, 2U);
    out[pos] = '\0';
}

static void ui_format_capacity(char *out, uint8_t limit, uint32_t value)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 2U))
    {
        return;
    }

    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), value, 1U);
    out[pos] = '\0';
}

static uint32_t ui_capacity_mah_from_4v2(uint32_t mwh)
{
    return (mwh / 42U) * 10U + (((mwh % 42U) * 10U + 21U) / 42U);
}

static uint16_t ui_mono_12_text_width(const char *text)
{
    uint16_t width;

    width = 0U;
    if (text != NULL)
    {
        while (*text != '\0')
        {
            width = (uint16_t)(width + ui_mono_12_glyph((uint16_t)(uint8_t)*text)->advance);
            ++text;
        }
    }

    return width;
}

static uint16_t ui_mono_9_text_width(const char *text)
{
    uint16_t width;

    width = 0U;
    if (text != NULL)
    {
        while (*text != '\0')
        {
            width = (uint16_t)(width + ui_mono_9_glyph((uint16_t)(uint8_t)*text)->advance);
            ++text;
        }
    }

    return width;
}

static uint16_t ui_mono_15_text_width(const char *text)
{
    uint16_t width;

    width = 0U;
    if (text != NULL)
    {
        while (*text != '\0')
        {
            width = (uint16_t)(width + ui_mono_15_glyph((uint16_t)(uint8_t)*text)->advance);
            ++text;
        }
    }

    return width;
}

static uint16_t ui_zh_15_text_width(const uint16_t *codes, uint8_t count)
{
    uint16_t width;
    uint8_t index;

    width = 0U;
    if (codes == NULL)
    {
        return 0U;
    }

    for (index = 0U; index < count; ++index)
    {
        const font_zh_15_glyph_t *glyph;

        glyph = ui_zh_15_glyph(codes[index]);
        if (glyph != NULL)
        {
            width = (uint16_t)(width + glyph->advance);
        }
    }

    return width;
}

static uint16_t ui_center_x(uint16_t x, uint16_t width, uint16_t text_width)
{
    if (text_width >= width)
    {
        return x;
    }
    return (uint16_t)(x + (width - text_width) / 2U);
}

static uint8_t ui_mv_between(int16_t mv, int16_t low, int16_t high)
{
    return ((mv >= low) && (mv <= high)) ? 1U : 0U;
}

static uint8_t ui_mv_near(int16_t mv, int16_t target, int16_t tolerance)
{
    int16_t diff;

    diff = (mv >= target) ? (int16_t)(mv - target) : (int16_t)(target - mv);
    return (diff <= tolerance) ? 1U : 0U;
}

static const char *ui_infer_dpdm_protocol(const protocol_snapshot_t *protocol)
{
    int16_t dp;
    int16_t dm;
    int16_t diff;

    if (protocol == NULL)
    {
        return "---";
    }

    dp = protocol->dp_mv;
    dm = protocol->dm_mv;
    diff = (dp >= dm) ? (int16_t)(dp - dm) : (int16_t)(dm - dp);

    if (protocol->kind == PROTOCOL_KIND_QC)
    {
        if ((protocol->target_mv >= 18500) ||
            (ui_mv_between(dp, 2850, 3600) && ui_mv_between(dm, 2850, 3600)))
        {
            return "QC20";
        }
        if ((protocol->target_mv >= 11000) ||
            (ui_mv_between(dp, 450, 900) && ui_mv_between(dm, 450, 900)))
        {
            return "QC12";
        }
        if ((protocol->target_mv >= 8500) ||
            (ui_mv_between(dp, 2850, 3600) && ui_mv_between(dm, 450, 900)))
        {
            return "QC9";
        }
        if ((dp >= 300) || (dm >= 300))
        {
            return "QC5";
        }
        return "QC";
    }

    if ((dp < 150) && (dm < 150))
    {
        if (protocol->kind == PROTOCOL_KIND_PD)
        {
            return "PD";
        }
        return "---";
    }

    if (ui_mv_near(dp, 1200, 250) && ui_mv_near(dm, 1200, 250))
    {
        return "SAMS";
    }
    if (ui_mv_near(dp, 2000, 300) && ui_mv_near(dm, 2000, 300))
    {
        return "APL1A";
    }
    if ((ui_mv_near(dp, 2000, 350) && ui_mv_near(dm, 2700, 350)) ||
        (ui_mv_near(dp, 2700, 350) && ui_mv_near(dm, 2000, 350)) ||
        (ui_mv_near(dp, 2700, 350) && ui_mv_near(dm, 2700, 350)))
    {
        return "APL2A";
    }
    if (ui_mv_near(dp, 2700, 350) && ui_mv_near(dm, 3300, 350))
    {
        return "APL24";
    }
    if ((dp >= 300) && (dp <= 900) && (dm >= 300) && (dm <= 900) && (diff <= 250))
    {
        return "DCP";
    }
    if ((dp >= 2500) && (dm >= 2500))
    {
        return "APPLE";
    }
    if ((dp >= 500) && (dp <= 900) && (dm >= 2500))
    {
        return "QC?";
    }
    if ((dp >= 2500) && (dm >= 500) && (dm <= 900))
    {
        return "QC9";
    }
    if (protocol->kind == PROTOCOL_KIND_PD)
    {
        return "PD";
    }
    if (protocol->kind == PROTOCOL_KIND_QC)
    {
        return "QC";
    }
    if (protocol->kind == PROTOCOL_KIND_AFC)
    {
        return "AFC";
    }
    if (protocol->kind == PROTOCOL_KIND_FCP)
    {
        return "FCP";
    }

    return "---";
}

#if defined(PX1_HOST_TEST)
const char *ui_renderer_infer_dpdm_protocol_for_test(const protocol_snapshot_t *protocol)
{
    return ui_infer_dpdm_protocol(protocol);
}
#endif

static void ui_format_dpdm_voltage(char *out, uint8_t limit, int32_t mv)
{
    uint8_t pos;
    uint32_t centi_v;

    if ((out == NULL) || (limit < 6U))
    {
        return;
    }

    if (mv < 0)
    {
        mv = 0;
    }
    centi_v = (uint32_t)((mv + 5) / 10);
    pos = 0U;
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_v / 100U, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_v % 100U, 2U);
    out[pos] = '\0';
}

static void ui_append_char(char *out, uint8_t *pos, uint8_t limit, char ch)
{
    if ((out != NULL) && (pos != NULL) && (*pos < limit))
    {
        out[*pos] = ch;
        ++(*pos);
    }
}

static void ui_append_ascii_text(char *out, uint8_t *pos, uint8_t limit, const char *text)
{
    while ((out != NULL) && (pos != NULL) && (text != NULL) &&
           (*text != '\0') && (*pos < limit))
    {
        out[*pos] = *text;
        ++(*pos);
        ++text;
    }
}

static void ui_append_voltage_tenth(char *out, uint8_t *pos, uint8_t limit, int32_t mv)
{
    uint32_t deci_v;

    if (mv < 0)
    {
        mv = 0;
    }

    deci_v = (uint32_t)((mv + 50) / 100);
    ui_append_uint(out, pos, limit, deci_v / 10U, 1U);
    if ((deci_v % 10U) != 0U)
    {
        ui_append_char(out, pos, limit, '.');
        ui_append_uint(out, pos, limit, deci_v % 10U, 1U);
    }
}

static void ui_format_pdo_voltage(char *out, uint8_t limit, const service_pd_source_pdo_t *pdo)
{
    uint8_t pos;
    uint32_t hundredths;

    if ((out == NULL) || (limit < 2U))
    {
        return;
    }

    pos = 0U;
    if ((pdo == NULL) || (pdo->max_mv <= 0))
    {
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        out[pos] = '\0';
        return;
    }

    if ((pdo->min_mv > 0) && (pdo->min_mv != pdo->max_mv))
    {
        ui_append_voltage_tenth(out, &pos, (uint8_t)(limit - 1U), pdo->min_mv);
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        ui_append_voltage_tenth(out, &pos, (uint8_t)(limit - 1U), pdo->max_mv);
    }
    else
    {
        hundredths = (uint32_t)((pdo->max_mv + 5) / 10);
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), hundredths / 100U, 1U);
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '.');
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), hundredths % 100U, 2U);
    }
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'V');
    out[pos] = '\0';
}

static void ui_format_pdo_current(char *out, uint8_t limit, int32_t ma)
{
    uint8_t pos;
    uint32_t centi_a;

    if ((out == NULL) || (limit < 2U))
    {
        return;
    }

    pos = 0U;
    if (ma <= 0)
    {
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'A');
        out[pos] = '\0';
        return;
    }

    centi_a = (uint32_t)((ma + 5) / 10);
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_a / 100U, 1U);
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), '.');
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), centi_a % 100U, 2U);
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'A');
    out[pos] = '\0';
}

static void ui_format_pdo_power(char *out, uint8_t limit, uint16_t deci_w)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 2U))
    {
        return;
    }

    pos = 0U;
    if (deci_w == 0U)
    {
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '-');
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'W');
        out[pos] = '\0';
        return;
    }

    if (deci_w >= 1000U)
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (deci_w + 5U) / 10U, 1U);
    }
    else
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), deci_w / 10U, 1U);
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), '.');
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), deci_w % 10U, 1U);
    }
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'W');
    out[pos] = '\0';
}

static const char *ui_trigger_pdo_type_text(uint8_t type)
{
    switch (type)
    {
        case SERVICE_PD_SOURCE_PDO_FIXED:
            return "PDO";
        case SERVICE_PD_SOURCE_PDO_BATTERY:
            return "BAT";
        case SERVICE_PD_SOURCE_PDO_VARIABLE:
            return "VAR";
        case SERVICE_PD_SOURCE_PDO_PPS:
            return "PPS";
        case SERVICE_PD_SOURCE_PDO_AVS:
            return "AVS";
        default:
            return "PDO";
    }
}

static const char *ui_trigger_state_text(const protocol_snapshot_t *protocol)
{
    if (protocol == NULL)
    {
        return "RD";
    }

    switch (protocol->request_state)
    {
        case PROTOCOL_REQUEST_REQUESTING:
            return "REQ";
        case PROTOCOL_REQUEST_ACCEPTED:
            return "ACC";
        case PROTOCOL_REQUEST_READY:
            return "OK";
        case PROTOCOL_REQUEST_FAILED:
            return "FAIL";
        case PROTOCOL_REQUEST_AVAILABLE:
            return "PDO";
        case PROTOCOL_REQUEST_IDLE:
        default:
            return "RD";
    }
}

static void ui_format_trigger_header(char *out, uint8_t limit, const service_pd_source_pdo_t *pdo)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), "PDO");
    if (pdo != NULL)
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), pdo->position, 1U);
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), ' ');
        ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), ui_trigger_pdo_type_text(pdo->type));
    }
    out[pos] = '\0';
}

static void ui_format_trigger_mv(char *out, uint8_t limit, int32_t mv)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    ui_append_voltage_tenth(out, &pos, (uint8_t)(limit - 1U), mv);
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), 'V');
    out[pos] = '\0';
}

static void ui_draw_trigger_row_on_line(uint16_t row,
                                        uint16_t y,
                                        uint8_t selected,
                                        const service_pd_source_pdo_t *pdo)
{
    char index_text[4];
    char voltage_text[13];
    char current_text[8];
    uint8_t pos;
    uint16_t color;

    if (pdo == NULL)
    {
        return;
    }

    if (selected != 0U)
    {
        ui_draw_rect_on_line(row, 1U, y, 157U, 11U, 0x31A6U);
        if ((row >= (uint16_t)(y + 2U)) && (row <= (uint16_t)(y + 8U)))
        {
            uint16_t local_row;
            uint16_t width;

            local_row = (uint16_t)(row - y - 2U);
            width = (local_row <= 3U) ? (uint16_t)(local_row + 1U) : (uint16_t)(7U - local_row);
            ui_draw_rect_on_line(row, 3U, row, width, 1U, UI_COLOR_MAIN_AMBER);
        }
    }

    pos = 0U;
    ui_append_uint(index_text, &pos, (uint8_t)(sizeof(index_text) - 1U), pdo->position, 1U);
    index_text[pos] = '\0';
    ui_format_pdo_voltage(voltage_text, sizeof(voltage_text), pdo);
    ui_format_pdo_current(current_text, sizeof(current_text), pdo->current_ma);
    color = (selected != 0U) ? UI_COLOR_MAIN_AMBER : UI_COLOR_TEXT;

    ui_draw_mono_9_on_line(row, (selected != 0U) ? 11U : 2U, (uint16_t)(y + 1U), index_text, color);
    ui_draw_mono_9_on_line(row, 23U, (uint16_t)(y + 1U), ui_trigger_pdo_type_text(pdo->type), color);
    ui_draw_mono_9_on_line(row, 50U, (uint16_t)(y + 1U), voltage_text, color);
    ui_draw_mono_9_on_line(row, 116U, (uint16_t)(y + 1U), current_text, color);
}

void ui_renderer_draw_trigger_select_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol)
{
    service_pd_source_caps_snapshot_t caps;
    uint8_t count;
    uint8_t selected;
    uint8_t scroll;
    uint8_t max_scroll;
    uint16_t row;
    char count_text[8];
    uint8_t pos;

    service_pd_copy_source_caps(&caps);
    count = caps.count;
    selected = ui_model_trigger_selected_index(state);
    scroll = ui_model_trigger_scroll(state);
    max_scroll = (count > 5U) ? (uint8_t)(count - 5U) : 0U;
    if (scroll > max_scroll)
    {
        scroll = max_scroll;
    }

    pos = 0U;
    ui_append_ascii_text(count_text, &pos, (uint8_t)(sizeof(count_text) - 1U), "PDO:");
    ui_append_uint(count_text, &pos, (uint8_t)(sizeof(count_text) - 1U), count, 1U);
    count_text[pos] = '\0';

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        uint8_t index;

        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_mono_9_on_line(row, 3U, 2U, "TRIGGER", UI_COLOR_MAIN_CYAN);
        ui_draw_mono_9_on_line(row, 72U, 2U, count_text, UI_COLOR_MAIN_GREEN);
        ui_draw_mono_9_on_line(row, 132U, 2U, ui_trigger_state_text(protocol), UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 2U, 12U, 156U, 1U, UI_COLOR_FRAME);

        if (count == 0U)
        {
            ui_draw_mono_15_on_line(row, 19U, 24U, "HOLD RD", UI_COLOR_MAIN_AMBER);
            ui_draw_mono_15_on_line(row, 14U, 47U, "WAIT PDO", UI_COLOR_MAIN_CYAN);
        }

        for (index = 0U; index < 5U; ++index)
        {
            uint8_t pdo_index;

            pdo_index = (uint8_t)(scroll + index);
            if ((pdo_index >= count) || (pdo_index >= SERVICE_PD_SOURCE_PDO_MAX))
            {
                continue;
            }
            ui_draw_trigger_row_on_line(row,
                                        (uint16_t)(15U + index * 12U),
                                        (pdo_index == selected) ? 1U : 0U,
                                        &caps.pdos[pdo_index]);
        }

        if (count > 5U)
        {
            uint16_t marker_y;

            ui_draw_rect_on_line(row, 157U, 14U, 1U, 61U, UI_COLOR_FRAME);
            marker_y = (uint16_t)(14U + ((uint16_t)scroll * 58U) / max_scroll);
            ui_draw_rect_on_line(row, 156U, marker_y, 3U, 5U, UI_COLOR_MAIN_AMBER);
        }
        ui_end_row(row);
    }
}

void ui_renderer_draw_trigger_adjust_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol)
{
    service_pd_source_caps_snapshot_t caps;
    service_pd_source_pdo_t pdo;
    uint8_t selected;
    char header[12];
    char target[9];
    char range[18];
    char step[10];
    char min_v[8];
    char max_v[8];
    uint16_t row;
    uint8_t pos;

    service_pd_copy_source_caps(&caps);
    selected = ui_model_trigger_selected_index(state);
    if ((caps.count != 0U) && (selected < caps.count) && (selected < SERVICE_PD_SOURCE_PDO_MAX))
    {
        pdo = caps.pdos[selected];
    }
    else
    {
        pdo = (service_pd_source_pdo_t){ 0 };
    }

    ui_format_trigger_header(header, sizeof(header), &pdo);
    ui_format_trigger_mv(target, sizeof(target), ui_model_trigger_target_mv(state));
    ui_format_trigger_mv(min_v, sizeof(min_v), ui_model_trigger_min_mv(state));
    ui_format_trigger_mv(max_v, sizeof(max_v), ui_model_trigger_max_mv(state));

    pos = 0U;
    ui_append_ascii_text(range, &pos, (uint8_t)(sizeof(range) - 1U), min_v);
    ui_append_char(range, &pos, (uint8_t)(sizeof(range) - 1U), '-');
    ui_append_ascii_text(range, &pos, (uint8_t)(sizeof(range) - 1U), max_v);
    range[pos] = '\0';

    pos = 0U;
    ui_append_ascii_text(step, &pos, (uint8_t)(sizeof(step) - 1U), "STEP ");
    ui_append_uint(step, &pos, (uint8_t)(sizeof(step) - 1U), (uint32_t)ui_model_trigger_step_mv(state), 1U);
    ui_append_ascii_text(step, &pos, (uint8_t)(sizeof(step) - 1U), "MV");
    step[pos] = '\0';

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_mono_9_on_line(row, 3U, 2U, header, UI_COLOR_MAIN_CYAN);
        ui_draw_mono_9_on_line(row, 132U, 2U, ui_trigger_state_text(protocol), UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 2U, 12U, 156U, 1U, UI_COLOR_FRAME);
        ui_draw_mono_15_on_line(row,
                                ui_center_x(0U, LCD_WIDTH, ui_mono_15_text_width(target)),
                                20U,
                                target,
                                UI_COLOR_MAIN_GREEN);
        ui_draw_mono_9_on_line(row, ui_center_x(0U, LCD_WIDTH, ui_mono_9_text_width(range)), 43U, range, UI_COLOR_TEXT);
        ui_draw_mono_9_on_line(row, ui_center_x(0U, LCD_WIDTH, ui_mono_9_text_width(step)), 54U, step, UI_COLOR_MAIN_CYAN);
        ui_draw_mono_15_on_line(row, 6U, 63U, "-", UI_COLOR_MAIN_AMBER);
        ui_draw_mono_15_on_line(row, ui_center_x(0U, LCD_WIDTH, ui_mono_15_text_width("OK")), 63U, "OK", UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 145U, 63U, "+", UI_COLOR_MAIN_AMBER);
        ui_end_row(row);
    }
}

static int32_t ui_measure_voltage_mv(const measure_snapshot_t *measure)
{
    return ((measure != NULL) && (measure->voltage_valid != 0U)) ? measure->voltage_avg_mv : 0;
}

static int32_t ui_measure_current_ma(const measure_snapshot_t *measure)
{
    return ((measure != NULL) && (measure->current_valid != 0U)) ? measure->current_avg_ma : 0;
}

static int32_t ui_measure_current_deci_ma(const measure_snapshot_t *measure)
{
    if ((measure == NULL) || (measure->current_valid == 0U))
    {
        return 0;
    }
    if (measure->current_avg_deci_ma != 0)
    {
        return measure->current_avg_deci_ma;
    }
    return measure->current_avg_ma * 10;
}

static int32_t ui_measure_stat_current_deci_ma(const measure_snapshot_t *measure, uint8_t average)
{
    int32_t deci_ma;
    int32_t ma;

    if (measure == NULL)
    {
        return 0;
    }

    deci_ma = (average != 0U) ? measure->stat_current_avg_deci_ma : measure->stat_current_max_deci_ma;
    if (deci_ma != 0)
    {
        return deci_ma;
    }

    ma = (average != 0U) ? measure->stat_current_avg_ma : measure->stat_current_max_ma;
    return ma * 10;
}

static int32_t ui_measure_power_mw(const measure_snapshot_t *measure)
{
    return ((measure != NULL) && (measure->power_valid != 0U)) ? measure->power_mw : 0;
}

static void ui_begin_row(uint16_t row)
{
    (void)row;
    ui_clear_line(UI_COLOR_BLACK);
}

static void ui_end_row(uint16_t row)
{
    ui_flush_line(row);
}

void ui_renderer_init(void)
{
    g_frame_streaming = 0U;
    g_frame_next_row = 0U;
}

void ui_renderer_begin_frame_stream(void)
{
    g_frame_streaming = 1U;
    g_frame_next_row = 0U;
    bsp_lcd_set_window(0U, 0U, LCD_WIDTH, LCD_HEIGHT);
}

void ui_renderer_end_frame_stream(void)
{
    g_frame_streaming = 0U;
    g_frame_next_row = 0U;
}

void ui_renderer_draw_boot_screen(void)
{
    uint16_t row;

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_end_row(row);
    }
}

void ui_renderer_draw_main_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    char voltage[8];
    char current[8];
    char power[8];
    int32_t mv;
    int32_t deci_ma;
    int32_t mw;
    uint16_t row;

    (void)state;
    (void)protocol;
    mv = ((measure != NULL) && (measure->voltage_valid != 0U)) ? measure->voltage_avg_mv : 0;
    deci_ma = ui_measure_current_deci_ma(measure);
    mw = ((measure != NULL) && (measure->power_valid != 0U)) ? measure->power_mw : 0;
    ui_format_meter_5digits(voltage, sizeof(voltage), mv);
    ui_format_current_5digits(current, sizeof(current), deci_ma);
    ui_format_meter_5digits(power, sizeof(power), mw);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_row_label_chip_on_line(row, 1U, 0U, g_label_voltage, 2U, 0x0600U);
        ui_draw_meter_text_on_line(row, 57U, 0U, voltage, 'V', UI_COLOR_MAIN_GREEN);

        ui_draw_row_label_chip_on_line(row, 1U, 27U, g_label_current, 2U, 0x0015U);
        ui_draw_meter_text_on_line(row, 57U, 27U, current, 'A', UI_COLOR_MAIN_CYAN);

        ui_draw_row_label_chip_on_line(row, 1U, 54U, g_label_power, 2U, 0x8200U);
        ui_draw_meter_text_on_line(row, 57U, 54U, power, 'W', UI_COLOR_MAIN_AMBER);
        ui_end_row(row);
    }
}

static void ui_draw_dpdm_flow_arrow_on_line(uint16_t row,
                                            uint16_t x,
                                            uint16_t y,
                                            uint16_t width,
                                            uint8_t frame,
                                            uint8_t reverse,
                                            uint16_t color)
{
    uint16_t usable_x;
    uint16_t usable_w;
    uint16_t center;
    uint16_t line_y;
    uint8_t phase;

    if ((row < y) || (row >= (uint16_t)(y + 15U)))
    {
        return;
    }

    if (width < 16U)
    {
        return;
    }

    usable_x = (uint16_t)(x + 1U);
    usable_w = (uint16_t)(width - 2U);
    line_y = (uint16_t)(y + 8U);
    phase = (uint8_t)(frame % 12U);

    if (reverse != 0U)
    {
        center = (uint16_t)(usable_x + usable_w - 6U - phase);
        while (center > (uint16_t)(usable_x + 4U))
        {
            if (row == line_y || row == (uint16_t)(line_y + 1U))
            {
                uint16_t col;

                for (col = (uint16_t)(center + 2U); col <= (uint16_t)(center + 7U); ++col)
                {
                    if (col < (uint16_t)(usable_x + usable_w))
                    {
                        ui_put_pixel(col, color);
                    }
                }
            }
            if (row == (uint16_t)(line_y - 3U)) { ui_put_pixel((uint16_t)(center + 3U), color); }
            if (row == (uint16_t)(line_y - 2U)) { ui_put_pixel((uint16_t)(center + 2U), color); }
            if (row == (uint16_t)(line_y - 1U)) { ui_put_pixel((uint16_t)(center + 1U), color); }
            if (row == line_y || row == (uint16_t)(line_y + 1U)) { ui_put_pixel(center, color); }
            if (row == (uint16_t)(line_y + 2U)) { ui_put_pixel((uint16_t)(center + 1U), color); }
            if (row == (uint16_t)(line_y + 3U)) { ui_put_pixel((uint16_t)(center + 2U), color); }
            if (row == (uint16_t)(line_y + 4U)) { ui_put_pixel((uint16_t)(center + 3U), color); }

            center = (uint16_t)(center - 12U);
        }
        return;
    }

    center = (uint16_t)(usable_x + 5U + phase);
    while (center < (uint16_t)(usable_x + usable_w - 4U))
    {
        if (row == line_y || row == (uint16_t)(line_y + 1U))
        {
            uint16_t col;

            for (col = (uint16_t)(center - 7U); col < (uint16_t)(center - 1U); ++col)
            {
                if (col >= usable_x)
                {
                    ui_put_pixel(col, color);
                }
            }
        }
        if (row == (uint16_t)(line_y - 3U)) { ui_put_pixel((uint16_t)(center - 3U), color); }
        if (row == (uint16_t)(line_y - 2U)) { ui_put_pixel((uint16_t)(center - 2U), color); }
        if (row == (uint16_t)(line_y - 1U)) { ui_put_pixel((uint16_t)(center - 1U), color); }
        if (row == line_y || row == (uint16_t)(line_y + 1U)) { ui_put_pixel(center, color); }
        if (row == (uint16_t)(line_y + 2U)) { ui_put_pixel((uint16_t)(center - 1U), color); }
        if (row == (uint16_t)(line_y + 3U)) { ui_put_pixel((uint16_t)(center - 2U), color); }
        if (row == (uint16_t)(line_y + 4U)) { ui_put_pixel((uint16_t)(center - 3U), color); }

        center = (uint16_t)(center + 12U);
    }
}

void ui_renderer_draw_dpdm_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    char voltage[8];
    char current[8];
    char power[8];
    char dp[6];
    char dm[6];
    char dp_line[8];
    char dm_line[8];
    char protocol_line[8];
    const char *detected;
    uint8_t reverse_arrow;
    uint8_t frame;
    uint16_t row;

    ui_format_meter_5digits(voltage, sizeof(voltage), ui_measure_voltage_mv(measure));
    ui_format_current_5digits(current, sizeof(current), ui_measure_current_deci_ma(measure));
    ui_format_meter_5digits(power, sizeof(power), ui_measure_power_mw(measure));
    ui_format_dpdm_voltage(dp, sizeof(dp), (protocol != NULL) ? protocol->dp_mv : 0);
    ui_format_dpdm_voltage(dm, sizeof(dm), (protocol != NULL) ? protocol->dm_mv : 0);
    detected = ui_infer_dpdm_protocol(protocol);
    dp_line[0] = 'D';
    dp_line[1] = '+';
    dp_line[2] = dp[0];
    dp_line[3] = dp[1];
    dp_line[4] = dp[2];
    dp_line[5] = dp[3];
    dp_line[6] = '\0';
    dm_line[0] = 'D';
    dm_line[1] = '-';
    dm_line[2] = dm[0];
    dm_line[3] = dm[1];
    dm_line[4] = dm[2];
    dm_line[5] = dm[3];
    dm_line[6] = '\0';
    protocol_line[0] = ' ';
    protocol_line[1] = ' ';
    protocol_line[2] = ' ';
    protocol_line[3] = ' ';
    protocol_line[4] = ' ';
    protocol_line[5] = ' ';
    protocol_line[6] = '\0';
    if (detected[0] != '\0')
    {
        uint8_t len;
        uint8_t start;
        uint8_t index;

        len = 0U;
        while ((detected[len] != '\0') && (len < 6U))
        {
            ++len;
        }
        start = (uint8_t)((6U - len) / 2U);
        for (index = 0U; index < len; ++index)
        {
            protocol_line[start + index] = detected[index];
        }
    }
    frame = ui_model_liveness_frame(state);
    reverse_arrow = 1U;
    if ((measure != NULL) && (measure->current_valid != 0U) && (measure->current_avg_ma < 0))
    {
        reverse_arrow ^= 1U;
    }
    if (ui_model_rotation_degrees(state) == 180U)
    {
        reverse_arrow ^= 1U;
    }

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_meter_text_on_line(row, 1U, 0U, voltage, 'V', UI_COLOR_MAIN_GREEN);
        ui_draw_meter_text_on_line(row, 1U, 27U, current, 'A', UI_COLOR_MAIN_CYAN);
        ui_draw_meter_text_on_line(row, 1U, 54U, power, 'W', UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 104U, 0U, 55U, 39U, UI_COLOR_DPDM_TOP_BG);
        ui_draw_rect_on_line(row, 104U, 41U, 55U, 39U, UI_COLOR_DPDM_BOTTOM_BG);
        ui_draw_mono_15_on_line(row, 105U, 3U, dp_line, UI_COLOR_DPDM_LINE);
        ui_draw_mono_15_on_line(row, 105U, 21U, dm_line, UI_COLOR_DPDM_LINE);
        ui_draw_mono_15_on_line(row, 105U, 44U, protocol_line, UI_COLOR_DPDM_PROTOCOL);
        ui_draw_dpdm_flow_arrow_on_line(row, 105U, 62U, 53U, frame, reverse_arrow, UI_COLOR_DPDM_ARROW);
        ui_end_row(row);
    }
}

void ui_renderer_draw_power_stats_page(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure)
{
    char voltage[8];
    char current[8];
    char power[8];
    char stat_v[8];
    char stat_i[8];
    char stat_p[8];
    uint8_t average;
    uint16_t row;

    average = ui_model_power_stats_average(state);
    ui_format_meter_5digits(voltage, sizeof(voltage), ui_measure_voltage_mv(measure));
    ui_format_current_5digits(current, sizeof(current), ui_measure_current_deci_ma(measure));
    ui_format_meter_5digits(power, sizeof(power), ui_measure_power_mw(measure));

    ui_format_meter_4digits(stat_v,
                            sizeof(stat_v),
                            (measure != NULL) ? (average ? measure->stat_voltage_avg_mv : measure->stat_voltage_max_mv) : 0);
    ui_format_current_4digits(stat_i,
                              sizeof(stat_i),
                              ui_measure_stat_current_deci_ma(measure, average));
    ui_format_meter_4digits(stat_p,
                            sizeof(stat_p),
                            (measure != NULL) ? (average ? measure->stat_power_avg_mw : measure->stat_power_max_mw) : 0);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_meter_text_on_line(row, 1U, 0U, voltage, 'V', UI_COLOR_MAIN_GREEN);
        ui_draw_meter_text_on_line(row, 1U, 27U, current, 'A', UI_COLOR_MAIN_CYAN);
        ui_draw_meter_text_on_line(row, 1U, 54U, power, 'W', UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 104U, 0U, 55U, 21U, UI_COLOR_STATS_TOP_BG);
        ui_draw_rect_on_line(row, 104U, 23U, 55U, 57U, UI_COLOR_STATS_BOTTOM_BG);
        ui_draw_zh_15_on_line(row, 116U, 3U, average ? g_label_average : g_label_max, 2U, UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 104U, 27U, stat_v, UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 147U, 27U, "V", UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 104U, 45U, stat_i, UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 147U, 45U, "A", UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 104U, 63U, stat_p, UI_COLOR_TEXT);
        ui_draw_mono_15_on_line(row, 147U, 63U, "W", UI_COLOR_TEXT);
        ui_end_row(row);
    }
}

void ui_renderer_draw_capacity_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure)
{
    char voltage[8];
    char current[8];
    char power[8];
    char time_text[10];
    char value_text[10];
    const char *unit_text;
    uint8_t show_wh;
    uint16_t row;

    ui_format_meter_5digits(voltage, sizeof(voltage), ui_measure_voltage_mv(measure));
    ui_format_current_5digits(current, sizeof(current), ui_measure_current_deci_ma(measure));
    ui_format_meter_5digits(power, sizeof(power), ui_measure_power_mw(measure));
    ui_format_time(time_text, sizeof(time_text), (measure != NULL) ? measure->stat_elapsed_s : 0U);
    show_wh = ui_model_capacity_show_wh(state);
    if (show_wh != 0U)
    {
        unit_text = "Wh";
        ui_format_wh(value_text, sizeof(value_text), (measure != NULL) ? measure->stat_energy_mwh : 0U);
    }
    else
    {
        unit_text = "mAh";
        ui_format_capacity(value_text,
                           sizeof(value_text),
                           ui_capacity_mah_from_4v2((measure != NULL) ? measure->stat_energy_mwh : 0U));
    }

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_draw_meter_text_on_line(row, 1U, 0U, voltage, 'V', UI_COLOR_MAIN_GREEN);
        ui_draw_meter_text_on_line(row, 1U, 27U, current, 'A', UI_COLOR_MAIN_CYAN);
        ui_draw_meter_text_on_line(row, 1U, 54U, power, 'W', UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 104U, 0U, 55U, 39U, UI_COLOR_DPDM_TOP_BG);
        ui_draw_rect_on_line(row, 104U, 41U, 55U, 39U, UI_COLOR_DPDM_BOTTOM_BG);
        ui_draw_zh_15_on_line(row,
                              ui_center_x(104U, 55U, ui_zh_15_text_width(g_label_stat_time, 4U)),
                              3U,
                              g_label_stat_time,
                              4U,
                              UI_COLOR_TEXT);
        ui_draw_mono_12_on_line(row, ui_center_x(104U, 55U, ui_mono_12_text_width(time_text)), 23U, time_text, UI_COLOR_TEXT);
        ui_draw_mono_15_baseline_on_line(row, ui_center_x(104U, 55U, ui_mono_15_text_width(unit_text)), 44U, unit_text, UI_COLOR_TEXT);
        ui_draw_mono_15_baseline_on_line(row, ui_center_x(104U, 55U, ui_mono_15_text_width(value_text)), 62U, value_text, UI_COLOR_TEXT);
        ui_end_row(row);
    }
}

void ui_renderer_draw_scope_page(const measure_snapshot_t *measure)
{
    char voltage[8];
    char current[8];
    char power[8];
    uint16_t row;
    uint16_t col;
    uint16_t point_count;
    int32_t voltage_min_mv;
    int32_t voltage_max_mv;
    int32_t current_min_ma;
    int32_t current_max_ma;
    int32_t display_voltage;
    int32_t display_current;

    ui_renderer_update_scope_history(measure);
    display_voltage = ui_measure_voltage_mv(measure);
    display_current = ui_measure_current_ma(measure);
    ui_format_meter_2dec(voltage, sizeof(voltage), display_voltage);
    ui_format_meter_2dec(current, sizeof(current), display_current);
    ui_format_meter_2dec(power, sizeof(power), ui_measure_power_mw(measure));
    ui_append_unit_char(voltage, sizeof(voltage), 'V');
    ui_append_unit_char(current, sizeof(current), 'A');
    ui_append_unit_char(power, sizeof(power), 'W');
    point_count = (g_scope_history.count >= 2U) ? g_scope_history.count : UI_SCOPE_HISTORY_CAPACITY;
    ui_scope_history_range(0U, display_voltage, &voltage_min_mv, &voltage_max_mv);
    ui_scope_history_range(1U, display_current, &current_min_ma, &current_max_ma);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);

        ui_draw_rect_on_line(row, 1U, 1U, 158U, 2U, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, 1U, 67U, 158U, 2U, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, 1U, 1U, 2U, 68U, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, 157U, 1U, 2U, 68U, UI_COLOR_SCOPE_GRID);
        for (col = 1U; col < 16U; ++col)
        {
            ui_draw_rect_on_line(row, (uint16_t)(1U + col * 10U), 3U, 1U, 64U, UI_COLOR_SCOPE_GRID);
        }
        for (col = 1U; col < 7U; ++col)
        {
            ui_draw_rect_on_line(row, 3U, (uint16_t)(2U + col * 10U), 154U, 1U, UI_COLOR_SCOPE_GRID);
        }

        for (col = 0U; col < point_count; ++col)
        {
            uint16_t x;
            uint16_t voltage_y;
            uint16_t current_y;
            uint16_t prev_voltage_y;
            uint16_t prev_current_y;
            uint16_t y_min;
            uint16_t y_max;
            int32_t point_voltage;
            int32_t point_current;
            int32_t prev_voltage;
            int32_t prev_current;

            x = col;
            point_voltage = (g_scope_history.count >= 2U) ? g_scope_history.voltage_mv[col] : display_voltage;
            point_current = (g_scope_history.count >= 2U) ? g_scope_history.current_ma[col] : display_current;
            voltage_y = ui_scope_value_to_y(point_voltage, voltage_min_mv, voltage_max_mv);
            current_y = ui_scope_value_to_y(ui_scope_abs_i32(point_current), current_min_ma, current_max_ma);

            if (col == 0U)
            {
                prev_voltage_y = voltage_y;
                prev_current_y = current_y;
            }
            else
            {
                prev_voltage = (g_scope_history.count >= 2U) ?
                               g_scope_history.voltage_mv[(uint16_t)(col - 1U)] :
                               display_voltage;
                prev_current = (g_scope_history.count >= 2U) ?
                               g_scope_history.current_ma[(uint16_t)(col - 1U)] :
                               display_current;
                prev_voltage_y = ui_scope_value_to_y(prev_voltage, voltage_min_mv, voltage_max_mv);
                prev_current_y = ui_scope_value_to_y(ui_scope_abs_i32(prev_current), current_min_ma, current_max_ma);
            }

            y_min = (voltage_y < prev_voltage_y) ? voltage_y : prev_voltage_y;
            y_max = (voltage_y > prev_voltage_y) ? voltage_y : prev_voltage_y;
            ui_draw_rect_on_line(row, x, y_min, 1U, (uint16_t)(y_max - y_min + 1U), UI_COLOR_SCOPE_VOLTAGE);
            y_min = (current_y < prev_current_y) ? current_y : prev_current_y;
            y_max = (current_y > prev_current_y) ? current_y : prev_current_y;
            ui_draw_rect_on_line(row, x, y_min, 1U, (uint16_t)(y_max - y_min + 1U), UI_COLOR_SCOPE_CURRENT);
        }

        ui_draw_mono_9_on_line(row,
                               (uint16_t)(158U - ui_mono_9_text_width(voltage)),
                               4U,
                               voltage,
                               UI_COLOR_SCOPE_VOLTAGE);
        ui_draw_mono_9_on_line(row,
                               (uint16_t)(158U - ui_mono_9_text_width(current)),
                               14U,
                               current,
                               UI_COLOR_SCOPE_CURRENT);

        ui_draw_mono_9_on_line(row, 2U, 70U, voltage, UI_COLOR_SCOPE_VOLTAGE);
        ui_draw_mono_9_on_line(row, 47U, 70U, current, UI_COLOR_SCOPE_CURRENT);
        ui_draw_mono_9_on_line(row, 94U, 70U, power, UI_COLOR_SCOPE_POWER);
        ui_draw_mono_9_on_line(row, 140U, 70U, "5US", UI_COLOR_TEXT);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_ripple_page(const ui_model_state_t *state,
                                  const measure_snapshot_t *measure)
{
    enum
    {
        RIPPLE_TAG_Y = 2U,
        RIPPLE_TAG_H = 11U,
        RIPPLE_GRAPH_X = 2U,
        RIPPLE_GRAPH_Y = 14U,
        RIPPLE_GRAPH_W = 156U,
        RIPPLE_GRAPH_H = 53U,
        RIPPLE_BOTTOM_Y = 68U
    };
    char ripple_text[8];
    char freq_text[8];
    char baseline_text[8];
    char range_text[8];
    uint16_t row;
    uint16_t col;
    uint16_t point_count;
    uint16_t inner_width;
    uint32_t graph_ripple_mv;
    int32_t graph_max_mv;
    int32_t graph_min_mv;
    int32_t graph_mid_mv;
    int32_t graph_span_mv;
    int32_t baseline_mv;

    ui_renderer_update_ripple_history(state, measure);
    ui_format_frequency_short(freq_text,
                              sizeof(freq_text),
                              ui_model_ripple_frequency_hz(state));
    ui_format_uint_suffix(range_text, sizeof(range_text), 50U, "MV");

    baseline_mv = ui_measure_voltage_mv(measure);
    graph_min_mv = baseline_mv;
    graph_max_mv = baseline_mv;
    if (g_ripple_history.count != 0U)
    {
        int32_t sum_mv;

        graph_min_mv = g_ripple_history.voltage_mv[0];
        graph_max_mv = graph_min_mv;
        sum_mv = graph_min_mv;
        for (col = 1U; col < g_ripple_history.count; ++col)
        {
            int32_t value_mv;

            value_mv = g_ripple_history.voltage_mv[col];
            sum_mv += value_mv;
            if (value_mv < graph_min_mv)
            {
                graph_min_mv = value_mv;
            }
            if (value_mv > graph_max_mv)
            {
                graph_max_mv = value_mv;
            }
        }
        baseline_mv = (int32_t)(sum_mv / g_ripple_history.count);
    }
    graph_ripple_mv = (uint32_t)(graph_max_mv - graph_min_mv);
    graph_mid_mv = (graph_max_mv + graph_min_mv) / 2;
    graph_span_mv = graph_max_mv - graph_min_mv;
    if (graph_span_mv < 50)
    {
        graph_span_mv = 50;
    }
    graph_min_mv = graph_mid_mv - (graph_span_mv / 2);
    graph_max_mv = graph_min_mv + graph_span_mv;
    point_count = (g_ripple_history.count >= 2U) ? g_ripple_history.count : UI_SCOPE_HISTORY_CAPACITY;
    inner_width = (uint16_t)(RIPPLE_GRAPH_W - 2U);
    ui_format_uint_suffix(ripple_text, sizeof(ripple_text), graph_ripple_mv, "MV");
    ui_format_meter_2dec(baseline_text, sizeof(baseline_text), baseline_mv);
    ui_append_unit_char(baseline_text, sizeof(baseline_text), 'V');

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);

        ui_draw_mono_9_chip_on_line(row, 2U, RIPPLE_TAG_Y, "P", 15U, 0x0600U);
        ui_draw_mono_9_on_line(row, 18U, (uint16_t)(RIPPLE_TAG_Y + 1U), ripple_text, UI_COLOR_TEXT);
        ui_draw_mono_9_chip_on_line(row, 52U, RIPPLE_TAG_Y, "F", 15U,
                                    ((state != NULL) && (ui_model_action_mode(state) != 0U)) ? 0x8200U : 0x0015U);
        ui_draw_mono_9_on_line(row, 68U, (uint16_t)(RIPPLE_TAG_Y + 1U), freq_text,
                               ((state != NULL) && (ui_model_ripple_paused(state) != 0U)) ? UI_COLOR_AMBER : UI_COLOR_TEXT);
        ui_draw_mono_9_chip_on_line(row, 102U, RIPPLE_TAG_Y, "B", 15U, 0x0600U);
        ui_draw_mono_9_on_line(row, 118U, (uint16_t)(RIPPLE_TAG_Y + 1U), baseline_text, UI_COLOR_TEXT);

        ui_draw_rect_on_line(row, RIPPLE_GRAPH_X, RIPPLE_GRAPH_Y, RIPPLE_GRAPH_W, 1U, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, RIPPLE_GRAPH_X, (uint16_t)(RIPPLE_GRAPH_Y + RIPPLE_GRAPH_H - 1U), RIPPLE_GRAPH_W, 1U, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, RIPPLE_GRAPH_X, RIPPLE_GRAPH_Y, 1U, RIPPLE_GRAPH_H, UI_COLOR_SCOPE_GRID);
        ui_draw_rect_on_line(row, (uint16_t)(RIPPLE_GRAPH_X + RIPPLE_GRAPH_W - 1U), RIPPLE_GRAPH_Y, 1U, RIPPLE_GRAPH_H, UI_COLOR_SCOPE_GRID);
        for (col = 1U; col < 15U; ++col)
        {
            ui_draw_rect_on_line(row, (uint16_t)(RIPPLE_GRAPH_X + col * 10U), (uint16_t)(RIPPLE_GRAPH_Y + 1U), 1U, (uint16_t)(RIPPLE_GRAPH_H - 2U), UI_COLOR_SCOPE_GRID);
        }
        for (col = 1U; col < 5U; ++col)
        {
            ui_draw_rect_on_line(row, (uint16_t)(RIPPLE_GRAPH_X + 1U), (uint16_t)(RIPPLE_GRAPH_Y + col * 10U), (uint16_t)(RIPPLE_GRAPH_W - 2U), 1U, UI_COLOR_SCOPE_GRID);
        }

        for (col = 0U; col < point_count; ++col)
        {
            uint16_t x;
            uint16_t prev_x;
            uint16_t y;
            uint16_t prev_y;
            uint16_t x_min;
            uint16_t x_max;
            uint16_t y_min;
            uint16_t y_max;
            int32_t value;
            int32_t prev_value;

            x = (g_ripple_history.count >= 2U) ?
                (uint16_t)(RIPPLE_GRAPH_X + 1U + (((uint32_t)(inner_width - 1U) * col) / (uint32_t)(UI_SCOPE_HISTORY_CAPACITY - 1U))) :
                (uint16_t)(RIPPLE_GRAPH_X + 1U + (((uint32_t)(inner_width - 1U) * col) / (uint32_t)(UI_SCOPE_HISTORY_CAPACITY - 1U)));
            value = (g_ripple_history.count >= 2U) ? g_ripple_history.voltage_mv[col] : baseline_mv;
            y = ui_scope_value_to_y_region(value, graph_min_mv, graph_max_mv, (uint16_t)(RIPPLE_GRAPH_Y + 2U), (uint16_t)(RIPPLE_GRAPH_H - 4U));
            if ((col == 0U) ||
                ((g_ripple_history.count >= UI_SCOPE_HISTORY_CAPACITY) &&
                 (col == g_ripple_history.next_index)))
            {
                prev_x = x;
                prev_y = y;
            }
            else
            {
                prev_x = (uint16_t)(RIPPLE_GRAPH_X + 1U + (((uint32_t)(inner_width - 1U) * (uint16_t)(col - 1U)) / (uint32_t)(UI_SCOPE_HISTORY_CAPACITY - 1U)));
                prev_value = (g_ripple_history.count >= 2U) ?
                             g_ripple_history.voltage_mv[(uint16_t)(col - 1U)] :
                             baseline_mv;
                prev_y = ui_scope_value_to_y_region(prev_value, graph_min_mv, graph_max_mv, (uint16_t)(RIPPLE_GRAPH_Y + 2U), (uint16_t)(RIPPLE_GRAPH_H - 4U));
            }
            x_min = (x < prev_x) ? x : prev_x;
            x_max = (x > prev_x) ? x : prev_x;
            y_min = (y < prev_y) ? y : prev_y;
            y_max = (y > prev_y) ? y : prev_y;
            ui_draw_rect_on_line(row, x_min, y_min, (uint16_t)(x_max - x_min + 1U), (uint16_t)(y_max - y_min + 1U), UI_COLOR_SCOPE_CURRENT);
        }
        if (g_ripple_history.count >= 2U)
        {
            uint16_t sweep_x;

            sweep_x = (uint16_t)(RIPPLE_GRAPH_X + 1U +
                                 (((uint32_t)(inner_width - 1U) * g_ripple_history.next_index) /
                                  (uint32_t)(UI_SCOPE_HISTORY_CAPACITY - 1U)));
            ui_draw_rect_on_line(row, sweep_x, (uint16_t)(RIPPLE_GRAPH_Y + 1U), 1U, (uint16_t)(RIPPLE_GRAPH_H - 2U), UI_COLOR_AMBER);
        }

        ui_draw_mono_9_chip_on_line(row, 2U, RIPPLE_BOTTOM_Y, "R", 15U, 0x0600U);
        ui_draw_mono_9_on_line(row, 18U, (uint16_t)(RIPPLE_BOTTOM_Y + 1U), range_text, UI_COLOR_TEXT);
        ui_draw_mono_9_chip_on_line(row, 62U, RIPPLE_BOTTOM_Y, "H", 15U, 0x0015U);
        ui_draw_mono_9_on_line(row, 78U, (uint16_t)(RIPPLE_BOTTOM_Y + 1U), "5US", UI_COLOR_TEXT);
        ui_draw_mono_9_chip_on_line(row, 132U, RIPPLE_BOTTOM_Y, "AC", 24U, 0x3010U);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

typedef struct
{
    const char *name;
    uint8_t supported;
} ui_protocol_row_t;

static int16_t ui_abs_i16(int16_t value)
{
    return (value < 0) ? (int16_t)-value : value;
}

static uint8_t ui_protocol_dpdm_valid(const protocol_snapshot_t *protocol)
{
    if (protocol == NULL)
    {
        return 0U;
    }

    return ((protocol->dp_mv > 0) || (protocol->dm_mv > 0)) ? 1U : 0U;
}

static uint8_t ui_protocol_is_dcp(const protocol_snapshot_t *protocol)
{
    int16_t avg;

    if (ui_protocol_dpdm_valid(protocol) == 0U)
    {
        return 0U;
    }

    avg = (int16_t)(((int32_t)protocol->dp_mv + protocol->dm_mv) / 2);
    return ((ui_abs_i16((int16_t)(protocol->dp_mv - protocol->dm_mv)) <= 180) &&
            ui_mv_between(avg, 350, 1200)) ? 1U : 0U;
}

static uint8_t ui_protocol_is_apple(const protocol_snapshot_t *protocol)
{
    if (ui_protocol_dpdm_valid(protocol) == 0U)
    {
        return 0U;
    }

    return ((ui_mv_between(protocol->dp_mv, 1800, 2200) && ui_mv_between(protocol->dm_mv, 1800, 2200)) ||
            (ui_mv_between(protocol->dp_mv, 2500, 2900) && ui_mv_between(protocol->dm_mv, 3000, 3500))) ? 1U : 0U;
}

static uint8_t ui_protocol_is_samsung(const protocol_snapshot_t *protocol)
{
    if (ui_protocol_dpdm_valid(protocol) == 0U)
    {
        return 0U;
    }

    return (ui_mv_between(protocol->dp_mv, 1050, 1350) &&
            ui_mv_between(protocol->dm_mv, 1050, 1350)) ? 1U : 0U;
}

static uint8_t ui_protocol_typec_supported(const protocol_snapshot_t *protocol)
{
    if (protocol == NULL)
    {
        return 0U;
    }

    return ((protocol->cc_attached != 0U) ||
            (protocol->cc_orientation != 0U) ||
            (protocol->cc_active_mask != 0U)) ? 1U : 0U;
}

static uint8_t ui_protocol_fill_rows(const protocol_snapshot_t *protocol,
                                     ui_protocol_row_t *rows,
                                     uint8_t max_rows)
{
    uint8_t count;
    protocol_kind_t kind;

    if ((rows == NULL) || (max_rows < 14U))
    {
        return 0U;
    }

    kind = (protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE;
    count = 0U;
    rows[count++] = (ui_protocol_row_t){ "PD3.0", (uint8_t)(((kind == PROTOCOL_KIND_PD) || ((protocol != NULL) && (protocol->source_fixed_count != 0U))) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "PPS", (uint8_t)(((protocol != NULL) && (protocol->pps_present != 0U)) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "QC2.0", (uint8_t)((kind == PROTOCOL_KIND_QC) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "QC3.0", (uint8_t)(((kind == PROTOCOL_KIND_QC) && ((protocol->legacy_step_offset != 0) || (protocol->target_mv > 5000))) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "FCP", (uint8_t)((kind == PROTOCOL_KIND_FCP) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "SCP", 0U };
    rows[count++] = (ui_protocol_row_t){ "AFC", (uint8_t)((kind == PROTOCOL_KIND_AFC) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "DCP", ui_protocol_is_dcp(protocol) };
    rows[count++] = (ui_protocol_row_t){ "CDP", 0U };
    rows[count++] = (ui_protocol_row_t){ "SDP", 0U };
    rows[count++] = (ui_protocol_row_t){ "APPLE", ui_protocol_is_apple(protocol) };
    rows[count++] = (ui_protocol_row_t){ "SAMS", ui_protocol_is_samsung(protocol) };
    rows[count++] = (ui_protocol_row_t){ "E-MARK", (uint8_t)(((protocol != NULL) && (protocol->emark_present != 0U)) ? 1U : 0U) };
    rows[count++] = (ui_protocol_row_t){ "TYPE-C", ui_protocol_typec_supported(protocol) };
    return count;
}

static void ui_draw_protocol_warning_button_on_line(uint16_t row,
                                                    uint16_t x,
                                                    const uint16_t *label,
                                                    uint8_t label_count,
                                                    uint8_t selected,
                                                    uint16_t selected_bg)
{
    uint16_t bg;

    bg = (selected != 0U) ? selected_bg : UI_COLOR_PANEL;
    ui_draw_rect_on_line(row, x, 62U, 62U, 16U, bg);
    ui_draw_box_on_line(row, x, 62U, 62U, 16U, (selected != 0U) ? UI_COLOR_TEXT : UI_COLOR_FRAME);
    ui_draw_zh_15_on_line(row,
                          ui_center_x(x, 62U, ui_zh_15_text_width(label, label_count)),
                          63U,
                          label,
                          label_count,
                          UI_COLOR_TEXT);
}

void ui_renderer_draw_protocol_warning_page(const ui_model_state_t *state)
{
    uint8_t confirm_selected;
    uint16_t row;

    confirm_selected = ui_model_protocol_warning_confirm_selected(state);
    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_rect_on_line(row, 1U, 1U, 158U, 78U, UI_COLOR_BLACK);
        ui_draw_zh_15_on_line(row, 5U, 3U, g_label_protocol_detect, 4U, UI_COLOR_MAIN_CYAN);
        ui_draw_mono_15_baseline_on_line(row, 111U, 3U, "WARN", UI_COLOR_RED);
        ui_draw_rect_on_line(row, 4U, 19U, 152U, 1U, UI_COLOR_FRAME);
        ui_draw_zh_15_on_line(row,
                              ui_center_x(0U, LCD_WIDTH, ui_zh_15_text_width(g_label_remove_load_zh, 4U)),
                              26U,
                              g_label_remove_load_zh,
                              4U,
                              UI_COLOR_MAIN_AMBER);
        ui_draw_mono_15_baseline_on_line(row,
                                         ui_center_x(0U, LCD_WIDTH, ui_mono_15_text_width("RISK")),
                                         45U,
                                         "RISK",
                                         UI_COLOR_RED);
        ui_draw_protocol_warning_button_on_line(row,
                                                17U,
                                                g_label_confirm_zh,
                                                2U,
                                                confirm_selected,
                                                UI_COLOR_MAIN_GREEN);
        ui_draw_protocol_warning_button_on_line(row,
                                                81U,
                                                g_label_cancel_zh,
                                                2U,
                                                (uint8_t)(confirm_selected == 0U),
                                                UI_COLOR_MAIN_PURPLE);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_protocol_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure,
                                    const protocol_snapshot_t *protocol)
{
    ui_protocol_row_t items[14];
    uint8_t count;
    uint8_t offset;
    uint16_t row;

    (void)measure;
    count = ui_protocol_fill_rows(protocol, items, (uint8_t)(sizeof(items) / sizeof(items[0])));
    offset = ui_model_protocol_scroll(state);
    if (count <= 5U)
    {
        offset = 0U;
    }
    else if (offset > (uint8_t)(count - 5U))
    {
        offset = (uint8_t)(count - 5U);
    }

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        uint8_t index;

        ui_begin_row(row);
        ui_draw_rect_on_line(row, 1U, 1U, 158U, 78U, UI_COLOR_BLACK);
        ui_draw_zh_15_on_line(row, 4U, 1U, g_label_protocol_detect, 4U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 3U, 16U, 154U, 1U, UI_COLOR_FRAME);
        for (index = 0U; index < 5U; ++index)
        {
            uint16_t y;
            uint8_t item_index;
            uint16_t color;
            uint16_t bg;

            item_index = (uint8_t)(offset + index);
            if (item_index >= count)
            {
                continue;
            }
            y = (uint16_t)(18U + index * 12U);
            color = items[item_index].supported ? UI_COLOR_MAIN_GREEN : UI_COLOR_RED;
            bg = items[item_index].supported ? 0x0104U : 0x1800U;
            ui_draw_rect_on_line(row, 3U, y, 71U, 11U, bg);
            ui_draw_rect_on_line(row, 77U, y, 78U, 11U, UI_COLOR_BLACK);
            ui_draw_rect_on_line(row, 3U, (uint16_t)(y + 11U), 152U, 1U, UI_COLOR_PANEL);
            ui_draw_mono_12_on_line(row,
                                    ui_center_x(5U, 66U, ui_mono_12_text_width(items[item_index].name)),
                                    (uint16_t)(y - 1U),
                                    items[item_index].name,
                                    color);
            ui_draw_mono_12_on_line(row,
                                    ui_center_x(86U, 61U, ui_mono_12_text_width(items[item_index].supported ? "SUP" : "--")),
                                    (uint16_t)(y - 1U),
                                    items[item_index].supported ? "SUP" : "--",
                                    color);
        }
        if (offset > 0U)
        {
            ui_draw_mono_12_on_line(row, 150U, 1U, "+", UI_COLOR_TEXT);
        }
        if ((count > 5U) && (offset < (uint8_t)(count - 5U)))
        {
            ui_draw_mono_12_on_line(row, 150U, 67U, "-", UI_COLOR_TEXT);
        }
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

static void ui_append_hex_nibble(char *out, uint8_t *pos, uint8_t limit, uint8_t value)
{
    value &= 0x0FU;
    ui_append_char(out, pos, limit, (char)((value < 10U) ? ('0' + value) : ('A' + value - 10U)));
}

static void ui_append_hex16(char *out, uint8_t *pos, uint8_t limit, uint16_t value)
{
    ui_append_hex_nibble(out, pos, limit, (uint8_t)(value >> 12));
    ui_append_hex_nibble(out, pos, limit, (uint8_t)(value >> 8));
    ui_append_hex_nibble(out, pos, limit, (uint8_t)(value >> 4));
    ui_append_hex_nibble(out, pos, limit, (uint8_t)value);
}

static void ui_format_pd_diag_line(char *out,
                                   uint8_t limit,
                                   const char *a,
                                   uint32_t av,
                                   const char *b,
                                   uint32_t bv,
                                   const char *c,
                                   uint32_t cv)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), a);
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), av, 1U);
    ui_append_char(out, &pos, (uint8_t)(limit - 1U), ' ');
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), b);
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), bv, 1U);
    if (c != NULL)
    {
        ui_append_char(out, &pos, (uint8_t)(limit - 1U), ' ');
        ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), c);
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), cv, 1U);
    }
    out[pos] = '\0';
}

static void ui_format_pd_diag_header_line(char *out, uint8_t limit, const bsp_usbpd_port_diag_t *diag)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), "LEN");
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (diag != NULL) ? diag->last_len : 0U, 1U);
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), " H");
    ui_append_hex16(out, &pos, (uint8_t)(limit - 1U), (diag != NULL) ? diag->last_header : 0U);
    ui_append_ascii_text(out, &pos, (uint8_t)(limit - 1U), " T");
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (diag != NULL) ? diag->last_msg_type : 0U, 1U);
    out[pos] = '\0';
}

void ui_renderer_draw_pdo_page(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol)
{
    uint16_t row;
    service_pd_source_caps_snapshot_t caps;
    uint8_t count;
    uint8_t scroll;
    uint8_t max_scroll;
    char header[8];
    uint8_t pos;
    bsp_usbpd_port_diag_t diag;

    (void)protocol;
    service_pd_copy_source_caps(&caps);
    bsp_usbpd_port_copy_diag(&diag);
    count = caps.count;
    scroll = ui_model_pdo_scroll(state);
    max_scroll = (count > 7U) ? (uint8_t)(count - 7U) : 0U;
    if (scroll > max_scroll)
    {
        scroll = max_scroll;
    }

    pos = 0U;
    header[pos++] = 'P';
    header[pos++] = 'D';
    header[pos++] = 'O';
    header[pos++] = ':';
    ui_append_uint(header, &pos, (uint8_t)(sizeof(header) - 1U), count, 1U);
    header[pos] = '\0';

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        uint8_t index;

        ui_begin_row(row);
        ui_draw_mono_9_on_line(row, 3U, 1U, header, UI_COLOR_MAIN_CYAN);
        if (count > 7U)
        {
            char page_text[6];

            pos = 0U;
            ui_append_uint(page_text, &pos, (uint8_t)(sizeof(page_text) - 1U), (uint32_t)(scroll + 1U), 1U);
            ui_append_char(page_text, &pos, (uint8_t)(sizeof(page_text) - 1U), '/');
            ui_append_uint(page_text, &pos, (uint8_t)(sizeof(page_text) - 1U), (uint32_t)(max_scroll + 1U), 1U);
            page_text[pos] = '\0';
            ui_draw_mono_9_on_line(row,
                                   (uint16_t)(157U - ui_mono_9_text_width(page_text)),
                                   1U,
                                   page_text,
                                   UI_COLOR_TEXT);
        }

        if (count == 0U)
        {
            char diag_line[22];

            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "RX", diag.rx_total,
                                   "Q", diag.queue_depth,
                                   "CC", diag.cc_orientation);
            ui_draw_mono_9_on_line(row, 3U, 13U, diag_line, UI_COLOR_TEXT);

            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "S0", diag.rx_sop0,
                                   "S1", diag.rx_sop1,
                                   "G", diag.rx_goodcrc);
            ui_draw_mono_9_on_line(row, 3U, 23U, diag_line, UI_COLOR_MAIN_GREEN);

            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "SRC", diag.rx_source_cap,
                                   "VDM", diag.rx_vdm,
                                   "OV", diag.rx_overflow);
            ui_draw_mono_9_on_line(row, 3U, 33U, diag_line, UI_COLOR_MAIN_CYAN);

            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "RST", diag.rx_reset,
                                   "ERR", diag.rx_buf_err,
                                   NULL,
                                   0U);
            ui_draw_mono_9_on_line(row, 3U, 43U, diag_line, UI_COLOR_RED);

            ui_format_pd_diag_header_line(diag_line, sizeof(diag_line), &diag);
            ui_draw_mono_9_on_line(row, 3U, 53U, diag_line, UI_COLOR_MAIN_AMBER);
        }

        for (index = 0U; index < 7U; ++index)
        {
            uint16_t y;
            uint8_t pdo_index;
            char index_text[4];
            char voltage_text[12];
            char current_text[8];
            char power_text[8];

            pdo_index = (uint8_t)(scroll + index);
            if ((pdo_index >= count) || (pdo_index >= SERVICE_PD_SOURCE_PDO_MAX))
            {
                continue;
            }

            pos = 0U;
            ui_append_uint(index_text,
                           &pos,
                           (uint8_t)(sizeof(index_text) - 1U),
                           caps.pdos[pdo_index].position,
                           1U);
            index_text[pos] = '\0';
            ui_format_pdo_voltage(voltage_text, sizeof(voltage_text), &caps.pdos[pdo_index]);
            ui_format_pdo_current(current_text, sizeof(current_text), caps.pdos[pdo_index].current_ma);
            ui_format_pdo_power(power_text, sizeof(power_text), caps.pdos[pdo_index].power_deci_w);

            y = (uint16_t)(11U + index * 10U);
            ui_draw_mono_9_on_line(row, 3U, y, index_text, UI_COLOR_MAIN_CYAN);
            ui_draw_mono_9_on_line(row, 16U, y, voltage_text, UI_COLOR_MAIN_GREEN);
            ui_draw_mono_9_on_line(row, 89U, y, current_text, UI_COLOR_TEXT);
            ui_draw_mono_9_on_line(row, 127U, y, power_text, UI_COLOR_MAIN_AMBER);
        }
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

static void ui_append_text(char *out, uint8_t *pos, uint8_t limit, const char *text)
{
    while ((out != NULL) && (pos != NULL) && (text != NULL) &&
           (*text != '\0') && (*pos < limit))
    {
        out[*pos] = *text++;
        ++(*pos);
    }
}

static const char *ui_emark_speed_text(const protocol_snapshot_t *protocol)
{
    if ((protocol == NULL) || (protocol->emark_present == 0U))
    {
        return "--";
    }
    if (protocol->emark_usb_speed_grade >= 4U)
    {
        return "USB4";
    }
    if (protocol->emark_usb_speed_grade >= 2U)
    {
        return "USB3";
    }
    return "USB2";
}

static const char *ui_emark_cable_type_text(const protocol_snapshot_t *protocol)
{
    if ((protocol == NULL) || (protocol->emark_present == 0U))
    {
        return "--";
    }
    if (protocol->emark_cable_type == 2U)
    {
        return "C-C";
    }
    if (protocol->emark_cable_type == 1U)
    {
        return "C-A";
    }
    return "CABLE";
}

static const char *ui_emark_epr_text(const protocol_snapshot_t *protocol)
{
    if ((protocol == NULL) || (protocol->emark_present == 0U))
    {
        return "--";
    }

    return (protocol->emark_epr_capable != 0U) ? "YES" : "NO";
}

static void ui_format_emark_u8_unit(char *out,
                                    uint8_t limit,
                                    uint8_t value,
                                    const char *unit)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    if (value == 0U)
    {
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), "--");
    }
    else
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), value, 1U);
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), unit);
    }
    out[pos] = '\0';
}

static void ui_format_emark_u8_raw(char *out, uint8_t limit, uint8_t value, uint8_t available)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    if (available == 0U)
    {
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), "--");
    }
    else
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), value, 1U);
    }
    out[pos] = '\0';
}

static void ui_format_emark_length(char *out, uint8_t limit, const protocol_snapshot_t *protocol)
{
    uint8_t pos;

    if ((out == NULL) || (limit == 0U))
    {
        return;
    }

    pos = 0U;
    if ((protocol == NULL) ||
        (protocol->emark_present == 0U) ||
        (protocol->emark_cable_length_m == 0U))
    {
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), "--");
    }
    else if (protocol->emark_cable_length_m >= 8U)
    {
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), ">7M");
    }
    else
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), protocol->emark_cable_length_m, 1U);
        ui_append_text(out, &pos, (uint8_t)(limit - 1U), "M");
    }
    out[pos] = '\0';
}

static const char *ui_cc_name(const protocol_snapshot_t *protocol)
{
    if (protocol == NULL)
    {
        return "--";
    }
    if (protocol->cc_orientation == 1U)
    {
        return "CC1";
    }
    if (protocol->cc_orientation == 2U)
    {
        return "CC2";
    }
    if ((protocol->cc_active_mask & 0x01U) != 0U)
    {
        return "CC1";
    }
    if ((protocol->cc_active_mask & 0x02U) != 0U)
    {
        return "CC2";
    }
    return (protocol->cc_attached != 0U) ? "CC" : "--";
}

static void ui_draw_emark_cell(uint16_t row,
                               uint16_t y,
                               uint16_t label_x,
                               uint16_t value_x,
                               const char *label,
                               const char *value,
                               uint16_t value_color)
{
    ui_draw_mono_9_on_line(row, label_x, y, label, UI_COLOR_MAIN_CYAN);
    ui_draw_mono_9_on_line(row, value_x, y, value, value_color);
}

void ui_renderer_draw_emark_page(const measure_snapshot_t *measure,
                                 const protocol_snapshot_t *protocol)
{
    uint16_t row;
    uint8_t has_emark;
    char current[8];
    char voltage[8];
    char length[8];
    char vdo[8];
    char firmware[8];
    char hardware[8];
    bsp_usbpd_port_diag_t diag;

    (void)measure;
    has_emark = ((protocol != NULL) && (protocol->emark_present != 0U)) ? 1U : 0U;
    bsp_usbpd_port_copy_diag(&diag);

    ui_format_emark_u8_unit(current,
                            sizeof(current),
                            ((protocol != NULL) && (has_emark != 0U)) ? protocol->emark_current_a : 0U,
                            "A");
    ui_format_emark_u8_unit(voltage,
                            sizeof(voltage),
                            ((protocol != NULL) && (has_emark != 0U)) ? protocol->emark_max_voltage_v : 0U,
                            "V");
    ui_format_emark_length(length, sizeof(length), protocol);
    ui_format_emark_u8_raw(vdo,
                           sizeof(vdo),
                           (protocol != NULL) ? protocol->emark_vdo_version : 0U,
                           has_emark);
    ui_format_emark_u8_raw(firmware,
                           sizeof(firmware),
                           (protocol != NULL) ? protocol->emark_firmware_version : 0U,
                           has_emark);
    ui_format_emark_u8_raw(hardware,
                           sizeof(hardware),
                           (protocol != NULL) ? protocol->emark_hardware_version : 0U,
                           has_emark);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_emark_cell(row, 1U, 3U, 42U, "EMARK", has_emark ? "YES" : "NO",
                            has_emark ? UI_COLOR_MAIN_GREEN : UI_COLOR_RED);
        ui_draw_emark_cell(row, 1U, 86U, 122U, "CC", ui_cc_name(protocol),
                            ((protocol != NULL) && ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U))) ? UI_COLOR_MAIN_GREEN : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 11U, 3U, 42U, "CUR", current,
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 11U, 86U, 122U, "VOLT", voltage,
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 21U, 3U, 42U, "LEN", length,
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 21U, 86U, 122U, "SPD", ui_emark_speed_text(protocol),
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 31U, 3U, 42U, "TYPE", ui_emark_cable_type_text(protocol),
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 31U, 86U, 122U, "EPR", ui_emark_epr_text(protocol),
                            has_emark ? UI_COLOR_TEXT : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 41U, 3U, 42U, "VDO", vdo,
                            has_emark ? UI_COLOR_MAIN_CYAN : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 41U, 86U, 122U, "HW", hardware,
                            has_emark ? UI_COLOR_MAIN_CYAN : UI_COLOR_CC_DIM);
        ui_draw_emark_cell(row, 51U, 3U, 42U, "FW", firmware,
                            has_emark ? UI_COLOR_MAIN_CYAN : UI_COLOR_CC_DIM);
        if (has_emark == 0U)
        {
            char diag_line[22];

            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "RX", diag.rx_total,
                                   "S1", diag.rx_sop1,
                                   "VDM", diag.rx_vdm);
            ui_draw_mono_9_on_line(row, 3U, 61U, diag_line, UI_COLOR_MAIN_AMBER);
            ui_format_pd_diag_line(diag_line,
                                   sizeof(diag_line),
                                   "RST", diag.rx_reset,
                                   "ERR", diag.rx_buf_err,
                                   "OV", diag.rx_overflow);
            ui_draw_mono_9_on_line(row, 86U, 61U, diag_line, UI_COLOR_RED);
        }
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

static void ui_renderer_draw_menu_label_on_line(uint16_t row,
                                                uint8_t item,
                                                uint16_t x,
                                                uint16_t y,
                                                uint16_t color)
{
    uint16_t cursor;

    switch (item)
    {
        case 0U:
            cursor = ui_draw_mono_15_on_line(row, x, y, "PDO", color);
            ui_draw_zh_15_on_line(row, (uint16_t)(cursor + 1U), y, g_label_report, 2U, color);
            break;
        case 1U:
            ui_draw_zh_15_on_line(row, x, y, g_label_protocol_detect, 4U, color);
            break;
        case 2U:
            cursor = ui_draw_mono_15_on_line(row, x, y, "EMARK", color);
            ui_draw_zh_15_on_line(row, (uint16_t)(cursor + 1U), y, g_label_read, 2U, color);
            break;
        case 3U:
            cursor = ui_draw_mono_15_on_line(row, x, y, "EMARK", color);
            ui_draw_zh_15_on_line(row, (uint16_t)(cursor + 1U), y, g_label_emulate, 2U, color);
            break;
        case 4U:
            ui_draw_zh_15_on_line(row, x, y, g_label_kelvin, 5U, color);
            break;
        case 5U:
        default:
            ui_draw_zh_15_on_line(row, x, y, g_label_settings_zh, 2U, color);
            break;
    }
}

void ui_renderer_draw_menu_page(const ui_model_state_t *state)
{
    uint8_t selected;
    uint8_t scroll;
    uint8_t count;
    uint8_t visible;
    uint8_t row_index;
    uint16_t row;
    uint16_t progress_y;

    selected = ui_model_menu_selected_index(state);
    scroll = ui_model_menu_scroll(state);
    count = ui_model_menu_item_count();
    visible = 5U;
    if (count < visible)
    {
        visible = count;
    }
    progress_y = (count > 1U) ? (uint16_t)(((uint16_t)selected * (LCD_HEIGHT - 3U)) / (uint16_t)(count - 1U)) : 0U;

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        for (row_index = 0U; row_index < visible; ++row_index)
        {
            uint8_t item;
            uint16_t y;
            uint16_t color;

            item = (uint8_t)(scroll + row_index);
            if (item >= count)
            {
                continue;
            }

            y = (uint16_t)(row_index * 16U);
            color = (item == selected) ? UI_COLOR_MAIN_AMBER : UI_COLOR_MAIN_CYAN;
            if (item == selected)
            {
                uint16_t cursor;

                cursor = ui_draw_mono_15_on_line(row, 0U, y, ">", color);
                ui_renderer_draw_menu_label_on_line(row, item, cursor, y, color);
            }
            else
            {
                ui_renderer_draw_menu_label_on_line(row, item, 0U, y, color);
            }
        }

        ui_draw_rect_on_line(row, 158U, 0U, 1U, LCD_HEIGHT, UI_COLOR_FRAME);
        ui_draw_rect_on_line(row, 157U, progress_y, 3U, 3U, UI_COLOR_MAIN_AMBER);
        ui_end_row(row);
    }
}

void ui_renderer_draw_settings_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure)
{
    static const uint16_t row_y[2] = { 27U, 52U };
    static const uint16_t row_bg_y[2] = { 26U, 51U };
    static const uint16_t label_bg[2] = { UI_COLOR_MAIN_GREEN, UI_COLOR_MAIN_PURPLE };
    static const uint16_t selected_color = UI_COLOR_MAIN_PINK;
    char bright_text[8];
    char rotate_text[8];
    uint8_t pos;
    uint8_t selected;
    uint16_t row;
    uint8_t item;

    (void)measure;
    selected = ui_model_settings_selected_index(state);
    pos = 0U;
    ui_append_uint(bright_text, &pos, (uint8_t)(sizeof(bright_text) - 1U), ui_model_brightness_percent(state), 1U);
    if (pos < (uint8_t)(sizeof(bright_text) - 1U))
    {
        bright_text[pos++] = '%';
    }
    bright_text[pos] = '\0';

    pos = 0U;
    ui_append_uint(rotate_text, &pos, (uint8_t)(sizeof(rotate_text) - 1U), ui_model_rotation_degrees(state), 1U);
    rotate_text[pos] = '\0';

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_zh_15_on_line(row, 7U, 3U, g_label_settings_zh, 2U, UI_COLOR_MAIN_CYAN);
        ui_draw_zh_15_on_line(row,
                              127U,
                              3U,
                              ui_model_action_mode(state) ? g_label_edit_zh : g_label_view_zh,
                              2U,
                              ui_model_action_mode(state) ? UI_COLOR_MAIN_GREEN : UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 4U, 20U, 152U, 1U, UI_COLOR_FRAME);

        for (item = 0U; item < 2U; ++item)
        {
            const uint16_t *label;
            uint16_t label_x;
            uint16_t value_x;
            uint16_t value_width;
            uint16_t row_color;

            if (selected == item)
            {
                ui_draw_rect_on_line(row, 4U, row_bg_y[item], 152U, 17U, 0x18C3U);
            }

            label = (item == 0U) ? g_label_brightness_zh : g_label_rotation_zh;
            row_color = selected == item ? selected_color : UI_COLOR_TEXT;
            ui_draw_rect_on_line(row,
                                 8U,
                                 (uint16_t)(row_y[item] - 1U),
                                 46U,
                                 17U,
                                 selected == item ? selected_color : label_bg[item]);
            label_x = ui_center_x(8U, 46U, ui_zh_15_text_width(label, 2U));
            ui_draw_zh_15_on_line(row, label_x, row_y[item], label, 2U, UI_COLOR_TEXT);

            value_width = 60U;
            if (item == 0U)
            {
                value_x = ui_center_x(86U, value_width, ui_mono_15_text_width(bright_text));
                ui_draw_mono_15_baseline_on_line(row, value_x, row_y[item], bright_text, row_color);
            }
            else if (item == 1U)
            {
                value_x = ui_center_x(86U, value_width, ui_mono_15_text_width(rotate_text));
                ui_draw_mono_15_baseline_on_line(row, value_x, row_y[item], rotate_text, row_color);
            }
        }
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

#if (defined(PX1_ENABLE_KEY_DEBUG_RENDERER) && (PX1_ENABLE_KEY_DEBUG_RENDERER != 0)) || defined(PX1_HOST_TEST)
static void ui_format_mask_bits(char *out, uint8_t mask)
{
    out[0] = ((mask & 0x01U) != 0U) ? '1' : '0';
    out[1] = ((mask & 0x02U) != 0U) ? '1' : '0';
    out[2] = ((mask & 0x04U) != 0U) ? '1' : '0';
    out[3] = '\0';
}

void ui_renderer_draw_key_debug_page(uint8_t raw_high_mask,
                                     uint8_t active_mask,
                                     uint8_t pending_irq_mask,
                                     uint8_t seen_irq_mask,
                                     uint8_t event_mask,
                                     ui_page_t page,
                                     uint16_t event_count,
                                     uint16_t heartbeat)
{
    char raw_text[4];
    char active_text[4];
    char irq_text[4];
    char seen_text[4];
    char event_text[4];
    char page_text[4];
    char count_text[6];
    char heartbeat_text[6];
    uint8_t pos;
    uint16_t marker_x;
    uint16_t row;

    ui_format_mask_bits(raw_text, raw_high_mask);
    ui_format_mask_bits(active_text, active_mask);
    ui_format_mask_bits(irq_text, pending_irq_mask);
    ui_format_mask_bits(seen_text, seen_irq_mask);
    ui_format_mask_bits(event_text, event_mask);
    pos = 0U;
    ui_append_uint(page_text, &pos, (uint8_t)(sizeof(page_text) - 1U), (uint32_t)page, 1U);
    page_text[pos] = '\0';
    pos = 0U;
    ui_append_uint(count_text, &pos, (uint8_t)(sizeof(count_text) - 1U), (uint32_t)event_count, 1U);
    count_text[pos] = '\0';
    pos = 0U;
    ui_append_uint(heartbeat_text, &pos, (uint8_t)(sizeof(heartbeat_text) - 1U), (uint32_t)(heartbeat % 10000U), 1U);
    heartbeat_text[pos] = '\0';
    marker_x = (uint16_t)(heartbeat % (LCD_WIDTH - 10U));

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_ascii_on_line(row, 2U, 1U, "KEY", 2U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 42U, 4U, "DBG", 1U, UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 82U, 4U, "HB", 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 102U, 4U, heartbeat_text, 1U, UI_COLOR_GREEN);

        ui_draw_ascii_on_line(row, 2U, 18U, "RAW", 1U, UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 54U, 15U, raw_text, 2U, UI_COLOR_TEXT);

        ui_draw_ascii_on_line(row, 2U, 35U, "ACT", 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 54U, 32U, active_text, 2U, UI_COLOR_GREEN);

        ui_draw_ascii_on_line(row, 2U, 52U, "IRQ", 1U, UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 54U, 49U, irq_text, 2U, UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 105U, 52U, seen_text, 1U, UI_COLOR_DIM);

        ui_draw_ascii_on_line(row, 2U, 68U, "EVT", 1U, UI_COLOR_MAGENTA);
        ui_draw_ascii_on_line(row, 28U, 68U, event_text, 1U, UI_COLOR_MAGENTA);
        ui_draw_ascii_on_line(row, 64U, 68U, "PG", 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 82U, 68U, page_text, 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 108U, 68U, "N", 1U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 120U, 68U, count_text, 1U, UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 0U, 77U, LCD_WIDTH, 2U, UI_COLOR_PANEL);
        ui_draw_rect_on_line(row, marker_x, 76U, 10U, 4U, UI_COLOR_RED);
        ui_end_row(row);
    }
}
#endif

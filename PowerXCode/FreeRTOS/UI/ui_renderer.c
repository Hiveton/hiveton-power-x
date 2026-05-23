#include "ui_renderer.h"

#include <stddef.h>
#include <stdint.h>

#include "bsp_lcd_st7735.h"
#include "font_zh_12.h"
#include "ui_scope_model.h"
#include "ui_value_format.h"
#include "ui_widgets.h"

#define UI_COLOR_BLACK 0x0000U
#define UI_COLOR_PANEL 0x0841U
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
#define UI_COLOR_CC_ACTIVE 0x2FADU
#define UI_COLOR_CC_ACTIVE_DARK 0x1569U
#define UI_COLOR_CC_DIM 0x7C31U
#define UI_COLOR_CC_DARK 0x10C4U

static uint16_t g_line_buffer[LCD_WIDTH];

static const uint16_t g_label_protocol[] = { 0x534FU, 0x8BAEU };
static const uint16_t g_label_trigger[] = { 0x89E6U, 0x53D1U };
static const uint16_t g_label_settings[] = { 0x8BBEU, 0x7F6EU };
static const uint16_t g_label_brightness[] = { 0x4EAEU, 0x5EA6U };
static const uint16_t g_label_rotate[] = { 0x65CBU, 0x8F6CU };
static const uint16_t g_label_cable[] = { 0x7EBFU, 0x7F06U };
static const uint16_t g_label_voltage[] = { 0x7535U, 0x538BU };
static const uint16_t g_label_current[] = { 0x7535U, 0x6D41U };
static const uint16_t g_label_power[] = { 0x529FU, 0x7387U };

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

static const font_zh_12_glyph_t *ui_zh_glyph(uint16_t codepoint)
{
    size_t index;

    for (index = 0U; index < FONT_ZH_12_GLYPH_COUNT; ++index)
    {
        if (g_font_zh_12_glyphs[index].codepoint == codepoint)
        {
            return &g_font_zh_12_glyphs[index];
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

static void ui_draw_zh_on_line(uint16_t row,
                               uint16_t x,
                               uint16_t y,
                               const uint16_t *codes,
                               uint8_t count,
                               uint16_t color)
{
    uint8_t index;

    if ((codes == NULL) || (row < y) || (row >= (uint16_t)(y + FONT_ZH_12_HEIGHT)))
    {
        return;
    }

    for (index = 0U; index < count; ++index)
    {
        const font_zh_12_glyph_t *glyph;
        uint16_t col;
        uint16_t draw_x;
        uint16_t glyph_row;

        glyph = ui_zh_glyph(codes[index]);
        if (glyph == NULL)
        {
            continue;
        }

        draw_x = (uint16_t)(x + index * 13U);
        glyph_row = (uint16_t)(row - y);
        for (col = 0U; col < FONT_ZH_12_WIDTH; ++col)
        {
            if ((glyph->rows[glyph_row] & (uint16_t)(1U << (11U - col))) != 0U)
            {
                ui_put_pixel((uint16_t)(draw_x + col), color);
            }
        }
    }
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

static void ui_format_voltage(char *out, uint8_t limit, int32_t mv)
{
    ui_value_format_meter_5half(out, limit, mv);
}

static void ui_format_current(char *out, uint8_t limit, int32_t ma)
{
    ui_value_format_meter_5half(out, limit, ma);
}

static void ui_format_power(char *out, uint8_t limit, int32_t mw)
{
    ui_value_format_meter_5half(out, limit, mw);
}

static void ui_append_unit(char *out, uint8_t limit, char unit)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 2U))
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
        out[pos] = '\0';
    }
}

static const char *ui_protocol_name(protocol_kind_t kind)
{
    switch (kind)
    {
        case PROTOCOL_KIND_PD:
            return "PD";
        case PROTOCOL_KIND_QC:
            return "QC";
        case PROTOCOL_KIND_AFC:
            return "AFC";
        case PROTOCOL_KIND_FCP:
            return "FCP";
        case PROTOCOL_KIND_OTHER:
            return "OTHER";
        case PROTOCOL_KIND_NONE:
        default:
            return "--";
    }
}

static const char *ui_protocol_display_name(const protocol_snapshot_t *protocol, uint8_t compact)
{
    if ((protocol != NULL) &&
        (protocol->kind == PROTOCOL_KIND_NONE) &&
        ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U)))
    {
        return (compact != 0U) ? "TC" : "TYPEC";
    }

    return ui_protocol_name((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE);
}

static const char *ui_request_state_name(protocol_request_state_t state)
{
    switch (state)
    {
        case PROTOCOL_REQUEST_AVAILABLE:
            return "AVAIL";
        case PROTOCOL_REQUEST_REQUESTING:
            return "REQ";
        case PROTOCOL_REQUEST_ACCEPTED:
            return "ACC";
        case PROTOCOL_REQUEST_READY:
            return "READY";
        case PROTOCOL_REQUEST_FAILED:
            return "FAIL";
        case PROTOCOL_REQUEST_IDLE:
        default:
            return "IDLE";
    }
}

static void ui_format_target_label(char *out, uint8_t limit, int32_t target_mv)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 3U))
    {
        return;
    }

    pos = 0U;
    out[pos++] = 'T';
    if (target_mv <= 0)
    {
        out[pos++] = '-';
        out[pos] = '\0';
        return;
    }

    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (uint32_t)(target_mv / 1000), 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = 'V';
    }
    out[pos] = '\0';
}

static void ui_format_voltage_label(char *out, uint8_t limit, int32_t mv)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 3U))
    {
        return;
    }

    pos = 0U;
    if (mv <= 0)
    {
        out[pos++] = '-';
        out[pos] = '\0';
        return;
    }

    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (uint32_t)(mv / 1000), 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = 'V';
    }
    out[pos] = '\0';
}

static void ui_format_voltage_label_tenth(char *out, uint8_t limit, int32_t mv)
{
    uint8_t pos;
    uint32_t tenths;

    if ((out == NULL) || (limit < 5U))
    {
        return;
    }

    pos = 0U;
    if (mv <= 0)
    {
        out[pos++] = '-';
        out[pos] = '\0';
        return;
    }

    tenths = (uint32_t)((mv + 50) / 100);
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), tenths / 10U, 1U);
    if (((tenths % 10U) != 0U) && (pos < (uint8_t)(limit - 1U)))
    {
        out[pos++] = '.';
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = (char)('0' + (tenths % 10U));
        }
    }
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = 'V';
    }
    out[pos] = '\0';
}

static void ui_format_voltage_label_hundredth(char *out, uint8_t limit, int32_t mv)
{
    uint8_t pos;
    uint32_t hundredths;

    if ((out == NULL) || (limit < 6U))
    {
        return;
    }

    pos = 0U;
    if (mv <= 0)
    {
        out[pos++] = '-';
        out[pos] = '\0';
        return;
    }

    hundredths = (uint32_t)((mv + 5) / 10);
    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), hundredths / 100U, 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = '.';
    }
    if (pos < (uint8_t)(limit - 1U))
    {
        ui_append_uint(out, &pos, (uint8_t)(limit - 1U), hundredths % 100U, 2U);
    }
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = 'V';
    }
    out[pos] = '\0';
}

static void ui_format_current_label(char *out, uint8_t limit, int32_t ma)
{
    uint8_t pos;

    if ((out == NULL) || (limit < 3U))
    {
        return;
    }

    pos = 0U;
    if (ma <= 0)
    {
        out[pos++] = '-';
        out[pos] = '\0';
        return;
    }

    ui_append_uint(out, &pos, (uint8_t)(limit - 1U), (uint32_t)(ma / 1000), 1U);
    if (pos < (uint8_t)(limit - 1U))
    {
        out[pos++] = 'A';
    }
    out[pos] = '\0';
}

static int32_t ui_measure_voltage_mv(const measure_snapshot_t *measure)
{
    return ((measure != NULL) && (measure->voltage_valid != 0U)) ? measure->voltage_avg_mv : 0;
}

static int32_t ui_measure_current_ma(const measure_snapshot_t *measure)
{
    return ((measure != NULL) && (measure->current_valid != 0U)) ? measure->current_avg_ma : 0;
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
    char target[8];
    const char *protocol_name;
    uint8_t live_phase;
    uint16_t live_x;
    int32_t mv;
    int32_t ma;
    int32_t mw;
    uint16_t row;

    mv = ((measure != NULL) && (measure->voltage_valid != 0U)) ? measure->voltage_avg_mv : 0;
    ma = ((measure != NULL) && (measure->current_valid != 0U)) ? measure->current_avg_ma : 0;
    mw = ((measure != NULL) && (measure->power_valid != 0U)) ? measure->power_mw : 0;
    ui_format_voltage(voltage, sizeof(voltage), mv);
    ui_format_current(current, sizeof(current), ma);
    ui_format_power(power, sizeof(power), mw);
    ui_format_voltage_label(target,
                            sizeof(target),
                            (protocol != NULL) ? protocol->target_mv : 0);
    protocol_name = ui_protocol_display_name(protocol, 1U);
    live_phase = (uint8_t)(ui_model_liveness_frame(state) % 8U);
    live_x = (uint16_t)(132U + live_phase * 3U);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_rect_on_line(row, 0U, 0U, LCD_WIDTH, 12U, UI_COLOR_HEADER);
        ui_draw_ascii_on_line(row, 3U, 3U, "PX1", 1U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 46U, 3U, 34U, 7U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 45U, 2U, 36U, 9U, UI_COLOR_CC_ACTIVE_DARK);
        ui_draw_ascii_on_line(row, 48U, 4U, protocol_name, 1U, UI_COLOR_MAIN_GREEN);
        ui_draw_rect_on_line(row, 131U, 3U, 23U, 7U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 130U, 2U, 25U, 9U, UI_COLOR_CC_ACTIVE_DARK);
        ui_draw_ascii_on_line(row, 133U, 4U, target, 1U, UI_COLOR_MAIN_GREEN);
        ui_draw_rect_on_line(row, 0U, 12U, LCD_WIDTH, 1U, UI_COLOR_FRAME);

        ui_draw_zh_on_line(row, 3U, 18U, g_label_voltage, 2U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 38U, 15U, voltage, 3U, UI_COLOR_MAIN_GREEN);
        ui_draw_ascii_on_line(row, 146U, 28U, "V", 1U, UI_COLOR_MAIN_GREEN);
        ui_draw_rect_on_line(row, 0U, 38U, LCD_WIDTH, 1U, UI_COLOR_FRAME);

        ui_draw_zh_on_line(row, 3U, 42U, g_label_current, 2U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 43U, 41U, current, 2U, UI_COLOR_MAIN_AMBER);
        ui_draw_ascii_on_line(row, 122U, 47U, "A", 1U, UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 0U, 58U, LCD_WIDTH, 1U, UI_COLOR_FRAME);

        ui_draw_zh_on_line(row, 3U, 62U, g_label_power, 2U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 43U, 61U, power, 2U, UI_COLOR_MAIN_CYAN);
        ui_draw_ascii_on_line(row, 122U, 68U, "W", 1U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 131U, 74U, 25U, 2U, UI_COLOR_HEADER_INSET);
        ui_draw_rect_on_line(row, live_x, 74U, 4U, 2U, UI_COLOR_MAIN_GREEN);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_scope_page(const measure_snapshot_t *measure)
{
    char voltage[8];
    char current[8];
    char power[8];
    char min_text[8];
    char max_text[8];
    char ripple_text[8];
    ui_scope_metrics_t metrics;
    uint16_t row;
    uint8_t col;

    ui_scope_metrics_from_measure(measure, &metrics);
    ui_format_voltage(voltage, sizeof(voltage), ui_measure_voltage_mv(measure));
    ui_format_current(current, sizeof(current), ui_measure_current_ma(measure));
    ui_format_power(power, sizeof(power), ui_measure_power_mw(measure));
    ui_format_voltage(min_text, sizeof(min_text), (metrics.voltage_valid != 0U) ? metrics.voltage_min_mv : 0);
    ui_format_voltage(max_text, sizeof(max_text), (metrics.voltage_valid != 0U) ? metrics.voltage_max_mv : 0);
    ui_format_voltage(ripple_text, sizeof(ripple_text), (int32_t)metrics.ripple_pp_est_mv);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_ascii_on_line(row, 2U, 2U, "SCOPE", 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 52U, 2U, "R", 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 64U, 2U, ripple_text, 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 130U, 2U, "LIVE", 1U, UI_COLOR_GREEN);
        ui_draw_rect_on_line(row, 0U, 12U, LCD_WIDTH, 1U, UI_COLOR_PANEL);

        for (col = 0U; col < 7U; ++col)
        {
            ui_draw_rect_on_line(row, (uint16_t)(24U + col * 12U), 16U, 1U, 50U, UI_COLOR_PANEL);
        }
        ui_draw_rect_on_line(row, 20U, 27U, 92U, 1U, UI_COLOR_PANEL);
        ui_draw_rect_on_line(row, 20U, 44U, 92U, 1U, UI_COLOR_PANEL);
        ui_draw_rect_on_line(row, 20U, 61U, 92U, 1U, UI_COLOR_PANEL);

        ui_draw_ascii_on_line(row, 3U, 20U, "V", 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 3U, 37U, "A", 1U, UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 3U, 54U, "P", 1U, UI_COLOR_CYAN);
        for (col = 0U; col < 84U; ++col)
        {
            uint16_t x;
            uint16_t wave_a;
            uint16_t wave_b;
            uint16_t wave_c;

            x = (uint16_t)(24U + col);
            wave_a = (uint16_t)(22U + ((col * 5U + (col / 7U) * 3U) % 9U));
            wave_b = (uint16_t)(39U + ((col * 3U + (col / 5U) * 4U) % 9U));
            wave_c = (uint16_t)(56U + ((col * 7U + (col / 9U) * 2U) % 8U));
            ui_draw_rect_on_line(row, x, wave_a, 1U, 1U, UI_COLOR_GREEN);
            ui_draw_rect_on_line(row, x, wave_b, 1U, 1U, UI_COLOR_AMBER);
            ui_draw_rect_on_line(row, x, wave_c, 1U, 1U, UI_COLOR_CYAN);
        }

        ui_draw_ascii_on_line(row, 116U, 20U, voltage, 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 116U, 38U, current, 1U, UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 116U, 56U, power, 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 8U, 70U, "MIN", 1U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 36U, 70U, min_text, 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 88U, 70U, "MAX", 1U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 116U, 70U, max_text, 1U, UI_COLOR_GREEN);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_protocol_page(const measure_snapshot_t *measure,
                                    const protocol_snapshot_t *protocol)
{
    char voltage[9];
    char current[9];
    protocol_kind_t kind;
    const char *name;
    const char *state_name;
    const char *cable_text;
    char target_text[8];
    uint8_t name_scale;
    uint16_t name_x;
    uint16_t row;

    kind = (protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE;
    name = ui_protocol_display_name(protocol, 0U);
    state_name = ui_request_state_name((protocol != NULL) ? protocol->request_state : PROTOCOL_REQUEST_IDLE);
    if ((protocol != NULL) && (protocol->emark_present != 0U) && (protocol->emark_current_a >= 5U))
    {
        cable_text = "OK";
    }
    else if ((protocol != NULL) && (protocol->emark_present != 0U))
    {
        cable_text = "OK";
    }
    else
    {
        cable_text = "--";
    }
    name_scale = (kind == PROTOCOL_KIND_OTHER) ? 2U : 4U;
    name_x = (kind == PROTOCOL_KIND_OTHER) ? 6U : 8U;
    ui_format_target_label(target_text,
                           sizeof(target_text),
                           (protocol != NULL) ? protocol->target_mv : 0);
    ui_format_voltage(voltage,
                      sizeof(voltage),
                      ((measure != NULL) && (measure->voltage_valid != 0U)) ? measure->voltage_avg_mv : 0);
    ui_format_current(current,
                      sizeof(current),
                      ((measure != NULL) && (measure->current_valid != 0U)) ? measure->current_avg_ma : 0);
    ui_append_unit(voltage, sizeof(voltage), 'V');
    ui_append_unit(current, sizeof(current), 'A');

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_zh_on_line(row, 3U, 3U, g_label_protocol, 2U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 43U, 3U, 56U, 8U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 42U, 2U, 58U, 10U,
                            ((protocol != NULL) && (protocol->request_state == PROTOCOL_REQUEST_FAILED)) ? UI_COLOR_RED : UI_COLOR_CC_ACTIVE_DARK);
        ui_draw_ascii_on_line(row, 45U, 5U, state_name, 1U,
                              ((protocol != NULL) && (protocol->request_state == PROTOCOL_REQUEST_FAILED)) ? UI_COLOR_RED : UI_COLOR_MAIN_GREEN);
        ui_draw_rect_on_line(row, 135U, 3U, 17U, 8U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 134U, 2U, 19U, 10U,
                            ((protocol != NULL) && (protocol->kind != PROTOCOL_KIND_NONE)) ? UI_COLOR_CC_ACTIVE_DARK : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 137U, 5U, ((protocol != NULL) && (protocol->emark_present != 0U)) ? "E" : "-", 1U,
                              ((protocol != NULL) && (protocol->emark_present != 0U)) ? UI_COLOR_MAIN_GREEN : UI_COLOR_DIM);

        if (kind == PROTOCOL_KIND_PD)
        {
            ui_draw_ascii_on_line(row, 16U, 22U, "P", 5U, UI_COLOR_MAIN_GREEN);
            ui_draw_ascii_on_line(row, 46U, 22U, "D", 5U, UI_COLOR_MAIN_GREEN);
        }
        else
        {
            ui_draw_ascii_on_line(row, name_x, 22U, name, name_scale,
                                  ui_widgets_protocol_color(kind,
                                                            (protocol != NULL) ? protocol->emark_present : 0U));
        }
        ui_draw_rect_on_line(row, 83U, 17U, 1U, 46U, UI_COLOR_FRAME);
        ui_draw_ascii_on_line(row, 91U, 20U, voltage, 1U, UI_COLOR_MAIN_GREEN);
        ui_draw_ascii_on_line(row, 91U, 36U, current, 1U, UI_COLOR_MAIN_AMBER);
        ui_draw_ascii_on_line(row, 92U, 52U, target_text, 1U, UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 4U, 66U, 151U, 1U, UI_COLOR_FRAME);
        ui_draw_ascii_on_line(row, 8U, 69U, "PPS", 1U,
                              ((protocol != NULL) && (protocol->pps_present != 0U)) ? UI_COLOR_MAIN_CYAN : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 58U, 69U, ((protocol != NULL) && (protocol->emark_present != 0U)) ? "E-MARK" : "NOE", 1U,
                              ((protocol != NULL) && (protocol->emark_present != 0U)) ? UI_COLOR_MAIN_GREEN : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 118U, 69U, cable_text, 1U,
                              ((protocol != NULL) && (protocol->emark_present != 0U)) ? UI_COLOR_MAIN_GREEN : UI_COLOR_TEXT);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_trigger_page(const ui_model_state_t *state,
                                   const protocol_snapshot_t *protocol)
{
    unsigned char selected;
    unsigned char count;
    const char *protocol_name;
    const char *state_name;
    uint16_t row;

    selected = ui_model_trigger_selected_index(state);
    count = ui_model_trigger_preset_count();
    protocol_name = ui_protocol_name((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE);
    state_name = ui_request_state_name((protocol != NULL) ? protocol->request_state : PROTOCOL_REQUEST_IDLE);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        unsigned char prev_index;
        unsigned char next_index;
        int selected_mv;
        char selected_text[8];
        char prev_text[8];
        char next_text[8];
        uint8_t pos;

        ui_begin_row(row);
        ui_draw_zh_on_line(row, 3U, 3U, g_label_trigger, 2U, UI_COLOR_MAIN_AMBER);
        ui_draw_ascii_on_line(row, 112U, 4U, state_name, 1U,
                              ((protocol != NULL) && (protocol->request_state == PROTOCOL_REQUEST_FAILED)) ? UI_COLOR_RED : UI_COLOR_MAIN_GREEN);
        ui_draw_rect_on_line(row, 5U, 18U, 150U, 1U, UI_COLOR_FRAME);

        prev_index = (unsigned char)((selected + count - 1U) % count);
        next_index = (unsigned char)((selected + 1U) % count);
        selected_mv = ui_model_trigger_preset_mv_at(selected);

        pos = 0U;
        ui_append_uint(selected_text, &pos, (uint8_t)(sizeof(selected_text) - 1U), (uint32_t)(selected_mv / 1000), 1U);
        if (pos < (uint8_t)(sizeof(selected_text) - 1U))
        {
            selected_text[pos++] = 'V';
        }
        selected_text[pos] = '\0';

        pos = 0U;
        ui_append_uint(prev_text, &pos, (uint8_t)(sizeof(prev_text) - 1U), (uint32_t)(ui_model_trigger_preset_mv_at(prev_index) / 1000), 1U);
        if (pos < (uint8_t)(sizeof(prev_text) - 1U))
        {
            prev_text[pos++] = 'V';
        }
        prev_text[pos] = '\0';

        pos = 0U;
        ui_append_uint(next_text, &pos, (uint8_t)(sizeof(next_text) - 1U), (uint32_t)(ui_model_trigger_preset_mv_at(next_index) / 1000), 1U);
        if (pos < (uint8_t)(sizeof(next_text) - 1U))
        {
            next_text[pos++] = 'V';
        }
        next_text[pos] = '\0';

        ui_draw_ascii_on_line(row, 12U, 21U, "5V", 1U, selected == 0U ? UI_COLOR_MAIN_AMBER : UI_COLOR_CC_DIM);
        ui_draw_ascii_on_line(row, 45U, 21U, "9V", 1U, selected == 1U ? UI_COLOR_MAIN_AMBER : UI_COLOR_CC_DIM);
        ui_draw_ascii_on_line(row, 74U, 21U, "12V", 1U, selected == 2U ? UI_COLOR_MAIN_AMBER : UI_COLOR_CC_DIM);
        ui_draw_ascii_on_line(row, 111U, 21U, "15V", 1U, selected == 3U ? UI_COLOR_MAIN_AMBER : UI_COLOR_CC_DIM);
        ui_draw_ascii_on_line(row, 140U, 21U, "20V", 1U, selected == 4U ? UI_COLOR_MAIN_AMBER : UI_COLOR_CC_DIM);
        ui_draw_rect_on_line(row, 47U, 34U, 60U, 2U, UI_COLOR_MAIN_AMBER);
        (void)prev_text;
        (void)next_text;
        ui_draw_ascii_on_line(row, 58U, 39U, selected_text, 4U, UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 46U, 66U, 62U, 2U, UI_COLOR_MAIN_AMBER);
        ui_draw_ascii_on_line(row, 5U, 69U, protocol_name, 1U,
                              ui_widgets_protocol_color((protocol != NULL) ? protocol->kind : PROTOCOL_KIND_NONE,
                                                        (protocol != NULL) ? protocol->emark_present : 0U));
        ui_draw_ascii_on_line(row, 64U, 69U, "REC", 1U, UI_COLOR_TEXT);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_pdo_page(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol)
{
    uint16_t row;
    int32_t selected_mv;
    char target_text[8];

    selected_mv = ui_model_pdo_target_mv(state);
    ui_format_voltage_label_hundredth(target_text, sizeof(target_text), selected_mv);
    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        uint8_t index;

        ui_begin_row(row);
        ui_draw_ascii_on_line(row, 2U, 2U, "PDO", 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 38U, 2U, (ui_model_action_mode(state) != 0U) ? "EDIT" : "LIST", 1U,
                              (ui_model_action_mode(state) != 0U) ? UI_COLOR_AMBER : UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 70U, 2U, target_text, 1U, UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 112U, 2U, ui_request_state_name((protocol != NULL) ? protocol->request_state : PROTOCOL_REQUEST_IDLE), 1U,
                              ((protocol != NULL) && (protocol->request_state == PROTOCOL_REQUEST_FAILED)) ? UI_COLOR_RED : UI_COLOR_GREEN);
        ui_draw_rect_on_line(row, 0U, 12U, LCD_WIDTH, 1U, UI_COLOR_PANEL);

        for (index = 0U; index < 5U; ++index)
        {
            uint16_t y;
            int32_t mv;
            int32_t ma;
            char mv_text[8];
            char ma_text[8];
            uint16_t color;
            uint8_t draw_pps_row;

            y = (uint16_t)(18U + index * 12U);
            draw_pps_row = 0U;
            if ((protocol != NULL) && (protocol->pps_present != 0U) && (protocol->pps_max_mv > 0))
            {
                if (((protocol->source_fixed_count < 5U) && (index == protocol->source_fixed_count)) ||
                    ((protocol->source_fixed_count >= 5U) && (index == 4U)))
                {
                    draw_pps_row = 1U;
                }
            }

            if (draw_pps_row != 0U)
            {
                char min_text[8];
                char max_text[8];
                uint8_t fixed_has_selected_mv;
                uint8_t fixed_index;

                ui_format_voltage_label_tenth(min_text, sizeof(min_text), protocol->pps_min_mv);
                ui_format_voltage_label_tenth(max_text, sizeof(max_text), protocol->pps_max_mv);
                ui_format_current_label(ma_text, sizeof(ma_text), protocol->pps_max_ma);
                fixed_has_selected_mv = 0U;
                for (fixed_index = 0U; fixed_index < protocol->source_fixed_count; ++fixed_index)
                {
                    if (protocol->source_fixed_mv[fixed_index] == selected_mv)
                    {
                        fixed_has_selected_mv = 1U;
                        break;
                    }
                }
                color = ((fixed_has_selected_mv == 0U) &&
                         (selected_mv >= protocol->pps_min_mv) &&
                         (selected_mv <= protocol->pps_max_mv)) ? UI_COLOR_AMBER : UI_COLOR_CYAN;
                if (color == UI_COLOR_AMBER)
                {
                    ui_draw_rect_on_line(row, 2U, y - 2U, 156U, 10U, UI_COLOR_PANEL);
                }
                ui_draw_ascii_on_line(row, 8U, y, "PPS", 1U, color);
                ui_draw_ascii_on_line(row, 44U, y, min_text, 1U, color);
                ui_draw_ascii_on_line(row, 82U, y, max_text, 1U, color);
                ui_draw_ascii_on_line(row, 122U, y, ma_text, 1U, color);
                continue;
            }

            if ((protocol != NULL) && (index < protocol->source_fixed_count))
            {
                mv = protocol->source_fixed_mv[index];
                ma = protocol->source_fixed_ma[index];
            }
            else
            {
                mv = ui_model_trigger_preset_mv_at(index);
                ma = 3000;
            }

            ui_format_voltage_label(mv_text, sizeof(mv_text), mv);
            ui_format_current_label(ma_text, sizeof(ma_text), ma);
            color = (mv == selected_mv) ? UI_COLOR_AMBER : UI_COLOR_TEXT;
            if (mv == selected_mv)
            {
                ui_draw_rect_on_line(row, 2U, y - 2U, 156U, 10U, UI_COLOR_PANEL);
            }
            ui_draw_ascii_on_line(row, 8U, y, mv_text, 1U, color);
            ui_draw_ascii_on_line(row, 64U, y, ma_text, 1U, color);
            ui_draw_ascii_on_line(row, 120U, y, (mv == selected_mv) ? "SEL" : "FIX", 1U, color);
        }
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_qc_page(const ui_model_state_t *state,
                              const protocol_snapshot_t *protocol)
{
    char target_text[8];
    char dp_text[8];
    char dm_text[8];
    const char *state_name;
    uint16_t row;
    int32_t dp_mv;
    int32_t dm_mv;

    ui_format_voltage_label(target_text, sizeof(target_text), ui_model_qc_selected_mv(state));
    dp_mv = (protocol != NULL) ? protocol->dp_mv : 0;
    dm_mv = (protocol != NULL) ? protocol->dm_mv : 0;
    ui_format_voltage(dp_text, sizeof(dp_text), dp_mv);
    ui_format_voltage(dm_text, sizeof(dm_text), dm_mv);
    state_name = ui_request_state_name((protocol != NULL) ? protocol->request_state : PROTOCOL_REQUEST_IDLE);

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_ascii_on_line(row, 2U, 2U, "QC", 2U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 42U, 4U, (ui_model_action_mode(state) != 0U) ? "EDIT" : "DPDM", 1U,
                              (ui_model_action_mode(state) != 0U) ? UI_COLOR_AMBER : UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 118U, 4U, state_name, 1U,
                              ((protocol != NULL) && (protocol->request_state == PROTOCOL_REQUEST_FAILED)) ? UI_COLOR_RED : UI_COLOR_GREEN);
        ui_draw_rect_on_line(row, 0U, 15U, LCD_WIDTH, 1U, UI_COLOR_PANEL);

        ui_draw_ascii_on_line(row, 8U, 23U, "TGT", 1U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 54U, 20U, target_text, 3U, UI_COLOR_AMBER);
        ui_draw_rect_on_line(row, 0U, 46U, LCD_WIDTH, 1U, UI_COLOR_PANEL);

        ui_draw_ascii_on_line(row, 8U, 54U, "DP", 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 32U, 54U, dp_text, 1U, UI_COLOR_GREEN);
        ui_draw_ascii_on_line(row, 86U, 54U, "DM", 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 110U, 54U, dm_text, 1U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 8U, 68U, "QC2", 1U,
                              (ui_model_qc_selected_mv(state) == 15000) ? UI_COLOR_DIM : UI_COLOR_AMBER);
        ui_draw_ascii_on_line(row, 54U, 68U, "QC3", 1U,
                              (ui_model_qc_selected_mv(state) == 15000) ? UI_COLOR_AMBER : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 112U, 68U, ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_QC)) ? "ON" : "--", 1U,
                              ((protocol != NULL) && (protocol->kind == PROTOCOL_KIND_QC)) ? UI_COLOR_GREEN : UI_COLOR_DIM);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_cc_page(const measure_snapshot_t *measure,
                              const protocol_snapshot_t *protocol)
{
    uint16_t row;
    uint8_t cc_orientation;
    uint8_t cc1_active;
    uint8_t cc2_active;

    (void)measure;
    cc_orientation = (protocol != NULL) ? protocol->cc_orientation : 0U;
    cc1_active = (cc_orientation == 1U) ? 1U : 0U;
    cc2_active = (cc_orientation == 2U) ? 1U : 0U;

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_ascii_on_line(row, 2U, 2U, "CC", 2U, UI_COLOR_CYAN);
        ui_draw_ascii_on_line(row, 112U, 6U, "RP/RD", 1U, UI_COLOR_CC_DIM);
        ui_draw_rect_on_line(row, 4U, 22U, 151U, 1U, UI_COLOR_FRAME);

        ui_draw_ascii_on_line(row, 6U, 31U, "CC1", 1U, UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 37U, 30U, 96U, 9U, UI_COLOR_CC_DARK);
        ui_draw_rect_on_line(row, 37U, 30U, cc1_active ? 72U : 25U, 9U,
                             cc1_active ? UI_COLOR_CC_ACTIVE : UI_COLOR_CC_DIM);
        if (cc1_active != 0U)
        {
            ui_draw_rect_on_line(row, 109U, 30U, 24U, 9U, UI_COLOR_CC_ACTIVE_DARK);
        }
        ui_draw_ascii_on_line(row, 139U, 31U, cc1_active ? "ON" : "--", 1U,
                              cc1_active ? UI_COLOR_CC_ACTIVE : UI_COLOR_CC_DIM);

        ui_draw_ascii_on_line(row, 6U, 55U, "CC2", 1U, UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 37U, 54U, 96U, 9U, UI_COLOR_CC_DARK);
        ui_draw_rect_on_line(row, 37U, 54U, cc2_active ? 72U : 25U, 9U,
                             cc2_active ? UI_COLOR_CC_ACTIVE : UI_COLOR_CC_DIM);
        if (cc2_active != 0U)
        {
            ui_draw_rect_on_line(row, 109U, 54U, 24U, 9U, UI_COLOR_CC_ACTIVE_DARK);
        }
        ui_draw_ascii_on_line(row, 139U, 55U, cc2_active ? "ON" : "--", 1U,
                              cc2_active ? UI_COLOR_CC_ACTIVE : UI_COLOR_CC_DIM);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_cable_page(const measure_snapshot_t *measure,
                                 const protocol_snapshot_t *protocol)
{
    uint16_t row;
    uint8_t has_emark;
    uint8_t active;
    const char *current_text;
    const char *cc_text;
    const char *emark_text;

    has_emark = ((protocol != NULL) && (protocol->emark_present != 0U)) ? 1U : 0U;
    active = ((measure != NULL) &&
              (measure->voltage_valid != 0U) &&
              (measure->voltage_avg_mv >= 3000)) ? 1U : 0U;
    if (has_emark == 0U)
    {
        current_text = "--";
    }
    else if ((protocol != NULL) && (protocol->emark_current_a >= 5U))
    {
        current_text = "5A";
    }
    else if ((protocol != NULL) && (protocol->emark_current_a >= 3U))
    {
        current_text = "3A";
    }
    else
    {
        current_text = "--";
    }
    cc_text = ((protocol != NULL) &&
               ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U))) ? "CC OK" : "CC --";
    emark_text = has_emark ? "EM" : "--";

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_zh_on_line(row, 3U, 3U, g_label_cable, 2U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 95U, 4U, 56U, 8U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 94U, 3U, 58U, 10U, active ? UI_COLOR_CC_ACTIVE_DARK : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 97U, 6U, active ? "VBUS OK" : "NO BUS", 1U,
                              active ? UI_COLOR_MAIN_GREEN : UI_COLOR_DIM);

        ui_draw_ascii_on_line(row, 10U, 23U, has_emark ? "E-MARK" : "NO E", 2U,
                              has_emark ? UI_COLOR_MAIN_GREEN : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 12U, 51U, current_text, 3U,
                              has_emark ? UI_COLOR_MAIN_AMBER : UI_COLOR_TEXT);
        ui_draw_rect_on_line(row, 85U, 51U, 36U, 8U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 84U, 50U, 38U, 10U,
                            ((protocol != NULL) &&
                             ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U))) ? UI_COLOR_CC_ACTIVE_DARK : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 87U, 52U, cc_text, 1U,
                              ((protocol != NULL) &&
                               ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U))) ? UI_COLOR_MAIN_GREEN : UI_COLOR_DIM);
        ui_draw_rect_on_line(row, 127U, 51U, 26U, 8U, UI_COLOR_HEADER_INSET);
        ui_draw_box_on_line(row, 126U, 50U, 28U, 10U, has_emark ? UI_COLOR_MAIN_CYAN : UI_COLOR_DIM);
        ui_draw_ascii_on_line(row, 132U, 52U, emark_text, 1U,
                              has_emark ? UI_COLOR_MAIN_CYAN : UI_COLOR_DIM);
        ui_draw_frame_on_line(row, UI_COLOR_FRAME);
        ui_end_row(row);
    }
}

void ui_renderer_draw_settings_page(const ui_model_state_t *state)
{
    char brightness_text[6];
    char rotation_text[5];
    uint16_t row;
    uint8_t selected;
    uint8_t action_mode;
    uint8_t pos;

    selected = ui_model_settings_selected_index(state);
    action_mode = ui_model_action_mode(state);
    pos = 0U;
    ui_append_uint(brightness_text, &pos, (uint8_t)(sizeof(brightness_text) - 1U), ui_model_brightness_percent(state), 1U);
    if (pos < (uint8_t)(sizeof(brightness_text) - 1U))
    {
        brightness_text[pos++] = '%';
    }
    brightness_text[pos] = '\0';

    pos = 0U;
    ui_append_uint(rotation_text, &pos, (uint8_t)(sizeof(rotation_text) - 1U), ui_model_rotation_degrees(state), 1U);
    rotation_text[pos] = '\0';

    for (row = 0U; row < LCD_HEIGHT; ++row)
    {
        ui_begin_row(row);
        ui_draw_zh_on_line(row, 3U, 3U, g_label_settings, 2U, UI_COLOR_MAIN_CYAN);
        ui_draw_rect_on_line(row, 4U, 21U, 2U, 48U, UI_COLOR_MAIN_CYAN);
        if (action_mode != 0U)
        {
            if (selected == 0U)
            {
                ui_draw_rect_on_line(row, 4U, 20U, 3U, 17U, UI_COLOR_MAIN_AMBER);
            }
            else if (selected == 1U)
            {
                ui_draw_rect_on_line(row, 4U, 49U, 3U, 17U, UI_COLOR_MAIN_AMBER);
            }
            else
            {
                ui_draw_rect_on_line(row, 4U, 68U, 3U, 6U, UI_COLOR_MAIN_AMBER);
            }
        }

        ui_draw_zh_on_line(row, 14U, 25U, g_label_brightness, 2U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 86U, 27U, brightness_text, 1U, UI_COLOR_MAIN_AMBER);
        ui_draw_rect_on_line(row, 12U, 42U, 141U, 1U, UI_COLOR_FRAME);

        ui_draw_zh_on_line(row, 14U, 52U, g_label_rotate, 2U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 86U, 52U, rotation_text, 1U, UI_COLOR_MAIN_AMBER);
        ui_draw_ascii_on_line(row, 112U, 52U, "CN", 1U, UI_COLOR_TEXT);
        ui_draw_ascii_on_line(row, 136U, 52U, "ON", 1U, UI_COLOR_MAIN_GREEN);

        if ((action_mode != 0U) && (selected == 2U))
        {
            ui_draw_ascii_on_line(row, 14U, 69U, "TRIG", 1U, UI_COLOR_TEXT);
            ui_draw_ascii_on_line(row, 86U, 69U, (ui_model_trigger_manual(state) != 0U) ? "MAN" : "AUTO", 1U, UI_COLOR_MAIN_AMBER);
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

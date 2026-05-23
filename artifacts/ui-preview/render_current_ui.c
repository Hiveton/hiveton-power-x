#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp_lcd_st7735.h"
#include "ui_model.h"
#include "ui_renderer.h"

#define ATLAS_COLS 3U
#define ATLAS_ROWS 3U
#define ATLAS_WIDTH (LCD_WIDTH * ATLAS_COLS)
#define ATLAS_HEIGHT (LCD_HEIGHT * ATLAS_ROWS)

static uint16_t g_atlas[ATLAS_WIDTH * ATLAS_HEIGHT];
static uint16_t g_origin_x;
static uint16_t g_origin_y;
static uint16_t g_window_x;
static uint16_t g_window_y;
static uint16_t g_window_width;
static uint16_t g_push_index;

static void atlas_put(uint16_t x, uint16_t y, uint16_t color)
{
    if ((x < ATLAS_WIDTH) && (y < ATLAS_HEIGHT))
    {
        g_atlas[((uint32_t)y * ATLAS_WIDTH) + x] = color;
    }
}

static void atlas_clear_tile(uint8_t tile)
{
    uint16_t x;
    uint16_t y;

    g_origin_x = (uint16_t)((tile % ATLAS_COLS) * LCD_WIDTH);
    g_origin_y = (uint16_t)((tile / ATLAS_COLS) * LCD_HEIGHT);
    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        for (x = 0U; x < LCD_WIDTH; ++x)
        {
            atlas_put((uint16_t)(g_origin_x + x), (uint16_t)(g_origin_y + y), 0x0000U);
        }
    }
}

void bsp_lcd_init(void)
{
}

void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    (void)height;
    g_window_x = x;
    g_window_y = y;
    g_window_width = width;
    g_push_index = 0U;
}

void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count)
{
    uint16_t index;

    for (index = 0U; index < count; ++index)
    {
        uint16_t local_x;
        uint16_t local_y;

        local_x = (uint16_t)(g_window_x + (g_push_index % g_window_width));
        local_y = (uint16_t)(g_window_y + (g_push_index / g_window_width));
        atlas_put((uint16_t)(g_origin_x + local_x),
                  (uint16_t)(g_origin_y + local_y),
                  pixels[index]);
        ++g_push_index;
    }
}

void bsp_lcd_fill_color(uint16_t color)
{
    uint16_t x;
    uint16_t y;

    for (y = 0U; y < LCD_HEIGHT; ++y)
    {
        for (x = 0U; x < LCD_WIDTH; ++x)
        {
            atlas_put((uint16_t)(g_origin_x + x), (uint16_t)(g_origin_y + y), color);
        }
    }
}

void bsp_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    uint16_t px;
    uint16_t py;

    for (py = y; py < (uint16_t)(y + height); ++py)
    {
        for (px = x; px < (uint16_t)(x + width); ++px)
        {
            atlas_put((uint16_t)(g_origin_x + px), (uint16_t)(g_origin_y + py), color);
        }
    }
}

void bsp_lcd_draw_test_pattern(void)
{
}

static void ppm_write_rgb565(FILE *file, uint16_t color)
{
    uint8_t rgb[3];

    rgb[0] = (uint8_t)(((color >> 11) & 0x1FU) * 255U / 31U);
    rgb[1] = (uint8_t)(((color >> 5) & 0x3FU) * 255U / 63U);
    rgb[2] = (uint8_t)((color & 0x1FU) * 255U / 31U);
    (void)fwrite(rgb, 1U, sizeof(rgb), file);
}

static void atlas_write_ppm(const char *path)
{
    FILE *file;
    uint32_t i;

    file = fopen(path, "wb");
    if (file == 0)
    {
        return;
    }

    (void)fprintf(file, "P6\n%u %u\n255\n", (unsigned)ATLAS_WIDTH, (unsigned)ATLAS_HEIGHT);
    for (i = 0U; i < (uint32_t)ATLAS_WIDTH * ATLAS_HEIGHT; ++i)
    {
        ppm_write_rgb565(file, g_atlas[i]);
    }
    (void)fclose(file);
}

int main(int argc, char **argv)
{
    measure_snapshot_t measure = {
        .voltage_avg_mv = 9000,
        .current_avg_ma = 2000,
        .power_mw = 18000,
        .voltage_min_mv = 8950,
        .voltage_max_mv = 9050,
        .current_min_ma = 1900,
        .current_max_ma = 2100,
        .ripple_pp_est_mv = 100U,
        .ripple_level = 120U,
        .voltage_valid = 1U,
        .current_valid = 1U,
        .power_valid = 1U,
    };
    protocol_snapshot_t pd = {
        .kind = PROTOCOL_KIND_PD,
        .contract_mv = 9000,
        .contract_ma = 5000,
        .emark_present = 1U,
        .emark_current_a = 5U,
        .emark_usb_speed_grade = 3U,
        .emark_cable_type = 2U,
        .request_state = PROTOCOL_REQUEST_READY,
        .target_mv = 9000,
        .selected_pdo_index = 2U,
        .cc_orientation = 1U,
        .pps_present = 1U,
        .pps_min_mv = 3300,
        .pps_max_mv = 11000,
        .pps_max_ma = 3000,
        .source_fixed_count = 4U,
        .source_fixed_mv = { 5000, 9000, 12000, 20000 },
        .source_fixed_ma = { 3000, 3000, 3000, 3000 },
        .dp_mv = 600,
        .dm_mv = 600,
    };
    protocol_snapshot_t qc = {
        .kind = PROTOCOL_KIND_QC,
        .contract_mv = 9000,
        .request_state = PROTOCOL_REQUEST_READY,
        .target_mv = 9000,
        .dp_mv = 600,
        .dm_mv = 600,
    };
    ui_model_state_t state;
    const char *out_path;

    memset(g_atlas, 0, sizeof(g_atlas));
    ui_model_init(&state);
    ui_model_advance_liveness(&state);
    ui_model_advance_liveness(&state);
    ui_model_advance_liveness(&state);

    atlas_clear_tile(0U);
    ui_renderer_draw_main_page(&state, &measure, &pd);

    atlas_clear_tile(1U);
    ui_renderer_draw_protocol_page(&measure, &pd);

    atlas_clear_tile(2U);
    state.page = UI_PAGE_TRIGGER;
    state.trigger_preset_index = 1U;
    state.action_mode = 1U;
    ui_renderer_draw_trigger_page(&state, &pd);

    atlas_clear_tile(3U);
    state.page = UI_PAGE_CC;
    state.action_mode = 0U;
    ui_renderer_draw_cc_page(&measure, &pd);

    atlas_clear_tile(4U);
    state.page = UI_PAGE_CABLE;
    state.action_mode = 0U;
    ui_renderer_draw_cable_page(&measure, &pd);

    atlas_clear_tile(5U);
    state.page = UI_PAGE_SETTINGS;
    state.settings_selected_index = 0U;
    state.action_mode = 0U;
    ui_renderer_draw_settings_page(&state);

    atlas_clear_tile(6U);
    ui_renderer_draw_scope_page(&measure);

    atlas_clear_tile(7U);
    state.page = UI_PAGE_PDO;
    state.trigger_preset_index = 1U;
    ui_model_pdo_target_set_mv(&state, 9000);
    state.action_mode = 1U;
    ui_renderer_draw_pdo_page(&state, &pd);

    atlas_clear_tile(8U);
    state.page = UI_PAGE_QC;
    state.qc_preset_index = 3U;
    state.action_mode = 1U;
    ui_renderer_draw_qc_page(&state, &qc);

    out_path = (argc > 1) ? argv[1] : "artifacts/ui-preview/current-ui-atlas.ppm";
    atlas_write_ppm(out_path);
    return 0;
}

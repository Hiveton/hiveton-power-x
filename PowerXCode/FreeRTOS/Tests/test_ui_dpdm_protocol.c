#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "bsp_lcd_st7735.h"
#include "ui_renderer.h"

void bsp_lcd_init(void) {}
void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}
void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count)
{
    (void)pixels;
    (void)count;
}
void bsp_lcd_fill_color(uint16_t color)
{
    (void)color;
}
void bsp_lcd_fill_rect(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint16_t color)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
    (void)color;
}
void bsp_lcd_draw_test_pattern(void) {}

static void expect_dpdm(protocol_kind_t kind,
                        int32_t target_mv,
                        int16_t dp_mv,
                        int16_t dm_mv,
                        const char *expected)
{
    protocol_snapshot_t protocol;

    protocol_snapshot_reset(&protocol);
    protocol.kind = kind;
    protocol.target_mv = target_mv;
    protocol.dp_mv = dp_mv;
    protocol.dm_mv = dm_mv;

    assert(strcmp(ui_renderer_infer_dpdm_protocol_for_test(&protocol), expected) == 0);
}

int main(void)
{
    expect_dpdm(PROTOCOL_KIND_PD, 0, 610, 570, "DCP");
    expect_dpdm(PROTOCOL_KIND_QC, 9000, 3300, 600, "QC9");
    expect_dpdm(PROTOCOL_KIND_QC, 12000, 600, 600, "QC12");
    expect_dpdm(PROTOCOL_KIND_OTHER, 0, 2000, 2000, "APL1A");
    expect_dpdm(PROTOCOL_KIND_OTHER, 0, 2700, 3300, "APL24");
    expect_dpdm(PROTOCOL_KIND_OTHER, 0, 1200, 1200, "SAMS");
    expect_dpdm(PROTOCOL_KIND_PD, 0, 0, 0, "PD");
    expect_dpdm(PROTOCOL_KIND_NONE, 0, 0, 0, "---");
    return 0;
}

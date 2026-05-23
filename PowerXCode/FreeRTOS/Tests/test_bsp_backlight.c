#include <assert.h>

#include "bsp_backlight.h"

static void test_backlight_set_tracks_current_percent(void)
{
    bsp_backlight_init();
    assert(bsp_backlight_get_percent() == 0U);

    bsp_backlight_set(25U);
    assert(bsp_backlight_get_percent() == 25U);

    bsp_backlight_set(100U);
    assert(bsp_backlight_get_percent() == 100U);
}

static void test_backlight_set_clamps_to_100_percent(void)
{
    bsp_backlight_init();

    bsp_backlight_set(255U);
    assert(bsp_backlight_get_percent() == 100U);
}

int main(void)
{
    test_backlight_set_tracks_current_percent();
    test_backlight_set_clamps_to_100_percent();
    return 0;
}

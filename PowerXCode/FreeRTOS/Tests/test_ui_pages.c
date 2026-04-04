#include <assert.h>

#include "ui_pages.h"
#include "ui_renderer.h"

static int g_main_calls;
static int g_protocol_calls;
static int g_trigger_calls;
static int g_placeholder_calls;
static ui_page_t g_placeholder_page;
static int g_trigger_selected_mv;

void ui_renderer_init(void)
{
}

void ui_renderer_draw_boot_screen(void)
{
}

void ui_renderer_draw_placeholder_page(ui_page_t page)
{
    g_placeholder_calls++;
    g_placeholder_page = page;
}

void ui_renderer_draw_main_page(const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    (void)measure;
    (void)protocol;
    g_main_calls++;
}

void ui_renderer_draw_protocol_page(const protocol_snapshot_t *protocol)
{
    (void)protocol;
    g_protocol_calls++;
}

void ui_renderer_draw_trigger_page(const ui_model_state_t *state,
                                   const protocol_snapshot_t *protocol)
{
    g_trigger_selected_mv = ui_model_trigger_selected_mv(state);
    (void)protocol;
    g_trigger_calls++;
}

static void reset_counters(void)
{
    g_main_calls = 0;
    g_protocol_calls = 0;
    g_trigger_calls = 0;
    g_placeholder_calls = 0;
    g_placeholder_page = UI_PAGE_MAIN;
    g_trigger_selected_mv = 0;
}

static void test_trigger_page_routes_to_trigger_renderer(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_TRIGGER;
    ui_model_trigger_next(&state);
    ui_model_trigger_next(&state);

    ui_pages_draw(&state, 0, 0);

    assert(g_trigger_calls == 1);
    assert(g_placeholder_calls == 0);
    assert(g_trigger_selected_mv == 12000);
}

static void test_stats_page_still_routes_to_placeholder_renderer(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_STATS;

    ui_pages_draw(&state, 0, 0);

    assert(g_trigger_calls == 0);
    assert(g_placeholder_calls == 1);
    assert(g_placeholder_page == UI_PAGE_STATS);
}

int main(void)
{
    test_trigger_page_routes_to_trigger_renderer();
    test_stats_page_still_routes_to_placeholder_renderer();
    return 0;
}

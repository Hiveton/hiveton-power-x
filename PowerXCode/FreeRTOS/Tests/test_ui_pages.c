#include <assert.h>

#include "ui_pages.h"
#include "ui_renderer.h"

static int g_main_calls;
static int g_scope_calls;
static int g_ripple_calls;
static int g_trigger_select_calls;
static int g_trigger_adjust_calls;
static int g_protocol_calls;
static int g_pdo_calls;
static int g_emark_calls;
static int g_menu_calls;
static int g_settings_calls;
static int g_frame_begin_calls;
static int g_frame_end_calls;
static int g_last_protocol_mv;
static int g_last_protocol_ma;
static int g_last_cable_mv;
static int g_last_cable_valid;
static uint16_t g_last_main_liveness;
static protocol_kind_t g_last_protocol_kind;
static int g_last_protocol_cc_attached;

void ui_renderer_init(void)
{
}

void ui_renderer_draw_boot_screen(void)
{
}

void ui_renderer_begin_frame_stream(void)
{
    g_frame_begin_calls++;
}

void ui_renderer_end_frame_stream(void)
{
    g_frame_end_calls++;
}

void ui_renderer_draw_main_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    (void)measure;
    (void)protocol;
    g_last_main_liveness = ui_model_liveness_frame(state);
    g_main_calls++;
}

void ui_renderer_draw_dpdm_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)measure;
    (void)protocol;
    g_protocol_calls++;
}

void ui_renderer_draw_power_stats_page(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure)
{
    (void)state;
    (void)measure;
    g_scope_calls++;
}

void ui_renderer_draw_capacity_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure)
{
    (void)state;
    (void)measure;
    g_settings_calls++;
}

void ui_renderer_draw_scope_page(const measure_snapshot_t *measure)
{
    (void)measure;
    g_scope_calls++;
}

void ui_renderer_update_scope_history(const measure_snapshot_t *measure)
{
    (void)measure;
}

void ui_renderer_update_ripple_history(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure)
{
    (void)state;
    (void)measure;
}

void ui_renderer_draw_ripple_page(const ui_model_state_t *state,
                                  const measure_snapshot_t *measure)
{
    (void)state;
    (void)measure;
    g_ripple_calls++;
}

void ui_renderer_draw_trigger_select_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)protocol;
    g_trigger_select_calls++;
}

void ui_renderer_draw_trigger_adjust_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)protocol;
    g_trigger_adjust_calls++;
}

void ui_renderer_draw_protocol_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure,
                                    const protocol_snapshot_t *protocol)
{
    (void)state;
    if (protocol != 0)
    {
        g_last_protocol_kind = protocol->kind;
        g_last_protocol_cc_attached = protocol->cc_attached;
    }
    if (measure != 0)
    {
        g_last_protocol_mv = measure->voltage_avg_mv;
        g_last_protocol_ma = measure->current_avg_ma;
    }
    (void)protocol;
    g_protocol_calls++;
}

void ui_renderer_draw_protocol_warning_page(const ui_model_state_t *state)
{
    (void)state;
}

void ui_renderer_draw_pdo_page(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)protocol;
    g_pdo_calls++;
}

void ui_renderer_draw_emark_page(const measure_snapshot_t *measure,
                                 const protocol_snapshot_t *protocol)
{
    if (measure != 0)
    {
        g_last_cable_mv = measure->voltage_avg_mv;
        g_last_cable_valid = measure->voltage_valid;
    }
    (void)protocol;
    g_emark_calls++;
}

void ui_renderer_draw_menu_page(const ui_model_state_t *state)
{
    assert(state != 0);
    g_menu_calls++;
}

void ui_renderer_draw_settings_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure)
{
    assert(state != 0);
    (void)measure;
    g_settings_calls++;
}

static void reset_counters(void)
{
    g_main_calls = 0;
    g_scope_calls = 0;
    g_ripple_calls = 0;
    g_trigger_select_calls = 0;
    g_trigger_adjust_calls = 0;
    g_protocol_calls = 0;
    g_pdo_calls = 0;
    g_emark_calls = 0;
    g_menu_calls = 0;
    g_settings_calls = 0;
    g_frame_begin_calls = 0;
    g_frame_end_calls = 0;
    g_last_protocol_mv = 0;
    g_last_protocol_ma = 0;
    g_last_cable_mv = 0;
    g_last_cable_valid = 0;
    g_last_main_liveness = 0U;
    g_last_protocol_kind = PROTOCOL_KIND_NONE;
    g_last_protocol_cc_attached = 0;
}

static void test_main_page_routes_to_main_renderer(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);

    ui_pages_draw(&state, 0, 0);

    assert(g_main_calls == 1);
    assert(g_frame_begin_calls == 1);
    assert(g_frame_end_calls == 1);
}

static void test_main_page_passes_liveness_frame_to_renderer(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    ui_model_advance_liveness(&state);
    ui_model_advance_liveness(&state);

    ui_pages_draw(&state, 0, 0);

    assert(g_main_calls == 1);
    assert(g_last_main_liveness == 2U);
}

static void test_extra_pages_route_to_dedicated_renderers(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_SCOPE;

    ui_pages_draw(&state, 0, 0);

    assert(g_scope_calls == 1);

    state.page = UI_PAGE_RIPPLE;
    ui_pages_draw(&state, 0, 0);
    assert(g_ripple_calls == 1);

    state.page = UI_PAGE_PDO;
    ui_pages_draw(&state, 0, 0);
    assert(g_pdo_calls == 1);

    state.page = UI_PAGE_TRIGGER_SELECT;
    ui_pages_draw(&state, 0, 0);
    assert(g_trigger_select_calls == 1);

    state.page = UI_PAGE_TRIGGER_ADJUST;
    ui_pages_draw(&state, 0, 0);
    assert(g_trigger_adjust_calls == 1);

    state.page = UI_PAGE_EMARK;
    ui_pages_draw(&state, 0, 0);
    assert(g_emark_calls == 1);

    state.page = UI_PAGE_MENU;
    ui_pages_draw(&state, 0, 0);
    assert(g_menu_calls == 1);

    state.page = UI_PAGE_SETTINGS;
    ui_pages_draw(&state, 0, 0);
    assert(g_settings_calls == 1);
}

static void test_vbus_without_protocol_renders_as_other(void)
{
    ui_model_state_t state;
    measure_snapshot_t measure = {
        .voltage_avg_mv = 5000,
        .current_avg_ma = 1200,
        .voltage_valid = 1U,
        .current_valid = 1U,
    };
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;

    ui_pages_draw(&state, &measure, &protocol);

    assert(g_protocol_calls == 1);
    assert(g_last_protocol_kind == PROTOCOL_KIND_OTHER);
    assert(g_last_protocol_mv == 5000);
    assert(g_last_protocol_ma == 1200);
}

static void test_elevated_vbus_without_pd_does_not_guess_qc(void)
{
    ui_model_state_t state;
    measure_snapshot_t measure = {
        .voltage_avg_mv = 9000,
        .current_avg_ma = 800,
        .voltage_valid = 1U,
        .current_valid = 1U,
    };
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;

    ui_pages_draw(&state, &measure, &protocol);

    assert(g_protocol_calls == 1);
    assert(g_last_protocol_kind == PROTOCOL_KIND_OTHER);
    assert(g_last_protocol_mv == 9000);
    assert(g_last_protocol_ma == 800);
}

static void test_vbus_with_cc_attached_does_not_fallback_to_other(void)
{
    ui_model_state_t state;
    measure_snapshot_t measure = {
        .voltage_avg_mv = 5000,
        .current_avg_ma = 500,
        .voltage_valid = 1U,
        .current_valid = 1U,
    };
    protocol_snapshot_t protocol;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;
    protocol_snapshot_reset(&protocol);
    protocol_snapshot_set_cc_orientation(&protocol, 2U);

    ui_pages_draw(&state, &measure, &protocol);

    assert(g_protocol_calls == 1);
    assert(g_last_protocol_kind == PROTOCOL_KIND_NONE);
    assert(g_last_protocol_cc_attached == 1);
    assert(g_last_protocol_mv == 5000);
}

static void test_vbus_with_cc_orientation_does_not_fallback_to_other(void)
{
    ui_model_state_t state;
    measure_snapshot_t measure = {
        .voltage_avg_mv = 5000,
        .current_avg_ma = 500,
        .voltage_valid = 1U,
        .current_valid = 1U,
    };
    protocol_snapshot_t protocol;

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;
    protocol_snapshot_reset(&protocol);
    protocol.cc_orientation = 1U;

    ui_pages_draw(&state, &measure, &protocol);

    assert(g_protocol_calls == 1);
    assert(g_last_protocol_kind == PROTOCOL_KIND_NONE);
    assert(g_last_protocol_mv == 5000);
}

static void test_emark_page_receives_vbus_measurement(void)
{
    ui_model_state_t state;
    measure_snapshot_t measure = {
        .voltage_avg_mv = 5000,
        .voltage_valid = 1U,
    };
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };

    reset_counters();
    ui_model_init(&state);
    state.page = UI_PAGE_EMARK;

    ui_pages_draw(&state, &measure, &protocol);

    assert(g_emark_calls == 1);
    assert(g_last_cable_valid == 1);
    assert(g_last_cable_mv == 5000);
}

static void test_invalid_page_falls_back_to_product_home(void)
{
    ui_model_state_t state;

    reset_counters();
    ui_model_init(&state);
    state.page = (ui_page_t)99;

    ui_pages_draw(&state, 0, 0);

    assert(g_main_calls == 1);
}

int main(void)
{
    test_main_page_routes_to_main_renderer();
    test_main_page_passes_liveness_frame_to_renderer();
    test_extra_pages_route_to_dedicated_renderers();
    test_vbus_without_protocol_renders_as_other();
    test_elevated_vbus_without_pd_does_not_guess_qc();
    test_vbus_with_cc_attached_does_not_fallback_to_other();
    test_vbus_with_cc_orientation_does_not_fallback_to_other();
    test_emark_page_receives_vbus_measurement();
    test_invalid_page_falls_back_to_product_home();
    return 0;
}

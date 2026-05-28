#include <assert.h>
#include <stdint.h>

#include "app_ui_navigation.h"
#include "service_pd.h"

static int g_source_cap_request_calls;
static int g_sink_hold_calls;
static int g_pd_request_calls;
static uint8_t g_last_request_position;
static int32_t g_last_request_mv;
static service_pd_source_caps_snapshot_t g_caps;

void service_pd_request_source_capabilities(void)
{
    g_source_cap_request_calls++;
}

void service_pd_set_sink_hold(uint8_t enabled)
{
    g_sink_hold_calls += (enabled != 0U) ? 1 : -1;
}

uint8_t service_pd_request_pdo_position(uint8_t position, int32_t target_mv)
{
    g_pd_request_calls++;
    g_last_request_position = position;
    g_last_request_mv = target_mv;
    return 1U;
}

void service_pd_copy_source_caps(service_pd_source_caps_snapshot_t *snapshot)
{
    if (snapshot != 0)
    {
        *snapshot = g_caps;
    }
}

static void reset_apply_calls(void)
{
    g_source_cap_request_calls = 0;
    g_sink_hold_calls = 0;
    g_pd_request_calls = 0;
    g_last_request_position = 0U;
    g_last_request_mv = 0;
    g_caps = (service_pd_source_caps_snapshot_t){ 0 };
}

static void test_btn2_long_opens_menu(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };
    uint8_t redraw;

    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;
    keys.btn2_long = 1U;

    redraw = app_ui_navigation_apply(&state, &protocol, &keys);

    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_menu_selected_index(&state) == 0U);
    assert(redraw == 1U);
}

static void test_btn2_long_from_actionable_page_opens_menu_after_initial_short(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_PDO;
    state.action_mode = 1U;

    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_menu_selects_settings_and_long_returns_stack(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;

    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_menu_selected_index(&state) == 5U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SETTINGS);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(ui_model_action_mode(&state) == 0U);
}

static void test_btn1_btn3_switch_pages(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_DPDM);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_btn3_long_opens_trigger_and_holds_rd(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);

    keys.btn3_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER_SELECT);
    assert(ui_model_action_mode(&state) == 1U);
    assert(g_sink_hold_calls == 1);
    assert(g_source_cap_request_calls == 1);
}

static void test_btn1_long_steps_back_and_btn3_long_opens_trigger(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    keys.btn3_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER_SELECT);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_RIPPLE);
}

static void test_btn1_btn3_browse_all_pages_and_wrap(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_DPDM);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_POWER_STATS);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CAPACITY);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SCOPE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_RIPPLE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_RIPPLE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SCOPE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CAPACITY);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_POWER_STATS);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_DPDM);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_menu_emark_read_opens_emark_page_and_long_returns(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    ui_model_open_menu(&state);

    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_menu_selected_index(&state) == 2U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_EMARK);
    assert(ui_model_action_mode(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_power_stats_confirm_toggles_max_average(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_POWER_STATS;
    assert(ui_model_power_stats_average(&state) == 0U);

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_POWER_STATS);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_power_stats_average(&state) == 1U);

    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_POWER_STATS);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_power_stats_average(&state) == 0U);
}

static void test_capacity_confirm_toggles_mah_wh(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_CAPACITY;
    assert(ui_model_capacity_show_wh(&state) == 0U);

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CAPACITY);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_capacity_show_wh(&state) == 1U);

    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CAPACITY);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_capacity_show_wh(&state) == 0U);
}

static void test_ripple_confirm_pauses_and_long_confirm_opens_menu(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_RIPPLE;
    assert(ui_model_ripple_paused(&state) == 0U);
    assert(ui_model_ripple_frequency_hz(&state) == 100000U);

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_RIPPLE);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_ripple_paused(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_ripple_frequency_hz(&state) == 100000U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PDO);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_pdo_action_mode_scrolls_from_menu(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);

    ui_model_open_menu(&state);
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PDO);
    assert(ui_model_pdo_scroll(&state) == 0U);
    assert(g_source_cap_request_calls == 0);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_pdo_scroll(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
}

static void test_protocol_detection_menu_opens_warning_then_result_or_cancel(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    ui_model_open_menu(&state);

    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_menu_selected_index(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL_WARNING);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_protocol_warning_confirm_selected(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(ui_model_action_mode(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL_WARNING);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_protocol_warning_confirm_selected(&state) == 0U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_pdo_action_mode_scrolls_without_pd_request(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);
    state.page = UI_PAGE_PDO;
    state.action_mode = 1U;
    assert(ui_model_pdo_scroll(&state) == 0U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_pdo_scroll(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_trigger_select_requests_fixed_pdo_and_adjusts_pps(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    g_caps.count = 2U;
    g_caps.pdos[0] = (service_pd_source_pdo_t){
        .position = 1U,
        .type = SERVICE_PD_SOURCE_PDO_FIXED,
        .min_mv = 5000U,
        .max_mv = 5000U,
        .current_ma = 3000U,
    };
    g_caps.pdos[1] = (service_pd_source_pdo_t){
        .position = 4U,
        .type = SERVICE_PD_SOURCE_PDO_PPS,
        .min_mv = 5000U,
        .max_mv = 11000U,
        .current_ma = 3000U,
    };
    ui_model_init(&state);
    ui_model_open_trigger(&state);

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_pd_request_calls == 1);
    assert(g_last_request_position == 1U);
    assert(g_last_request_mv == 5000);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_selected_index(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER_ADJUST);
    assert(ui_model_trigger_target_mv(&state) == 5000);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_target_mv(&state) == 5020);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_pd_request_calls == 2);
    assert(g_last_request_position == 4U);
    assert(g_last_request_mv == 5020);
}

static void test_settings_action_mode_selects_setting_rows(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_SETTINGS;
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_settings_selected_index(&state) == 0U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_settings_selected_index(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_settings_selected_index(&state) == 0U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
}

static void test_settings_confirm_activates_selected_row_without_exit(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_SETTINGS;
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_brightness_percent(&state) == 100U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_brightness_percent(&state) == 25U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_settings_selected_index(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_rotation_degrees(&state) == 180U);
}

int main(void)
{
    test_btn2_long_opens_menu();
    test_btn2_long_from_actionable_page_opens_menu_after_initial_short();
    test_menu_selects_settings_and_long_returns_stack();
    test_btn1_btn3_switch_pages();
    test_btn3_long_opens_trigger_and_holds_rd();
    test_btn1_long_steps_back_and_btn3_long_opens_trigger();
    test_btn1_btn3_browse_all_pages_and_wrap();
    test_menu_emark_read_opens_emark_page_and_long_returns();
    test_power_stats_confirm_toggles_max_average();
    test_capacity_confirm_toggles_mah_wh();
    test_ripple_confirm_pauses_and_long_confirm_opens_menu();
    test_pdo_action_mode_scrolls_from_menu();
    test_protocol_detection_menu_opens_warning_then_result_or_cancel();
    test_pdo_action_mode_scrolls_without_pd_request();
    test_trigger_select_requests_fixed_pdo_and_adjusts_pps();
    test_settings_action_mode_selects_setting_rows();
    test_settings_confirm_activates_selected_row_without_exit();
    return 0;
}

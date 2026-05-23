#include <assert.h>
#include <stdint.h>

#include "app_ui_navigation.h"

static int g_auto_apply_calls;
static int g_pd_apply_calls;
static int g_qc_apply_calls;
static int g_pd_apply_target_mv;

void app_trigger_control_apply_auto(const ui_model_state_t *state,
                                    const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)protocol;
    g_auto_apply_calls++;
}

void app_trigger_control_apply_pd(const ui_model_state_t *state,
                                  const protocol_snapshot_t *protocol)
{
    (void)protocol;
    g_pd_apply_target_mv = ui_model_pdo_target_mv(state);
    g_pd_apply_calls++;
}

void app_trigger_control_apply_qc(const ui_model_state_t *state,
                                  const protocol_snapshot_t *protocol)
{
    (void)state;
    (void)protocol;
    g_qc_apply_calls++;
}

static void reset_apply_calls(void)
{
    g_auto_apply_calls = 0;
    g_pd_apply_calls = 0;
    g_qc_apply_calls = 0;
    g_pd_apply_target_mv = 0;
}

static void test_btn2_long_opens_settings_quick_menu(void)
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

    assert(state.page == UI_PAGE_SETTINGS);
    assert(ui_model_action_mode(&state) == 1U);
    assert(redraw == 1U);
}

static void test_btn2_long_from_actionable_page_opens_settings_after_initial_short(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_TRIGGER;

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER);
    assert(ui_model_action_mode(&state) == 1U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SETTINGS);
    assert(ui_model_action_mode(&state) == 1U);
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
    assert(state.page == UI_PAGE_SCOPE);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_btn1_btn3_long_events_switch_pages(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_NONE,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    keys.btn3_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SCOPE);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);
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
    assert(state.page == UI_PAGE_SCOPE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PDO);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_QC);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CC);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CABLE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SETTINGS);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SETTINGS);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CABLE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_CC);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_QC);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PDO);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_TRIGGER);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_SCOPE);
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_trigger_uses_action_mode_before_confirm(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);
    state.page = UI_PAGE_TRIGGER;
    keys.btn2_short = 1U;

    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(g_auto_apply_calls == 0);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_selected_mv(&state) == 9000);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_auto_apply_calls == 1);
    assert(ui_model_action_mode(&state) == 0U);
}

static void test_btn1_btn3_long_events_step_action_mode(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_TRIGGER;
    (void)ui_model_enter_action_mode(&state);

    keys.btn3_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_selected_mv(&state) == 9000);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn1_long = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_selected_mv(&state) == 5000);
}

static void test_pdo_and_qc_confirm_route_to_dedicated_actions(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);

    state.page = UI_PAGE_PDO;
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_pd_apply_calls == 1);

    state.page = UI_PAGE_QC;
    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_qc_selected_mv(&state) == 9000);
    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_qc_apply_calls == 1);
}

static void test_pdo_action_mode_steps_fine_target_before_confirm(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
        .pps_present = 1U,
        .pps_min_mv = 3300,
        .pps_max_mv = 21000,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);
    state.page = UI_PAGE_PDO;

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_pdo_target_mv(&state) == 5000);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn3_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_pdo_target_mv(&state) == 5020);
    assert(g_pd_apply_calls == 0);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(g_pd_apply_calls == 1);
    assert(g_pd_apply_target_mv == 5020);
    assert(ui_model_action_mode(&state) == 0U);
}

static void test_pdo_action_mode_starts_from_current_pd_target(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
        .target_mv = 9000,
        .contract_mv = 9000,
        .request_state = PROTOCOL_REQUEST_READY,
        .pps_present = 1U,
        .pps_min_mv = 3300,
        .pps_max_mv = 21000,
    };
    bsp_keys_event_t keys = { 0 };

    ui_model_init(&state);
    state.page = UI_PAGE_PDO;
    assert(ui_model_pdo_target_mv(&state) == 5000);

    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_pdo_target_mv(&state) == 9000);
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
    assert(ui_model_action_mode(&state) == 0U);
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
    assert(ui_model_settings_selected_index(&state) == 2U);

    keys = (bsp_keys_event_t){ 0 };
    keys.btn2_short = 1U;
    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_manual(&state) == 0U);
}

static void test_auto_trigger_mode_applies_target_while_changing_selection(void)
{
    ui_model_state_t state;
    protocol_snapshot_t protocol = {
        .kind = PROTOCOL_KIND_PD,
    };
    bsp_keys_event_t keys = { 0 };

    reset_apply_calls();
    ui_model_init(&state);
    state.page = UI_PAGE_SETTINGS;
    state.settings_selected_index = 2U;
    ui_model_settings_activate(&state);
    assert(ui_model_trigger_manual(&state) == 0U);

    state.page = UI_PAGE_TRIGGER;
    (void)ui_model_enter_action_mode(&state);
    keys.btn3_short = 1U;

    assert(app_ui_navigation_apply(&state, &protocol, &keys) == 1U);
    assert(ui_model_trigger_selected_mv(&state) == 9000);
    assert(g_auto_apply_calls == 1);
    assert(ui_model_action_mode(&state) == 1U);
}

int main(void)
{
    test_btn2_long_opens_settings_quick_menu();
    test_btn2_long_from_actionable_page_opens_settings_after_initial_short();
    test_btn1_btn3_switch_pages();
    test_btn1_btn3_long_events_switch_pages();
    test_btn1_btn3_browse_all_pages_and_wrap();
    test_trigger_uses_action_mode_before_confirm();
    test_btn1_btn3_long_events_step_action_mode();
    test_pdo_and_qc_confirm_route_to_dedicated_actions();
    test_pdo_action_mode_steps_fine_target_before_confirm();
    test_pdo_action_mode_starts_from_current_pd_target();
    test_settings_action_mode_selects_setting_rows();
    test_settings_confirm_activates_selected_row_without_exit();
    test_auto_trigger_mode_applies_target_while_changing_selection();
    return 0;
}

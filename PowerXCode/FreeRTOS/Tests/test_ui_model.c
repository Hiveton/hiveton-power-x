#include <assert.h>

#include "ui_model.h"

static void test_page_navigation(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(state.page == UI_PAGE_MAIN);
    assert(ui_model_action_mode(&state) == 0U);
    assert(ui_model_brightness_percent(&state) == 100U);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_SCOPE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_SCOPE);
}

static void test_forward_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_SCOPE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_TRIGGER);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PDO);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_QC);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_CC);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_CABLE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_SETTINGS);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_reverse_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_SETTINGS);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_CABLE);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_CC);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_QC);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_PDO);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_TRIGGER);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_SCOPE);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_trigger_preset_navigation(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_trigger_selected_mv(&state) == 5000);

    ui_model_trigger_next(&state);
    assert(ui_model_trigger_selected_mv(&state) == 9000);

    ui_model_trigger_next(&state);
    assert(ui_model_trigger_selected_mv(&state) == 12000);

    ui_model_trigger_next(&state);
    assert(ui_model_trigger_selected_mv(&state) == 15000);

    ui_model_trigger_next(&state);
    assert(ui_model_trigger_selected_mv(&state) == 20000);

    ui_model_trigger_next(&state);
    assert(ui_model_trigger_selected_mv(&state) == 5000);
}

static void test_trigger_preset_reverse_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_trigger_prev(&state);
    assert(ui_model_trigger_selected_mv(&state) == 20000);

    ui_model_trigger_prev(&state);
    assert(ui_model_trigger_selected_mv(&state) == 15000);
}

static void test_pdo_fine_target_steps_in_20mv_units(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_pdo_target_mv(&state) == 5000);

    ui_model_pdo_target_next(&state);
    assert(ui_model_pdo_target_mv(&state) == 5020);

    ui_model_pdo_target_prev(&state);
    assert(ui_model_pdo_target_mv(&state) == 5000);

    ui_model_pdo_target_prev(&state);
    assert(ui_model_pdo_target_mv(&state) == 4980);
}

static void test_action_mode_and_qc_target_navigation(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_enter_action_mode(&state) == 0U);

    state.page = UI_PAGE_TRIGGER;
    assert(ui_model_enter_action_mode(&state) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    ui_model_exit_action_mode(&state);
    assert(ui_model_action_mode(&state) == 0U);

    state.page = UI_PAGE_QC;
    assert(ui_model_qc_selected_mv(&state) == 5000);
    ui_model_qc_next(&state);
    assert(ui_model_qc_selected_mv(&state) == 9000);
    ui_model_qc_next(&state);
    assert(ui_model_qc_selected_mv(&state) == 12000);
    ui_model_qc_next(&state);
    assert(ui_model_qc_selected_mv(&state) == 15000);
    ui_model_qc_next(&state);
    assert(ui_model_qc_selected_mv(&state) == 20000);
    ui_model_qc_prev(&state);
    assert(ui_model_qc_selected_mv(&state) == 15000);
}

static void test_settings_selection_wraps(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_settings_selected_index(&state) == 0U);
    ui_model_settings_next(&state);
    assert(ui_model_settings_selected_index(&state) == 1U);
    ui_model_settings_prev(&state);
    assert(ui_model_settings_selected_index(&state) == 0U);
    ui_model_settings_prev(&state);
    assert(ui_model_settings_selected_index(&state) == 2U);
}

static void test_settings_activation_changes_values(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_brightness_percent(&state) == 100U);
    ui_model_settings_activate(&state);
    assert(ui_model_brightness_percent(&state) == 25U);

    ui_model_settings_next(&state);
    assert(ui_model_rotation_degrees(&state) == 0U);
    ui_model_settings_activate(&state);
    assert(ui_model_rotation_degrees(&state) == 180U);

    ui_model_settings_next(&state);
    assert(ui_model_trigger_manual(&state) == 1U);
    ui_model_settings_activate(&state);
    assert(ui_model_trigger_manual(&state) == 0U);
}

static void test_liveness_frame_advances_for_home_heartbeat(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_liveness_frame(&state) == 0U);

    ui_model_advance_liveness(&state);
    assert(ui_model_liveness_frame(&state) == 1U);

    ui_model_advance_liveness(&state);
    assert(ui_model_liveness_frame(&state) == 2U);
}

int main(void)
{
    test_page_navigation();
    test_forward_wraparound();
    test_reverse_wraparound();
    test_trigger_preset_navigation();
    test_trigger_preset_reverse_wraparound();
    test_pdo_fine_target_steps_in_20mv_units();
    test_action_mode_and_qc_target_navigation();
    test_settings_selection_wraps();
    test_settings_activation_changes_values();
    test_liveness_frame_advances_for_home_heartbeat();
    return 0;
}

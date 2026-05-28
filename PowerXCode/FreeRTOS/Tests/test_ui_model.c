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
    assert(state.page == UI_PAGE_DPDM);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_POWER_STATS);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_DPDM);
}

static void test_forward_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_DPDM);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_POWER_STATS);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_CAPACITY);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_SCOPE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_RIPPLE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_reverse_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_RIPPLE);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_MAIN);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_RIPPLE);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_SCOPE);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_CAPACITY);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_POWER_STATS);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_DPDM);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_menu_stack_and_scroll(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    state.page = UI_PAGE_PROTOCOL;
    ui_model_open_menu(&state);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_menu_selected_index(&state) == 0U);
    assert(ui_model_menu_scroll(&state) == 0U);

    ui_model_menu_activate(&state);
    assert(state.page == UI_PAGE_PDO);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_back(&state);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_next(&state);
    assert(ui_model_menu_selected_index(&state) == 1U);
    ui_model_menu_activate(&state);
    assert(state.page == UI_PAGE_PROTOCOL_WARNING);
    assert(ui_model_protocol_warning_confirm_selected(&state) == 1U);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_protocol_warning_toggle(&state);
    assert(ui_model_protocol_warning_confirm_selected(&state) == 0U);
    ui_model_protocol_warning_toggle(&state);
    assert(ui_model_protocol_warning_confirm_selected(&state) == 1U);

    ui_model_protocol_warning_accept(&state);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_back(&state);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_next(&state);
    assert(ui_model_menu_selected_index(&state) == 2U);
    ui_model_menu_activate(&state);
    assert(state.page == UI_PAGE_EMARK);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_back(&state);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_prev(&state);
    assert(ui_model_menu_selected_index(&state) == 1U);
    ui_model_menu_prev(&state);
    assert(ui_model_menu_selected_index(&state) == 0U);
    assert(ui_model_menu_scroll(&state) == 0U);

    ui_model_menu_prev(&state);
    assert(ui_model_menu_selected_index(&state) == 5U);
    assert(ui_model_menu_scroll(&state) == 1U);

    ui_model_menu_activate(&state);
    assert(state.page == UI_PAGE_SETTINGS);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_back(&state);
    assert(state.page == UI_PAGE_MENU);
    assert(ui_model_action_mode(&state) == 1U);

    ui_model_menu_back(&state);
    assert(state.page == UI_PAGE_PROTOCOL);
    assert(ui_model_action_mode(&state) == 0U);
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

static void test_pdo_scroll_clamps(void)
{
    ui_model_state_t state;
    uint8_t index;

    ui_model_init(&state);
    assert(ui_model_pdo_scroll(&state) == 0U);

    ui_model_pdo_scroll_prev(&state);
    assert(ui_model_pdo_scroll(&state) == 0U);

    for (index = 0U; index < 8U; ++index)
    {
        ui_model_pdo_scroll_next(&state);
    }
    assert(ui_model_pdo_scroll(&state) == 6U);

    ui_model_pdo_scroll_prev(&state);
    assert(ui_model_pdo_scroll(&state) == 5U);
}

static void test_trigger_page_selects_pdos_and_adjusts_range_voltage(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_open_trigger(&state);
    assert(state.page == UI_PAGE_TRIGGER_SELECT);
    assert(ui_model_action_mode(&state) == 1U);
    assert(ui_model_trigger_selected_index(&state) == 0U);
    assert(ui_model_trigger_scroll(&state) == 0U);

    ui_model_trigger_select_next(&state, 4U);
    assert(ui_model_trigger_selected_index(&state) == 1U);
    ui_model_trigger_select_prev(&state, 4U);
    assert(ui_model_trigger_selected_index(&state) == 0U);
    ui_model_trigger_select_prev(&state, 4U);
    assert(ui_model_trigger_selected_index(&state) == 3U);

    ui_model_trigger_enter_adjust(&state, 5000, 11000, 20, 9000);
    assert(state.page == UI_PAGE_TRIGGER_ADJUST);
    assert(ui_model_trigger_target_mv(&state) == 9000);

    ui_model_trigger_voltage_next(&state);
    assert(ui_model_trigger_target_mv(&state) == 9020);
    ui_model_trigger_voltage_prev(&state);
    assert(ui_model_trigger_target_mv(&state) == 9000);

    ui_model_trigger_enter_adjust(&state, 9000, 20000, 100, 25000);
    assert(ui_model_trigger_target_mv(&state) == 20000);
    ui_model_trigger_voltage_next(&state);
    assert(ui_model_trigger_target_mv(&state) == 20000);
}

static void test_action_mode_for_active_menu_pages(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_enter_action_mode(&state) == 0U);

    state.page = UI_PAGE_PDO;
    assert(ui_model_enter_action_mode(&state) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
    ui_model_exit_action_mode(&state);
    assert(ui_model_action_mode(&state) == 0U);

    state.page = UI_PAGE_TRIGGER_SELECT;
    assert(ui_model_enter_action_mode(&state) == 1U);
    assert(ui_model_action_mode(&state) == 1U);
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
    assert(ui_model_settings_selected_index(&state) == 1U);
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

static void test_power_stats_mode_toggles_between_max_and_average(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_power_stats_average(&state) == 0U);

    ui_model_power_stats_toggle(&state);
    assert(ui_model_power_stats_average(&state) == 1U);

    ui_model_power_stats_toggle(&state);
    assert(ui_model_power_stats_average(&state) == 0U);
}

static void test_capacity_mode_toggles_between_mah_and_wh(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_capacity_show_wh(&state) == 0U);

    ui_model_capacity_toggle(&state);
    assert(ui_model_capacity_show_wh(&state) == 1U);

    ui_model_capacity_toggle(&state);
    assert(ui_model_capacity_show_wh(&state) == 0U);
}

static void test_protocol_scroll_clamps(void)
{
    ui_model_state_t state;
    unsigned int index;

    ui_model_init(&state);
    assert(ui_model_protocol_scroll(&state) == 0U);

    for (index = 0U; index < 16U; ++index)
    {
        ui_model_protocol_scroll_next(&state);
    }
    assert(ui_model_protocol_scroll(&state) == 9U);

    for (index = 0U; index < 16U; ++index)
    {
        ui_model_protocol_scroll_prev(&state);
    }
    assert(ui_model_protocol_scroll(&state) == 0U);
}

static void test_ripple_pause_and_frequency_controls(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(ui_model_ripple_paused(&state) == 0U);
    assert(ui_model_ripple_frequency_hz(&state) == 100000U);

    ui_model_ripple_toggle_pause(&state);
    assert(ui_model_ripple_paused(&state) == 1U);
    ui_model_ripple_toggle_pause(&state);
    assert(ui_model_ripple_paused(&state) == 0U);

    ui_model_ripple_frequency_next(&state);
    assert(ui_model_ripple_frequency_hz(&state) == 500000U);
    ui_model_ripple_frequency_next(&state);
    assert(ui_model_ripple_frequency_hz(&state) == 1000U);
    ui_model_ripple_frequency_prev(&state);
    assert(ui_model_ripple_frequency_hz(&state) == 500000U);
}

int main(void)
{
    test_page_navigation();
    test_forward_wraparound();
    test_reverse_wraparound();
    test_menu_stack_and_scroll();
    test_pdo_fine_target_steps_in_20mv_units();
    test_pdo_scroll_clamps();
    test_trigger_page_selects_pdos_and_adjusts_range_voltage();
    test_action_mode_for_active_menu_pages();
    test_settings_selection_wraps();
    test_settings_activation_changes_values();
    test_liveness_frame_advances_for_home_heartbeat();
    test_power_stats_mode_toggles_between_max_and_average();
    test_capacity_mode_toggles_between_mah_and_wh();
    test_protocol_scroll_clamps();
    test_ripple_pause_and_frequency_controls();
    return 0;
}

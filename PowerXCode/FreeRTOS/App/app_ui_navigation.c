#include "app_ui_navigation.h"

#include "service_pd.h"

static volatile uint8_t g_pdo_active_source_cap_refresh_enable;

static uint8_t app_ui_navigation_trigger_selected_pdo(const ui_model_state_t *state,
                                                       service_pd_source_pdo_t *pdo)
{
    service_pd_source_caps_snapshot_t caps;
    uint8_t selected;

    if ((state == 0) || (pdo == 0))
    {
        return 0U;
    }

    service_pd_copy_source_caps(&caps);
    selected = ui_model_trigger_selected_index(state);
    if ((caps.count == 0U) || (selected >= caps.count) || (selected >= SERVICE_PD_SOURCE_PDO_MAX))
    {
        return 0U;
    }

    *pdo = caps.pdos[selected];
    return 1U;
}

static uint8_t app_ui_navigation_trigger_is_adjustable(const service_pd_source_pdo_t *pdo)
{
    if (pdo == 0)
    {
        return 0U;
    }

    return ((pdo->type == SERVICE_PD_SOURCE_PDO_PPS) ||
            (pdo->type == SERVICE_PD_SOURCE_PDO_AVS) ||
            (pdo->type == SERVICE_PD_SOURCE_PDO_VARIABLE)) ? 1U : 0U;
}

static int app_ui_navigation_trigger_step_mv(const service_pd_source_pdo_t *pdo)
{
    if (pdo == 0)
    {
        return 20;
    }

    return (pdo->type == SERVICE_PD_SOURCE_PDO_PPS) ? 20 : 100;
}

static int app_ui_navigation_trigger_default_mv(const service_pd_source_pdo_t *pdo,
                                                const protocol_snapshot_t *protocol)
{
    if (pdo == 0)
    {
        return 5000;
    }

    if ((protocol != 0) &&
        (protocol->target_mv >= (int32_t)pdo->min_mv) &&
        (protocol->target_mv <= (int32_t)pdo->max_mv))
    {
        return protocol->target_mv;
    }
    if ((protocol != 0) &&
        (protocol->contract_mv >= (int32_t)pdo->min_mv) &&
        (protocol->contract_mv <= (int32_t)pdo->max_mv))
    {
        return protocol->contract_mv;
    }
    if ((pdo->min_mv <= 5000U) && (pdo->max_mv >= 5000U))
    {
        return 5000;
    }

    return pdo->min_mv;
}

static void app_ui_navigation_open_trigger(ui_model_state_t *state)
{
    service_pd_set_sink_hold(1U);
    service_pd_request_source_capabilities();
    ui_model_open_trigger(state);
}

static void app_ui_navigation_trigger_select_next(ui_model_state_t *state)
{
    service_pd_source_caps_snapshot_t caps;

    service_pd_copy_source_caps(&caps);
    ui_model_trigger_select_next(state, caps.count);
}

static void app_ui_navigation_trigger_select_prev(ui_model_state_t *state)
{
    service_pd_source_caps_snapshot_t caps;

    service_pd_copy_source_caps(&caps);
    ui_model_trigger_select_prev(state, caps.count);
}

static void app_ui_navigation_trigger_confirm(ui_model_state_t *state,
                                              const protocol_snapshot_t *protocol)
{
    service_pd_source_pdo_t pdo;

    if (app_ui_navigation_trigger_selected_pdo(state, &pdo) == 0U)
    {
        service_pd_request_source_capabilities();
        return;
    }

    if ((state->page == UI_PAGE_TRIGGER_SELECT) &&
        (app_ui_navigation_trigger_is_adjustable(&pdo) != 0U))
    {
        ui_model_trigger_enter_adjust(state,
                                      pdo.min_mv,
                                      pdo.max_mv,
                                      app_ui_navigation_trigger_step_mv(&pdo),
                                      app_ui_navigation_trigger_default_mv(&pdo, protocol));
        return;
    }

    (void)service_pd_request_pdo_position(pdo.position,
                                          (state->page == UI_PAGE_TRIGGER_ADJUST) ?
                                          ui_model_trigger_target_mv(state) :
                                          pdo.max_mv);
}

static void app_ui_navigation_sync_pdo_target(ui_model_state_t *state,
                                              const protocol_snapshot_t *protocol)
{
    int target_mv;

    if ((state == 0) || (state->page != UI_PAGE_PDO))
    {
        return;
    }

    target_mv = 0;
    if ((protocol != 0) && (protocol->target_mv > 0))
    {
        target_mv = protocol->target_mv;
    }
    else if ((protocol != 0) && (protocol->contract_mv > 0))
    {
        target_mv = protocol->contract_mv;
    }
    else
    {
        return;
    }

    ui_model_pdo_target_set_mv(state, target_mv);
}

static void app_ui_navigation_activate_menu(ui_model_state_t *state)
{
    uint8_t selected_index;

    if (state == 0)
    {
        return;
    }

    selected_index = ui_model_menu_selected_index(state);
    ui_model_menu_activate(state);
    if ((state->page == UI_PAGE_PDO) && (g_pdo_active_source_cap_refresh_enable != 0U))
    {
        service_pd_request_source_capabilities();
    }
    (void)selected_index;
}

uint8_t app_ui_navigation_apply(ui_model_state_t *state,
                                const protocol_snapshot_t *protocol,
                                const bsp_keys_event_t *keys)
{
    uint8_t btn1_step;
    uint8_t btn3_step;
    uint8_t redraw;

    if ((state == 0) || (keys == 0))
    {
        return 0U;
    }

    btn1_step = (uint8_t)((keys->btn1_short != 0U) || (keys->btn1_long != 0U));
    btn3_step = (uint8_t)(keys->btn3_short != 0U);
    redraw = 0U;
    if (keys->btn2_long != 0U)
    {
        if (state->page == UI_PAGE_TRIGGER_ADJUST)
        {
            ui_model_trigger_back_to_select(state);
            redraw = 1U;
            return redraw;
        }
        if (state->page == UI_PAGE_TRIGGER_SELECT)
        {
            service_pd_set_sink_hold(0U);
            ui_model_menu_back(state);
            redraw = 1U;
            return redraw;
        }
        if ((state->page == UI_PAGE_MENU) ||
            (state->page == UI_PAGE_SETTINGS) ||
            (state->page == UI_PAGE_PDO) ||
            (state->page == UI_PAGE_EMARK) ||
            (state->page == UI_PAGE_PROTOCOL_WARNING) ||
            ((state->page == UI_PAGE_PROTOCOL) && (ui_model_action_mode(state) != 0U)))
        {
            ui_model_menu_back(state);
        }
        else
        {
            ui_model_open_menu(state);
        }
        redraw = 1U;
        return redraw;
    }

    if (ui_model_action_mode(state) != 0U)
    {
        if (btn1_step != 0U)
        {
            if (state->page == UI_PAGE_PDO)
            {
                ui_model_pdo_scroll_prev(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL)
            {
                ui_model_protocol_scroll_prev(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL_WARNING)
            {
                ui_model_protocol_warning_toggle(state);
            }
            else if (state->page == UI_PAGE_RIPPLE)
            {
                ui_model_ripple_frequency_prev(state);
            }
            else if (state->page == UI_PAGE_TRIGGER_SELECT)
            {
                app_ui_navigation_trigger_select_prev(state);
            }
            else if (state->page == UI_PAGE_TRIGGER_ADJUST)
            {
                ui_model_trigger_voltage_prev(state);
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_prev(state);
            }
            else if (state->page == UI_PAGE_MENU)
            {
                ui_model_menu_prev(state);
            }
            redraw = 1U;
        }

        if (btn3_step != 0U)
        {
            if (state->page == UI_PAGE_PDO)
            {
                ui_model_pdo_scroll_next(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL)
            {
                ui_model_protocol_scroll_next(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL_WARNING)
            {
                ui_model_protocol_warning_toggle(state);
            }
            else if (state->page == UI_PAGE_RIPPLE)
            {
                ui_model_ripple_frequency_next(state);
            }
            else if (state->page == UI_PAGE_TRIGGER_SELECT)
            {
                app_ui_navigation_trigger_select_next(state);
            }
            else if (state->page == UI_PAGE_TRIGGER_ADJUST)
            {
                ui_model_trigger_voltage_next(state);
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_next(state);
            }
            else if (state->page == UI_PAGE_MENU)
            {
                ui_model_menu_next(state);
            }
            redraw = 1U;
        }

        if (keys->btn2_short != 0U)
        {
            if (state->page == UI_PAGE_PDO)
            {
                redraw = 1U;
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_activate(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL_WARNING)
            {
                ui_model_protocol_warning_accept(state);
            }
            else if (state->page == UI_PAGE_PROTOCOL)
            {
                redraw = 1U;
            }
            else if ((state->page == UI_PAGE_TRIGGER_SELECT) ||
                     (state->page == UI_PAGE_TRIGGER_ADJUST))
            {
                app_ui_navigation_trigger_confirm(state, protocol);
            }
            else if (state->page == UI_PAGE_MENU)
            {
                app_ui_navigation_activate_menu(state);
            }
            else if (state->page == UI_PAGE_RIPPLE)
            {
                ui_model_exit_action_mode(state);
            }
            else
            {
                ui_model_exit_action_mode(state);
            }
            redraw = 1U;
        }
    }
    else
    {
        if (keys->btn3_long != 0U)
        {
            app_ui_navigation_open_trigger(state);
            redraw = 1U;
            return redraw;
        }

        if (btn1_step != 0U)
        {
            ui_model_prev_page(state);
            redraw = 1U;
        }

        if (btn3_step != 0U)
        {
            ui_model_next_page(state);
            redraw = 1U;
        }

        if (keys->btn2_short != 0U)
        {
            if (state->page == UI_PAGE_POWER_STATS)
            {
                ui_model_power_stats_toggle(state);
                redraw = 1U;
            }
            else if (state->page == UI_PAGE_CAPACITY)
            {
                ui_model_capacity_toggle(state);
                redraw = 1U;
            }
            else if (state->page == UI_PAGE_RIPPLE)
            {
                ui_model_ripple_toggle_pause(state);
                redraw = 1U;
            }
            else if (state->page == UI_PAGE_MENU)
            {
                app_ui_navigation_activate_menu(state);
                redraw = 1U;
            }
            else
            {
                redraw = ui_model_enter_action_mode(state);
            }
            if (redraw != 0U)
            {
                app_ui_navigation_sync_pdo_target(state, protocol);
            }
        }
    }

    return redraw;
}

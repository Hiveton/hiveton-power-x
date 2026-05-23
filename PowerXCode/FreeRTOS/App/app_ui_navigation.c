#include "app_ui_navigation.h"

#include "app_trigger_control.h"

static void app_ui_navigation_apply_auto_step(ui_model_state_t *state,
                                              const protocol_snapshot_t *protocol)
{
    if ((state == 0) || (ui_model_trigger_manual(state) != 0U))
    {
        return;
    }

    if (state->page == UI_PAGE_TRIGGER)
    {
        app_trigger_control_apply_auto(state, protocol);
    }
    else if (state->page == UI_PAGE_PDO)
    {
        app_trigger_control_apply_pd(state, protocol);
    }
    else if (state->page == UI_PAGE_QC)
    {
        app_trigger_control_apply_qc(state, protocol);
    }
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
        target_mv = ui_model_trigger_selected_mv(state);
    }

    ui_model_pdo_target_set_mv(state, target_mv);
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
    btn3_step = (uint8_t)((keys->btn3_short != 0U) || (keys->btn3_long != 0U));
    redraw = 0U;
    if (keys->btn2_long != 0U)
    {
        if ((state->page == UI_PAGE_SETTINGS) && (ui_model_action_mode(state) != 0U))
        {
            ui_model_exit_action_mode(state);
        }
        else
        {
            state->page = UI_PAGE_SETTINGS;
            (void)ui_model_enter_action_mode(state);
        }
        redraw = 1U;
        return redraw;
    }

    if (ui_model_action_mode(state) != 0U)
    {
        if (btn1_step != 0U)
        {
            if ((state->page == UI_PAGE_TRIGGER) || (state->page == UI_PAGE_PDO))
            {
                if (state->page == UI_PAGE_PDO)
                {
                    ui_model_pdo_target_prev(state);
                }
                else
                {
                    ui_model_trigger_prev(state);
                }
            }
            else if (state->page == UI_PAGE_QC)
            {
                ui_model_qc_prev(state);
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_prev(state);
            }
            app_ui_navigation_apply_auto_step(state, protocol);
            redraw = 1U;
        }

        if (btn3_step != 0U)
        {
            if ((state->page == UI_PAGE_TRIGGER) || (state->page == UI_PAGE_PDO))
            {
                if (state->page == UI_PAGE_PDO)
                {
                    ui_model_pdo_target_next(state);
                }
                else
                {
                    ui_model_trigger_next(state);
                }
            }
            else if (state->page == UI_PAGE_QC)
            {
                ui_model_qc_next(state);
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_next(state);
            }
            app_ui_navigation_apply_auto_step(state, protocol);
            redraw = 1U;
        }

        if (keys->btn2_short != 0U)
        {
            if (state->page == UI_PAGE_TRIGGER)
            {
                app_trigger_control_apply_auto(state, protocol);
                ui_model_exit_action_mode(state);
            }
            else if (state->page == UI_PAGE_PDO)
            {
                app_trigger_control_apply_pd(state, protocol);
                ui_model_exit_action_mode(state);
            }
            else if (state->page == UI_PAGE_QC)
            {
                app_trigger_control_apply_qc(state, protocol);
                ui_model_exit_action_mode(state);
            }
            else if (state->page == UI_PAGE_SETTINGS)
            {
                ui_model_settings_activate(state);
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
            redraw = ui_model_enter_action_mode(state);
            if (redraw != 0U)
            {
                app_ui_navigation_sync_pdo_target(state, protocol);
            }
        }
    }

    return redraw;
}

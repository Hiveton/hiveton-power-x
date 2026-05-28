#include "ui_model.h"

#define UI_MODEL_MENU_ITEM_COUNT 6U
#define UI_MODEL_MENU_VISIBLE_ROWS 5U
#define UI_MODEL_SETTINGS_ITEM_COUNT 2U
#define UI_MODEL_PROTOCOL_SCROLL_COUNT 10U
#define UI_MODEL_PDO_SCROLL_COUNT 7U
#define UI_MODEL_BRIGHTNESS_COUNT 4U
#define UI_MODEL_RIPPLE_FREQ_COUNT 5U
#define UI_MODEL_PDO_TARGET_DEFAULT_MV 5000U
#define UI_MODEL_PDO_TARGET_MIN_MV 3300U
#define UI_MODEL_PDO_TARGET_MAX_MV 21000U
#define UI_MODEL_PDO_TARGET_STEP_MV 20U
#define UI_MODEL_TRIGGER_VISIBLE_ROWS 5U
#define UI_MODEL_TRIGGER_TARGET_DEFAULT_MV 5000U
#define UI_MODEL_TRIGGER_TARGET_MIN_MV 3300U
#define UI_MODEL_TRIGGER_TARGET_MAX_MV 48000U
#define UI_MODEL_TRIGGER_TARGET_STEP_MV 20U
#define UI_MODEL_LIVENESS_FRAME_COUNT 48U

static const uint8_t g_brightness_percent[UI_MODEL_BRIGHTNESS_COUNT] =
{
    25U,
    50U,
    75U,
    100U
};

static const uint32_t g_ripple_frequency_hz[UI_MODEL_RIPPLE_FREQ_COUNT] =
{
    1000U,
    10000U,
    50000U,
    100000U,
    500000U
};

static ui_page_t ui_model_normalize_page(ui_page_t page)
{
    if ((page < UI_PAGE_MAIN) || (page > UI_PAGE_SETTINGS))
    {
        return UI_PAGE_MAIN;
    }

    return page;
}

static ui_page_t ui_model_next_page_value(ui_page_t page)
{
    switch (ui_model_normalize_page(page))
    {
        case UI_PAGE_MAIN:
            return UI_PAGE_DPDM;
        case UI_PAGE_DPDM:
            return UI_PAGE_POWER_STATS;
        case UI_PAGE_POWER_STATS:
            return UI_PAGE_CAPACITY;
        case UI_PAGE_CAPACITY:
            return UI_PAGE_SCOPE;
        case UI_PAGE_SCOPE:
            return UI_PAGE_RIPPLE;
        case UI_PAGE_RIPPLE:
            return UI_PAGE_MAIN;
        case UI_PAGE_PDO:
        case UI_PAGE_TRIGGER_SELECT:
        case UI_PAGE_TRIGGER_ADJUST:
        case UI_PAGE_EMARK:
        case UI_PAGE_MENU:
        case UI_PAGE_PROTOCOL_WARNING:
        case UI_PAGE_PROTOCOL:
        case UI_PAGE_SETTINGS:
        default:
            return UI_PAGE_MAIN;
    }
}

static ui_page_t ui_model_prev_page_value(ui_page_t page)
{
    switch (ui_model_normalize_page(page))
    {
        case UI_PAGE_MAIN:
            return UI_PAGE_RIPPLE;
        case UI_PAGE_DPDM:
            return UI_PAGE_MAIN;
        case UI_PAGE_POWER_STATS:
            return UI_PAGE_DPDM;
        case UI_PAGE_CAPACITY:
            return UI_PAGE_POWER_STATS;
        case UI_PAGE_SCOPE:
            return UI_PAGE_CAPACITY;
        case UI_PAGE_RIPPLE:
            return UI_PAGE_SCOPE;
        case UI_PAGE_PDO:
        case UI_PAGE_TRIGGER_SELECT:
        case UI_PAGE_TRIGGER_ADJUST:
        case UI_PAGE_EMARK:
        case UI_PAGE_MENU:
        case UI_PAGE_PROTOCOL_WARNING:
        case UI_PAGE_PROTOCOL:
        case UI_PAGE_SETTINGS:
        default:
            return UI_PAGE_RIPPLE;
    }
}

void ui_model_init(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = UI_PAGE_MAIN;
    state->menu_selected_index = 0U;
    state->menu_return_page = UI_PAGE_MAIN;
    state->settings_selected_index = 0U;
    state->brightness_index = (unsigned char)(UI_MODEL_BRIGHTNESS_COUNT - 1U);
    state->rotation_index = 0U;
    state->power_stats_average = 0U;
    state->capacity_show_wh = 0U;
    state->protocol_warning_confirm = 1U;
    state->protocol_scroll = 0U;
    state->pdo_scroll = 0U;
    state->trigger_selected_index = 0U;
    state->trigger_scroll = 0U;
    state->ripple_paused = 0U;
    state->ripple_frequency_index = 3U;
    state->action_mode = 0U;
    state->liveness_frame = 0U;
    state->pdo_target_mv = UI_MODEL_PDO_TARGET_DEFAULT_MV;
    state->trigger_target_mv = UI_MODEL_TRIGGER_TARGET_DEFAULT_MV;
    state->trigger_min_mv = UI_MODEL_TRIGGER_TARGET_MIN_MV;
    state->trigger_max_mv = UI_MODEL_TRIGGER_TARGET_MAX_MV;
    state->trigger_step_mv = UI_MODEL_TRIGGER_TARGET_STEP_MV;
}

void ui_model_open_menu(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if ((state->page != UI_PAGE_MENU) &&
        (state->page != UI_PAGE_SETTINGS) &&
        (state->page != UI_PAGE_PDO) &&
        (state->page != UI_PAGE_TRIGGER_SELECT) &&
        (state->page != UI_PAGE_TRIGGER_ADJUST) &&
        (state->page != UI_PAGE_PROTOCOL_WARNING))
    {
        state->menu_return_page = ui_model_normalize_page(state->page);
    }
    state->page = UI_PAGE_MENU;
    state->action_mode = 1U;
}

void ui_model_menu_back(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->page == UI_PAGE_TRIGGER_ADJUST)
    {
        state->page = UI_PAGE_TRIGGER_SELECT;
        state->action_mode = 1U;
        return;
    }

    if ((state->page == UI_PAGE_SETTINGS) ||
        (state->page == UI_PAGE_PDO) ||
        (state->page == UI_PAGE_TRIGGER_SELECT) ||
        (state->page == UI_PAGE_EMARK) ||
        (state->page == UI_PAGE_PROTOCOL_WARNING) ||
        (state->page == UI_PAGE_PROTOCOL))
    {
        state->page = UI_PAGE_MENU;
        state->action_mode = 1U;
        return;
    }

    state->page = ui_model_normalize_page(state->menu_return_page);
    if ((state->page == UI_PAGE_MENU) ||
        (state->page == UI_PAGE_SETTINGS) ||
        (state->page == UI_PAGE_TRIGGER_SELECT) ||
        (state->page == UI_PAGE_TRIGGER_ADJUST) ||
        (state->page == UI_PAGE_PROTOCOL_WARNING))
    {
        state->page = UI_PAGE_MAIN;
    }
    state->action_mode = 0U;
}

void ui_model_next_page(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = ui_model_next_page_value(state->page);
    state->action_mode = 0U;
}

void ui_model_prev_page(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = ui_model_prev_page_value(state->page);
    state->action_mode = 0U;
}

int ui_model_pdo_target_mv(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return (int)UI_MODEL_PDO_TARGET_DEFAULT_MV;
    }

    if ((state->pdo_target_mv < UI_MODEL_PDO_TARGET_MIN_MV) ||
        (state->pdo_target_mv > UI_MODEL_PDO_TARGET_MAX_MV))
    {
        return (int)UI_MODEL_PDO_TARGET_DEFAULT_MV;
    }

    return (int)state->pdo_target_mv;
}

void ui_model_pdo_target_set_mv(ui_model_state_t *state, int target_mv)
{
    uint16_t normalized_mv;

    if (state == 0)
    {
        return;
    }

    if (target_mv < (int)UI_MODEL_PDO_TARGET_MIN_MV)
    {
        target_mv = (int)UI_MODEL_PDO_TARGET_MIN_MV;
    }
    else if (target_mv > (int)UI_MODEL_PDO_TARGET_MAX_MV)
    {
        target_mv = (int)UI_MODEL_PDO_TARGET_MAX_MV;
    }

    normalized_mv = (uint16_t)(((uint32_t)target_mv + (UI_MODEL_PDO_TARGET_STEP_MV / 2U)) /
                               UI_MODEL_PDO_TARGET_STEP_MV);
    normalized_mv = (uint16_t)(normalized_mv * UI_MODEL_PDO_TARGET_STEP_MV);
    state->pdo_target_mv = normalized_mv;
}

void ui_model_pdo_target_next(ui_model_state_t *state)
{
    uint16_t target_mv;

    if (state == 0)
    {
        return;
    }

    target_mv = (uint16_t)ui_model_pdo_target_mv(state);
    if (target_mv <= (uint16_t)(UI_MODEL_PDO_TARGET_MAX_MV - UI_MODEL_PDO_TARGET_STEP_MV))
    {
        target_mv = (uint16_t)(target_mv + UI_MODEL_PDO_TARGET_STEP_MV);
    }
    state->pdo_target_mv = target_mv;
}

void ui_model_pdo_target_prev(ui_model_state_t *state)
{
    uint16_t target_mv;

    if (state == 0)
    {
        return;
    }

    target_mv = (uint16_t)ui_model_pdo_target_mv(state);
    if (target_mv >= (uint16_t)(UI_MODEL_PDO_TARGET_MIN_MV + UI_MODEL_PDO_TARGET_STEP_MV))
    {
        target_mv = (uint16_t)(target_mv - UI_MODEL_PDO_TARGET_STEP_MV);
    }
    state->pdo_target_mv = target_mv;
}

unsigned char ui_model_page_is_actionable(ui_page_t page)
{
    switch (ui_model_normalize_page(page))
    {
        case UI_PAGE_PDO:
        case UI_PAGE_TRIGGER_SELECT:
        case UI_PAGE_TRIGGER_ADJUST:
        case UI_PAGE_PROTOCOL_WARNING:
        case UI_PAGE_PROTOCOL:
        case UI_PAGE_RIPPLE:
        case UI_PAGE_MENU:
        case UI_PAGE_SETTINGS:
            return 1U;
        case UI_PAGE_MAIN:
        case UI_PAGE_DPDM:
        case UI_PAGE_POWER_STATS:
        case UI_PAGE_CAPACITY:
        case UI_PAGE_SCOPE:
        case UI_PAGE_EMARK:
        default:
            return 0U;
    }
}

void ui_model_menu_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->menu_selected_index = (unsigned char)((state->menu_selected_index + 1U) % UI_MODEL_MENU_ITEM_COUNT);
}

void ui_model_menu_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->menu_selected_index == 0U)
    {
        state->menu_selected_index = (unsigned char)(UI_MODEL_MENU_ITEM_COUNT - 1U);
    }
    else
    {
        --state->menu_selected_index;
    }
}

unsigned char ui_model_menu_selected_index(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }
    if (state->menu_selected_index >= UI_MODEL_MENU_ITEM_COUNT)
    {
        return 0U;
    }
    return state->menu_selected_index;
}

unsigned char ui_model_menu_scroll(const ui_model_state_t *state)
{
    unsigned char selected;

    selected = ui_model_menu_selected_index(state);
    if (selected >= UI_MODEL_MENU_VISIBLE_ROWS)
    {
        return (unsigned char)(selected - (UI_MODEL_MENU_VISIBLE_ROWS - 1U));
    }
    return 0U;
}

unsigned char ui_model_menu_item_count(void)
{
    return UI_MODEL_MENU_ITEM_COUNT;
}

void ui_model_menu_activate(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (ui_model_menu_selected_index(state) == 0U)
    {
        state->page = UI_PAGE_PDO;
        state->pdo_scroll = 0U;
        state->action_mode = 1U;
    }
    else if (ui_model_menu_selected_index(state) == 1U)
    {
        state->page = UI_PAGE_PROTOCOL_WARNING;
        state->protocol_warning_confirm = 1U;
        state->protocol_scroll = 0U;
        state->action_mode = 1U;
    }
    else if (ui_model_menu_selected_index(state) == 2U)
    {
        state->page = UI_PAGE_EMARK;
        state->action_mode = 1U;
    }
    else if (ui_model_menu_selected_index(state) == 5U)
    {
        state->page = UI_PAGE_SETTINGS;
        state->action_mode = 1U;
    }
}

void ui_model_power_stats_toggle(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->power_stats_average = (state->power_stats_average == 0U) ? 1U : 0U;
}

unsigned char ui_model_power_stats_average(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (state->power_stats_average != 0U) ? 1U : 0U;
}

void ui_model_capacity_toggle(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->capacity_show_wh = (state->capacity_show_wh == 0U) ? 1U : 0U;
}

unsigned char ui_model_capacity_show_wh(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (state->capacity_show_wh != 0U) ? 1U : 0U;
}

void ui_model_protocol_warning_toggle(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->protocol_warning_confirm = (state->protocol_warning_confirm == 0U) ? 1U : 0U;
}

unsigned char ui_model_protocol_warning_confirm_selected(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 1U;
    }

    return (state->protocol_warning_confirm != 0U) ? 1U : 0U;
}

void ui_model_protocol_warning_accept(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (ui_model_protocol_warning_confirm_selected(state) != 0U)
    {
        state->page = UI_PAGE_PROTOCOL;
        state->protocol_scroll = 0U;
    }
    else
    {
        state->page = UI_PAGE_MENU;
    }
    state->action_mode = 1U;
}

void ui_model_protocol_scroll_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->protocol_scroll < (unsigned char)(UI_MODEL_PROTOCOL_SCROLL_COUNT - 1U))
    {
        state->protocol_scroll++;
    }
}

void ui_model_protocol_scroll_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->protocol_scroll > 0U)
    {
        state->protocol_scroll--;
    }
}

unsigned char ui_model_protocol_scroll(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return state->protocol_scroll;
}

void ui_model_pdo_scroll_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->pdo_scroll < (unsigned char)(UI_MODEL_PDO_SCROLL_COUNT - 1U))
    {
        state->pdo_scroll++;
    }
}

void ui_model_pdo_scroll_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->pdo_scroll > 0U)
    {
        state->pdo_scroll--;
    }
}

unsigned char ui_model_pdo_scroll(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return state->pdo_scroll;
}

static uint16_t ui_model_trigger_clamp_mv(int value_mv, uint16_t min_mv, uint16_t max_mv, uint16_t step_mv)
{
    uint32_t normalized_mv;

    if (step_mv == 0U)
    {
        step_mv = UI_MODEL_TRIGGER_TARGET_STEP_MV;
    }
    if (min_mv < UI_MODEL_TRIGGER_TARGET_MIN_MV)
    {
        min_mv = UI_MODEL_TRIGGER_TARGET_MIN_MV;
    }
    if (max_mv > UI_MODEL_TRIGGER_TARGET_MAX_MV)
    {
        max_mv = UI_MODEL_TRIGGER_TARGET_MAX_MV;
    }
    if (max_mv < min_mv)
    {
        max_mv = min_mv;
    }
    if (value_mv < (int)min_mv)
    {
        value_mv = (int)min_mv;
    }
    else if (value_mv > (int)max_mv)
    {
        value_mv = (int)max_mv;
    }

    normalized_mv = ((uint32_t)value_mv + (step_mv / 2U)) / step_mv;
    normalized_mv *= step_mv;
    if (normalized_mv < min_mv)
    {
        normalized_mv = min_mv;
    }
    if (normalized_mv > max_mv)
    {
        normalized_mv = max_mv;
    }
    return (uint16_t)normalized_mv;
}

void ui_model_open_trigger(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if ((state->page != UI_PAGE_TRIGGER_SELECT) &&
        (state->page != UI_PAGE_TRIGGER_ADJUST))
    {
        state->menu_return_page = ui_model_normalize_page(state->page);
    }
    state->page = UI_PAGE_TRIGGER_SELECT;
    state->trigger_selected_index = 0U;
    state->trigger_scroll = 0U;
    state->action_mode = 1U;
}

static void ui_model_trigger_update_scroll(ui_model_state_t *state, unsigned char count)
{
    unsigned char selected;
    unsigned char max_scroll;

    if ((state == 0) || (count == 0U))
    {
        return;
    }

    selected = state->trigger_selected_index;
    if (selected >= count)
    {
        selected = (unsigned char)(count - 1U);
        state->trigger_selected_index = selected;
    }
    max_scroll = (count > UI_MODEL_TRIGGER_VISIBLE_ROWS) ?
                 (unsigned char)(count - UI_MODEL_TRIGGER_VISIBLE_ROWS) : 0U;
    if (state->trigger_scroll > max_scroll)
    {
        state->trigger_scroll = max_scroll;
    }
    if (selected < state->trigger_scroll)
    {
        state->trigger_scroll = selected;
    }
    else if (selected >= (unsigned char)(state->trigger_scroll + UI_MODEL_TRIGGER_VISIBLE_ROWS))
    {
        state->trigger_scroll = (unsigned char)(selected - (UI_MODEL_TRIGGER_VISIBLE_ROWS - 1U));
    }
}

void ui_model_trigger_select_next(ui_model_state_t *state, unsigned char count)
{
    if ((state == 0) || (count == 0U))
    {
        return;
    }

    state->trigger_selected_index = (unsigned char)((state->trigger_selected_index + 1U) % count);
    ui_model_trigger_update_scroll(state, count);
}

void ui_model_trigger_select_prev(ui_model_state_t *state, unsigned char count)
{
    if ((state == 0) || (count == 0U))
    {
        return;
    }

    if (state->trigger_selected_index == 0U)
    {
        state->trigger_selected_index = (unsigned char)(count - 1U);
    }
    else
    {
        state->trigger_selected_index--;
    }
    ui_model_trigger_update_scroll(state, count);
}

unsigned char ui_model_trigger_selected_index(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return state->trigger_selected_index;
}

unsigned char ui_model_trigger_scroll(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return state->trigger_scroll;
}

void ui_model_trigger_enter_adjust(ui_model_state_t *state,
                                   int min_mv,
                                   int max_mv,
                                   int step_mv,
                                   int default_mv)
{
    if (state == 0)
    {
        return;
    }

    if (step_mv <= 0)
    {
        step_mv = (int)UI_MODEL_TRIGGER_TARGET_STEP_MV;
    }
    if (min_mv <= 0)
    {
        min_mv = (int)UI_MODEL_TRIGGER_TARGET_MIN_MV;
    }
    if (max_mv < min_mv)
    {
        max_mv = min_mv;
    }
    state->trigger_min_mv = (uint16_t)min_mv;
    state->trigger_max_mv = (uint16_t)max_mv;
    state->trigger_step_mv = (uint16_t)step_mv;
    state->trigger_target_mv = ui_model_trigger_clamp_mv(default_mv,
                                                         state->trigger_min_mv,
                                                         state->trigger_max_mv,
                                                         state->trigger_step_mv);
    state->page = UI_PAGE_TRIGGER_ADJUST;
    state->action_mode = 1U;
}

void ui_model_trigger_back_to_select(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = UI_PAGE_TRIGGER_SELECT;
    state->action_mode = 1U;
}

int ui_model_trigger_target_mv(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return (int)UI_MODEL_TRIGGER_TARGET_DEFAULT_MV;
    }

    return (int)state->trigger_target_mv;
}

int ui_model_trigger_min_mv(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return (int)UI_MODEL_TRIGGER_TARGET_MIN_MV;
    }

    return (int)state->trigger_min_mv;
}

int ui_model_trigger_max_mv(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return (int)UI_MODEL_TRIGGER_TARGET_MAX_MV;
    }

    return (int)state->trigger_max_mv;
}

int ui_model_trigger_step_mv(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return (int)UI_MODEL_TRIGGER_TARGET_STEP_MV;
    }

    return (int)state->trigger_step_mv;
}

void ui_model_trigger_voltage_next(ui_model_state_t *state)
{
    uint16_t target_mv;

    if (state == 0)
    {
        return;
    }

    target_mv = state->trigger_target_mv;
    if (target_mv <= (uint16_t)(state->trigger_max_mv - state->trigger_step_mv))
    {
        target_mv = (uint16_t)(target_mv + state->trigger_step_mv);
    }
    state->trigger_target_mv = ui_model_trigger_clamp_mv((int)target_mv,
                                                         state->trigger_min_mv,
                                                         state->trigger_max_mv,
                                                         state->trigger_step_mv);
}

void ui_model_trigger_voltage_prev(ui_model_state_t *state)
{
    uint16_t target_mv;

    if (state == 0)
    {
        return;
    }

    target_mv = state->trigger_target_mv;
    if (target_mv >= (uint16_t)(state->trigger_min_mv + state->trigger_step_mv))
    {
        target_mv = (uint16_t)(target_mv - state->trigger_step_mv);
    }
    state->trigger_target_mv = ui_model_trigger_clamp_mv((int)target_mv,
                                                         state->trigger_min_mv,
                                                         state->trigger_max_mv,
                                                         state->trigger_step_mv);
}

void ui_model_ripple_toggle_pause(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->ripple_paused = (state->ripple_paused == 0U) ? 1U : 0U;
}

unsigned char ui_model_ripple_paused(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (state->ripple_paused != 0U) ? 1U : 0U;
}

void ui_model_ripple_frequency_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->ripple_frequency_index = (unsigned char)((state->ripple_frequency_index + 1U) % UI_MODEL_RIPPLE_FREQ_COUNT);
}

void ui_model_ripple_frequency_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->ripple_frequency_index == 0U)
    {
        state->ripple_frequency_index = (unsigned char)(UI_MODEL_RIPPLE_FREQ_COUNT - 1U);
    }
    else
    {
        --state->ripple_frequency_index;
    }
}

uint32_t ui_model_ripple_frequency_hz(const ui_model_state_t *state)
{
    unsigned char index;

    if (state == 0)
    {
        return g_ripple_frequency_hz[3U];
    }

    index = state->ripple_frequency_index;
    if (index >= UI_MODEL_RIPPLE_FREQ_COUNT)
    {
        index = 3U;
    }
    return g_ripple_frequency_hz[index];
}

unsigned char ui_model_enter_action_mode(ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    if (ui_model_page_is_actionable(state->page) == 0U)
    {
        state->action_mode = 0U;
        return 0U;
    }

    state->action_mode = 1U;
    return 1U;
}

void ui_model_exit_action_mode(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->action_mode = 0U;
}

unsigned char ui_model_action_mode(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (state->action_mode != 0U) ? 1U : 0U;
}

void ui_model_settings_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->settings_selected_index = (unsigned char)((state->settings_selected_index + 1U) % UI_MODEL_SETTINGS_ITEM_COUNT);
}

void ui_model_settings_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->settings_selected_index == 0U)
    {
        state->settings_selected_index = (unsigned char)(UI_MODEL_SETTINGS_ITEM_COUNT - 1U);
    }
    else
    {
        state->settings_selected_index--;
    }
}

unsigned char ui_model_settings_selected_index(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    if (state->settings_selected_index >= UI_MODEL_SETTINGS_ITEM_COUNT)
    {
        return 0U;
    }

    return state->settings_selected_index;
}

void ui_model_settings_activate(ui_model_state_t *state)
{
    unsigned char selected;

    if (state == 0)
    {
        return;
    }

    selected = ui_model_settings_selected_index(state);
    switch (selected)
    {
        case 0U:
            state->brightness_index = (unsigned char)((state->brightness_index + 1U) % UI_MODEL_BRIGHTNESS_COUNT);
            break;
        case 1U:
            state->rotation_index = (state->rotation_index == 0U) ? 1U : 0U;
            break;
        default:
            break;
    }
}

uint8_t ui_model_brightness_percent(const ui_model_state_t *state)
{
    unsigned char index;

    if (state == 0)
    {
        return 100U;
    }

    index = state->brightness_index;
    if (index >= UI_MODEL_BRIGHTNESS_COUNT)
    {
        index = (unsigned char)(UI_MODEL_BRIGHTNESS_COUNT - 1U);
    }

    return g_brightness_percent[index];
}

uint16_t ui_model_rotation_degrees(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (state->rotation_index == 0U) ? 0U : 180U;
}

void ui_model_advance_liveness(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->liveness_frame = (unsigned char)((state->liveness_frame + 1U) % UI_MODEL_LIVENESS_FRAME_COUNT);
}

unsigned char ui_model_liveness_frame(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    return (unsigned char)(state->liveness_frame % UI_MODEL_LIVENESS_FRAME_COUNT);
}

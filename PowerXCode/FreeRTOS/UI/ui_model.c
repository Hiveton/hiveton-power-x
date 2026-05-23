#include "ui_model.h"

#define UI_MODEL_TRIGGER_PRESET_COUNT 5U
#define UI_MODEL_QC_PRESET_COUNT 5U
#define UI_MODEL_SETTINGS_ITEM_COUNT 3U
#define UI_MODEL_BRIGHTNESS_COUNT 4U
#define UI_MODEL_PDO_TARGET_DEFAULT_MV 5000U
#define UI_MODEL_PDO_TARGET_MIN_MV 3300U
#define UI_MODEL_PDO_TARGET_MAX_MV 21000U
#define UI_MODEL_PDO_TARGET_STEP_MV 20U
#define UI_MODEL_LIVENESS_FRAME_COUNT 48U

static const int g_trigger_presets_mv[UI_MODEL_TRIGGER_PRESET_COUNT] =
{
    5000,
    9000,
    12000,
    15000,
    20000
};

static const uint8_t g_brightness_percent[UI_MODEL_BRIGHTNESS_COUNT] =
{
    25U,
    50U,
    75U,
    100U
};

static const int g_qc_presets_mv[UI_MODEL_QC_PRESET_COUNT] =
{
    5000,
    9000,
    12000,
    15000,
    20000
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
            return UI_PAGE_SCOPE;
        case UI_PAGE_SCOPE:
            return UI_PAGE_PROTOCOL;
        case UI_PAGE_PROTOCOL:
            return UI_PAGE_TRIGGER;
        case UI_PAGE_TRIGGER:
            return UI_PAGE_PDO;
        case UI_PAGE_PDO:
            return UI_PAGE_QC;
        case UI_PAGE_QC:
            return UI_PAGE_CC;
        case UI_PAGE_CC:
            return UI_PAGE_CABLE;
        case UI_PAGE_CABLE:
            return UI_PAGE_SETTINGS;
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
            return UI_PAGE_SETTINGS;
        case UI_PAGE_SCOPE:
            return UI_PAGE_MAIN;
        case UI_PAGE_PROTOCOL:
            return UI_PAGE_SCOPE;
        case UI_PAGE_TRIGGER:
            return UI_PAGE_PROTOCOL;
        case UI_PAGE_PDO:
            return UI_PAGE_TRIGGER;
        case UI_PAGE_QC:
            return UI_PAGE_PDO;
        case UI_PAGE_CABLE:
            return UI_PAGE_CC;
        case UI_PAGE_CC:
            return UI_PAGE_QC;
        case UI_PAGE_SETTINGS:
        default:
            return UI_PAGE_CABLE;
    }
}

void ui_model_init(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = UI_PAGE_MAIN;
    state->trigger_preset_index = 0U;
    state->qc_preset_index = 0U;
    state->settings_selected_index = 0U;
    state->brightness_index = (unsigned char)(UI_MODEL_BRIGHTNESS_COUNT - 1U);
    state->rotation_index = 0U;
    state->trigger_manual = 1U;
    state->action_mode = 0U;
    state->liveness_frame = 0U;
    state->pdo_target_mv = UI_MODEL_PDO_TARGET_DEFAULT_MV;
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

void ui_model_trigger_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->trigger_preset_index = (unsigned char)((state->trigger_preset_index + 1U) % UI_MODEL_TRIGGER_PRESET_COUNT);
}

void ui_model_trigger_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->trigger_preset_index == 0U)
    {
        state->trigger_preset_index = (unsigned char)(UI_MODEL_TRIGGER_PRESET_COUNT - 1U);
    }
    else
    {
        state->trigger_preset_index--;
    }
}

int ui_model_trigger_selected_mv(const ui_model_state_t *state)
{
    unsigned char index;

    if (state == 0)
    {
        return g_trigger_presets_mv[0];
    }

    index = state->trigger_preset_index;
    if (index >= UI_MODEL_TRIGGER_PRESET_COUNT)
    {
        index = 0U;
    }

    return g_trigger_presets_mv[index];
}

unsigned char ui_model_trigger_selected_index(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    if (state->trigger_preset_index >= UI_MODEL_TRIGGER_PRESET_COUNT)
    {
        return 0U;
    }

    return state->trigger_preset_index;
}

unsigned char ui_model_trigger_preset_count(void)
{
    return UI_MODEL_TRIGGER_PRESET_COUNT;
}

int ui_model_trigger_preset_mv_at(unsigned char index)
{
    if (index >= UI_MODEL_TRIGGER_PRESET_COUNT)
    {
        index = 0U;
    }

    return g_trigger_presets_mv[index];
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

void ui_model_qc_next(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->qc_preset_index = (unsigned char)((state->qc_preset_index + 1U) % UI_MODEL_QC_PRESET_COUNT);
}

void ui_model_qc_prev(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    if (state->qc_preset_index == 0U)
    {
        state->qc_preset_index = (unsigned char)(UI_MODEL_QC_PRESET_COUNT - 1U);
    }
    else
    {
        state->qc_preset_index--;
    }
}

int ui_model_qc_selected_mv(const ui_model_state_t *state)
{
    unsigned char index;

    if (state == 0)
    {
        return g_qc_presets_mv[0];
    }

    index = state->qc_preset_index;
    if (index >= UI_MODEL_QC_PRESET_COUNT)
    {
        index = 0U;
    }

    return g_qc_presets_mv[index];
}

unsigned char ui_model_qc_selected_index(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 0U;
    }

    if (state->qc_preset_index >= UI_MODEL_QC_PRESET_COUNT)
    {
        return 0U;
    }

    return state->qc_preset_index;
}

unsigned char ui_model_page_is_actionable(ui_page_t page)
{
    switch (ui_model_normalize_page(page))
    {
        case UI_PAGE_TRIGGER:
        case UI_PAGE_PDO:
        case UI_PAGE_QC:
        case UI_PAGE_SETTINGS:
            return 1U;
        case UI_PAGE_MAIN:
        case UI_PAGE_SCOPE:
        case UI_PAGE_PROTOCOL:
        case UI_PAGE_CC:
        case UI_PAGE_CABLE:
        default:
            return 0U;
    }
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
        case 2U:
            state->trigger_manual = (state->trigger_manual == 0U) ? 1U : 0U;
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

unsigned char ui_model_trigger_manual(const ui_model_state_t *state)
{
    if (state == 0)
    {
        return 1U;
    }

    return (state->trigger_manual != 0U) ? 1U : 0U;
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

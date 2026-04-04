#include "ui_model.h"

#define UI_MODEL_TRIGGER_PRESET_COUNT 5U

static const int g_trigger_presets_mv[UI_MODEL_TRIGGER_PRESET_COUNT] =
{
    5000,
    9000,
    12000,
    15000,
    20000
};

static ui_page_t ui_model_normalize_page(ui_page_t page)
{
    if ((page < UI_PAGE_MAIN) || (page > UI_PAGE_STATS))
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
            return UI_PAGE_PROTOCOL;
        case UI_PAGE_PROTOCOL:
            return UI_PAGE_TRIGGER;
        case UI_PAGE_TRIGGER:
            return UI_PAGE_STATS;
        case UI_PAGE_STATS:
        default:
            return UI_PAGE_MAIN;
    }
}

static ui_page_t ui_model_prev_page_value(ui_page_t page)
{
    switch (ui_model_normalize_page(page))
    {
        case UI_PAGE_MAIN:
            return UI_PAGE_STATS;
        case UI_PAGE_PROTOCOL:
            return UI_PAGE_MAIN;
        case UI_PAGE_TRIGGER:
            return UI_PAGE_PROTOCOL;
        case UI_PAGE_STATS:
        default:
            return UI_PAGE_TRIGGER;
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
}

void ui_model_next_page(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = ui_model_next_page_value(state->page);
}

void ui_model_prev_page(ui_model_state_t *state)
{
    if (state == 0)
    {
        return;
    }

    state->page = ui_model_prev_page_value(state->page);
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

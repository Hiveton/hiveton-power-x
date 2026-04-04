#include <assert.h>

#include "ui_model.h"

static void test_page_navigation(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    assert(state.page == UI_PAGE_MAIN);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_forward_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_TRIGGER);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_STATS);

    ui_model_next_page(&state);
    assert(state.page == UI_PAGE_MAIN);
}

static void test_reverse_wraparound(void)
{
    ui_model_state_t state;

    ui_model_init(&state);
    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_STATS);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_TRIGGER);

    ui_model_prev_page(&state);
    assert(state.page == UI_PAGE_PROTOCOL);

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

int main(void)
{
    test_page_navigation();
    test_forward_wraparound();
    test_reverse_wraparound();
    test_trigger_preset_navigation();
    test_trigger_preset_reverse_wraparound();
    return 0;
}

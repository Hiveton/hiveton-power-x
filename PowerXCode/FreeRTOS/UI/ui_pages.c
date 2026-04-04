#include "ui_renderer.h"
#include "ui_pages.h"

void ui_pages_draw(const ui_model_state_t *state,
                   const measure_snapshot_t *measure,
                   const protocol_snapshot_t *protocol)
{
    if (state == 0)
    {
        return;
    }

    switch (state->page)
    {
        case UI_PAGE_MAIN:
            ui_renderer_draw_main_page(measure, protocol);
            break;
        case UI_PAGE_PROTOCOL:
            ui_renderer_draw_protocol_page(protocol);
            break;
        case UI_PAGE_TRIGGER:
            ui_renderer_draw_trigger_page(state, protocol);
            break;
        case UI_PAGE_STATS:
            ui_renderer_draw_placeholder_page(state->page);
            break;
        default:
            ui_renderer_draw_placeholder_page(UI_PAGE_MAIN);
            break;
    }
}

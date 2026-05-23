#include "ui_renderer.h"
#include "ui_pages.h"

#define UI_PAGES_VBUS_PRESENT_MV 3000

static uint8_t ui_pages_protocol_cc_attached(const protocol_snapshot_t *protocol)
{
    if (protocol == 0)
    {
        return 0U;
    }

    return ((protocol->cc_attached != 0U) || (protocol->cc_orientation != 0U)) ? 1U : 0U;
}

static protocol_snapshot_t ui_pages_display_protocol(const measure_snapshot_t *measure,
                                                     const protocol_snapshot_t *protocol)
{
    protocol_snapshot_t display_protocol;

    if (protocol != 0)
    {
        display_protocol = *protocol;
    }
    else
    {
        protocol_snapshot_reset(&display_protocol);
    }

    if ((display_protocol.kind == PROTOCOL_KIND_NONE) &&
        (ui_pages_protocol_cc_attached(&display_protocol) == 0U) &&
        (measure != 0) &&
        (measure->voltage_valid != 0U) &&
        (measure->voltage_avg_mv >= UI_PAGES_VBUS_PRESENT_MV))
    {
        protocol_snapshot_set_other(&display_protocol);
    }

    return display_protocol;
}

void ui_pages_draw(const ui_model_state_t *state,
                   const measure_snapshot_t *measure,
                   const protocol_snapshot_t *protocol)
{
    protocol_snapshot_t display_protocol;

    if (state == 0)
    {
        return;
    }

    display_protocol = ui_pages_display_protocol(measure, protocol);

    switch (state->page)
    {
        case UI_PAGE_MAIN:
            ui_renderer_draw_main_page(state, measure, &display_protocol);
            break;
        case UI_PAGE_SCOPE:
            ui_renderer_draw_scope_page(measure);
            break;
        case UI_PAGE_PROTOCOL:
            ui_renderer_draw_protocol_page(measure, &display_protocol);
            break;
        case UI_PAGE_TRIGGER:
            ui_renderer_draw_trigger_page(state, &display_protocol);
            break;
        case UI_PAGE_PDO:
            ui_renderer_draw_pdo_page(state, &display_protocol);
            break;
        case UI_PAGE_QC:
            ui_renderer_draw_qc_page(state, &display_protocol);
            break;
        case UI_PAGE_CC:
            ui_renderer_draw_cc_page(measure, &display_protocol);
            break;
        case UI_PAGE_CABLE:
            ui_renderer_draw_cable_page(measure, &display_protocol);
            break;
        case UI_PAGE_SETTINGS:
            ui_renderer_draw_settings_page(state);
            break;
        default:
            ui_renderer_draw_main_page(state, measure, &display_protocol);
            break;
    }
}

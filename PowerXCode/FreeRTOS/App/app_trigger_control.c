#include "app_trigger_control.h"

#include "service_legacy_charge.h"
#include "service_pd.h"

static uint8_t app_trigger_control_is_qc2_fixed_target(int target_mv)
{
    return ((target_mv == 5000) ||
            (target_mv == 9000) ||
            (target_mv == 12000) ||
            (target_mv == 20000)) ? 1U : 0U;
}

static legacy_protocol_t app_trigger_control_legacy_protocol(protocol_kind_t kind, int target_mv)
{
    switch (kind)
    {
        case PROTOCOL_KIND_QC:
            return (app_trigger_control_is_qc2_fixed_target(target_mv) != 0U) ?
                   LEGACY_PROTOCOL_QC2 :
                   LEGACY_PROTOCOL_QC3;
        case PROTOCOL_KIND_AFC:
            return LEGACY_PROTOCOL_AFC;
        case PROTOCOL_KIND_FCP:
            return LEGACY_PROTOCOL_FCP;
        case PROTOCOL_KIND_OTHER:
        case PROTOCOL_KIND_NONE:
        case PROTOCOL_KIND_PD:
        default:
            return LEGACY_PROTOCOL_NONE;
    }
}

void app_trigger_control_apply_auto(const ui_model_state_t *state,
                                    const protocol_snapshot_t *protocol)
{
    legacy_protocol_t legacy_protocol;
    int selected_mv;

    selected_mv = ui_model_trigger_selected_mv(state);
    legacy_protocol = app_trigger_control_legacy_protocol((protocol != 0) ? protocol->kind : PROTOCOL_KIND_NONE,
                                                         selected_mv);

    if (legacy_protocol != LEGACY_PROTOCOL_NONE)
    {
        service_legacy_charge_request_voltage_mv(legacy_protocol, selected_mv);
        return;
    }

    service_pd_set_preferred_voltage_mv(selected_mv);
}

void app_trigger_control_apply_pd(const ui_model_state_t *state,
                                  const protocol_snapshot_t *protocol)
{
    int selected_mv;

    (void)protocol;
    selected_mv = ((state != 0) && (state->page == UI_PAGE_PDO)) ?
                  ui_model_pdo_target_mv(state) :
                  ui_model_trigger_selected_mv(state);
    service_pd_set_preferred_voltage_mv(selected_mv);
}

void app_trigger_control_apply_qc(const ui_model_state_t *state,
                                  const protocol_snapshot_t *protocol)
{
    int selected_mv;

    selected_mv = ui_model_qc_selected_mv(state);
    if ((protocol != 0) && (protocol->kind == PROTOCOL_KIND_PD))
    {
        service_pd_set_preferred_voltage_mv(selected_mv);
        return;
    }

    service_legacy_charge_request_voltage_mv(app_trigger_control_is_qc2_fixed_target(selected_mv) != 0U ?
                                             LEGACY_PROTOCOL_QC2 :
                                             LEGACY_PROTOCOL_QC3,
                                             selected_mv);
}

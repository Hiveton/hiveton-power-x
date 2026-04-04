#include "app_trigger_control.h"

#include "service_legacy_charge.h"
#include "service_pd.h"

static legacy_protocol_t app_trigger_control_legacy_protocol(protocol_kind_t kind)
{
    switch (kind)
    {
        case PROTOCOL_KIND_QC:
            return LEGACY_PROTOCOL_QC2;
        case PROTOCOL_KIND_AFC:
            return LEGACY_PROTOCOL_AFC;
        case PROTOCOL_KIND_FCP:
            return LEGACY_PROTOCOL_FCP;
        case PROTOCOL_KIND_NONE:
        case PROTOCOL_KIND_PD:
        default:
            return LEGACY_PROTOCOL_NONE;
    }
}

void app_trigger_control_apply(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol)
{
    legacy_protocol_t legacy_protocol;
    int selected_mv;

    selected_mv = ui_model_trigger_selected_mv(state);
    legacy_protocol = app_trigger_control_legacy_protocol((protocol != 0) ? protocol->kind : PROTOCOL_KIND_NONE);

    if (legacy_protocol != LEGACY_PROTOCOL_NONE)
    {
        service_legacy_charge_request_voltage_mv(legacy_protocol, selected_mv);
        return;
    }

    service_pd_set_preferred_voltage_mv(selected_mv);
}

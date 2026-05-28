#include "app_protocol_arbiter.h"

static uint8_t app_protocol_snapshot_cc_attached(const protocol_snapshot_t *snapshot);
static void app_protocol_snapshot_merge_dpdm(protocol_snapshot_t *snapshot,
                                             const protocol_snapshot_t *dpdm_snapshot);
static void app_protocol_snapshot_merge_cc(protocol_snapshot_t *snapshot,
                                           const protocol_snapshot_t *cc_snapshot);
static void app_protocol_snapshot_merge_emark(protocol_snapshot_t *snapshot,
                                              const protocol_snapshot_t *pd_snapshot);

void app_protocol_arbiter_init(app_protocol_arbiter_t *arbiter)
{
    if (arbiter == 0)
    {
        return;
    }

    protocol_snapshot_reset(&arbiter->pd_snapshot);
    protocol_snapshot_reset(&arbiter->legacy_snapshot);
    arbiter->pd_ready = 0U;
    arbiter->legacy_ready = 0U;
    arbiter->pd_was_active = 0U;
}

void app_protocol_arbiter_publish(app_protocol_arbiter_t *arbiter,
                                  app_protocol_source_t source,
                                  const protocol_snapshot_t *snapshot)
{
    if ((arbiter == 0) || (snapshot == 0))
    {
        return;
    }

    if (source == APP_PROTOCOL_SOURCE_PD)
    {
        arbiter->pd_snapshot = *snapshot;
        arbiter->pd_ready = 1U;
        if (snapshot->kind == PROTOCOL_KIND_PD)
        {
            arbiter->pd_was_active = 1U;
        }
        else if (app_protocol_snapshot_cc_attached(snapshot) != 0U)
        {
            arbiter->pd_was_active = 1U;
        }
        else if (arbiter->pd_was_active != 0U)
        {
            protocol_snapshot_reset(&arbiter->legacy_snapshot);
            arbiter->legacy_ready = 1U;
            arbiter->pd_was_active = 0U;
        }
    }
    else
    {
        arbiter->legacy_snapshot = *snapshot;
        arbiter->legacy_ready = 1U;
    }
}

static uint8_t app_protocol_snapshot_cc_attached(const protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return 0U;
    }

    return ((snapshot->cc_attached != 0U) || (snapshot->cc_orientation != 0U)) ? 1U : 0U;
}

static uint8_t app_protocol_snapshot_legacy_is_active_request(const protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return 0U;
    }

    return ((snapshot->request_state == PROTOCOL_REQUEST_REQUESTING) ||
            (snapshot->request_state == PROTOCOL_REQUEST_READY) ||
            (snapshot->request_state == PROTOCOL_REQUEST_FAILED)) ? 1U : 0U;
}

static void app_protocol_snapshot_merge_dpdm(protocol_snapshot_t *snapshot,
                                             const protocol_snapshot_t *dpdm_snapshot)
{
    if ((snapshot == 0) || (dpdm_snapshot == 0))
    {
        return;
    }

    if (dpdm_snapshot->dp_mv != 0)
    {
        snapshot->dp_mv = dpdm_snapshot->dp_mv;
    }
    if (dpdm_snapshot->dm_mv != 0)
    {
        snapshot->dm_mv = dpdm_snapshot->dm_mv;
    }
}

static void app_protocol_snapshot_merge_cc(protocol_snapshot_t *snapshot,
                                           const protocol_snapshot_t *cc_snapshot)
{
    if ((snapshot == 0) || (cc_snapshot == 0) ||
        (app_protocol_snapshot_cc_attached(cc_snapshot) == 0U))
    {
        return;
    }

    snapshot->cc_attached = cc_snapshot->cc_attached;
    snapshot->cc_orientation = cc_snapshot->cc_orientation;
    snapshot->cc_active_mask = cc_snapshot->cc_active_mask;
}

static void app_protocol_snapshot_merge_emark(protocol_snapshot_t *snapshot,
                                              const protocol_snapshot_t *pd_snapshot)
{
    if ((snapshot == 0) || (pd_snapshot == 0) || (pd_snapshot->emark_present == 0U))
    {
        return;
    }

    snapshot->emark_present = 1U;
    snapshot->emark_current_a = pd_snapshot->emark_current_a;
    snapshot->emark_max_voltage_v = pd_snapshot->emark_max_voltage_v;
    snapshot->emark_cable_length_m = pd_snapshot->emark_cable_length_m;
    snapshot->emark_epr_capable = pd_snapshot->emark_epr_capable;
    snapshot->emark_usb_speed_grade = pd_snapshot->emark_usb_speed_grade;
    snapshot->emark_cable_type = pd_snapshot->emark_cable_type;
    snapshot->emark_vdo_version = pd_snapshot->emark_vdo_version;
    snapshot->emark_firmware_version = pd_snapshot->emark_firmware_version;
    snapshot->emark_hardware_version = pd_snapshot->emark_hardware_version;
}

void app_protocol_arbiter_copy(const app_protocol_arbiter_t *arbiter,
                               protocol_snapshot_t *snapshot)
{
    if (snapshot == 0)
    {
        return;
    }

    if (arbiter == 0)
    {
        protocol_snapshot_reset(snapshot);
        return;
    }

    if ((arbiter->pd_ready != 0U) && (arbiter->pd_snapshot.kind == PROTOCOL_KIND_PD))
    {
        *snapshot = arbiter->pd_snapshot;
        if (arbiter->legacy_ready != 0U)
        {
            app_protocol_snapshot_merge_dpdm(snapshot, &arbiter->legacy_snapshot);
        }
        app_protocol_snapshot_merge_emark(snapshot, &arbiter->pd_snapshot);
        return;
    }

    if ((arbiter->pd_ready != 0U) &&
        (app_protocol_snapshot_cc_attached(&arbiter->pd_snapshot) != 0U) &&
        ((arbiter->legacy_ready == 0U) ||
         (arbiter->legacy_snapshot.kind == PROTOCOL_KIND_NONE) ||
         (arbiter->legacy_snapshot.kind == PROTOCOL_KIND_OTHER) ||
         (app_protocol_snapshot_legacy_is_active_request(&arbiter->legacy_snapshot) == 0U)))
    {
        *snapshot = arbiter->pd_snapshot;
        if (arbiter->legacy_ready != 0U)
        {
            app_protocol_snapshot_merge_dpdm(snapshot, &arbiter->legacy_snapshot);
        }
        app_protocol_snapshot_merge_emark(snapshot, &arbiter->pd_snapshot);
        return;
    }

    if ((arbiter->legacy_ready != 0U) && (arbiter->legacy_snapshot.kind != PROTOCOL_KIND_NONE))
    {
        *snapshot = arbiter->legacy_snapshot;
        if (arbiter->pd_ready != 0U)
        {
            app_protocol_snapshot_merge_cc(snapshot, &arbiter->pd_snapshot);
            app_protocol_snapshot_merge_emark(snapshot, &arbiter->pd_snapshot);
        }
        return;
    }

    if (arbiter->pd_ready != 0U)
    {
        *snapshot = arbiter->pd_snapshot;
        return;
    }

    if (arbiter->legacy_ready != 0U)
    {
        *snapshot = arbiter->legacy_snapshot;
        return;
    }

    protocol_snapshot_reset(snapshot);
}

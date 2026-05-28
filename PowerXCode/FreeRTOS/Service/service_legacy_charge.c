#include "service_legacy_charge.h"

#include "bsp_cc_ext_rd.h"
#include "bsp_dpdm.h"
#include "service_charge_protocols.h"
#if defined(__riscv)
#include "bsp_usbpd_port.h"
#endif

#define LEGACY_CHARGE_QC2_DEFAULT_MV 5000
#define LEGACY_CHARGE_QC2_MIN_MV 5000
#define LEGACY_CHARGE_QC2_MAX_MV 20000
#define LEGACY_CHARGE_QC2_9V_MV 9000
#define LEGACY_CHARGE_QC2_12V_MV 12000
#define LEGACY_CHARGE_QC2_20V_MV 20000
#define LEGACY_CHARGE_READY_TOLERANCE_MV 900
#define LEGACY_CHARGE_REQUEST_TIMEOUT_MS 2200U
#define LEGACY_CHARGE_FAILED_HOLD_MS 1500U
#define LEGACY_CHARGE_DPDM_PRESENT_MV 300
#define LEGACY_CHARGE_QC3_MIN_OFFSET -7
#define LEGACY_CHARGE_QC3_MAX_OFFSET 75

typedef struct
{
    uint32_t request_elapsed_ms;
    uint32_t failed_hold_elapsed_ms;
    int8_t qc3_applied_step_offset;
    uint8_t failed_visible;
} legacy_charge_runtime_t;

static legacy_protocol_t g_detected_protocol = LEGACY_PROTOCOL_NONE;
static legacy_protocol_t g_requested_protocol = LEGACY_PROTOCOL_NONE;
static legacy_charge_request_t g_request = { LEGACY_PROTOCOL_NONE, 0, 0, LEGACY_CHARGE_STATUS_IDLE, 0, 0 };
static legacy_charge_runtime_t g_runtime;
static uint8_t g_passive_dpdm_present;

static uint8_t service_legacy_charge_typec_cc_attached(void)
{
#if defined(__riscv)
    return (bsp_usbpd_port_current_cc() != 0U) ? 1U : 0U;
#else
    return 0U;
#endif
}

static bsp_dpdm_mode_t service_legacy_charge_mode_for_protocol(legacy_protocol_t protocol)
{
    switch (protocol)
    {
        case LEGACY_PROTOCOL_QC3:
            return BSP_DPDM_MODE_DP;
        case LEGACY_PROTOCOL_AFC:
            return BSP_DPDM_MODE_DM;
        case LEGACY_PROTOCOL_QC2:
        case LEGACY_PROTOCOL_FCP:
            return BSP_DPDM_MODE_BOTH;
        case LEGACY_PROTOCOL_NONE:
        default:
            return BSP_DPDM_MODE_HIZ;
    }
}

static int32_t service_legacy_charge_clamp_qc2_voltage(int32_t target_mv)
{
    if (target_mv < LEGACY_CHARGE_QC2_MIN_MV)
    {
        return LEGACY_CHARGE_QC2_MIN_MV;
    }

    if (target_mv > LEGACY_CHARGE_QC2_MAX_MV)
    {
        return LEGACY_CHARGE_QC2_MAX_MV;
    }

    return target_mv;
}

static protocol_kind_t service_legacy_charge_protocol_kind(legacy_protocol_t protocol)
{
    switch (protocol)
    {
        case LEGACY_PROTOCOL_QC2:
        case LEGACY_PROTOCOL_QC3:
            return PROTOCOL_KIND_QC;
        case LEGACY_PROTOCOL_AFC:
            return PROTOCOL_KIND_AFC;
        case LEGACY_PROTOCOL_FCP:
            return PROTOCOL_KIND_FCP;
        case LEGACY_PROTOCOL_NONE:
        default:
            return PROTOCOL_KIND_NONE;
    }
}

static protocol_request_state_t service_legacy_charge_request_state(legacy_charge_status_t status)
{
    switch (status)
    {
        case LEGACY_CHARGE_STATUS_AVAILABLE:
            return PROTOCOL_REQUEST_AVAILABLE;
        case LEGACY_CHARGE_STATUS_REQUESTING:
            return PROTOCOL_REQUEST_REQUESTING;
        case LEGACY_CHARGE_STATUS_READY:
            return PROTOCOL_REQUEST_READY;
        case LEGACY_CHARGE_STATUS_FAILED:
            return PROTOCOL_REQUEST_FAILED;
        case LEGACY_CHARGE_STATUS_IDLE:
        default:
            return PROTOCOL_REQUEST_IDLE;
    }
}

static int32_t service_legacy_charge_quantize_qc2_voltage(int32_t target_mv)
{
    return charge_protocol_qc2_quantize_voltage_mv(service_legacy_charge_clamp_qc2_voltage(target_mv));
}

static int32_t service_legacy_charge_clamp_qc3_voltage(int32_t target_mv)
{
    int32_t min_mv;
    int32_t max_mv;

    min_mv = LEGACY_CHARGE_QC2_DEFAULT_MV + (LEGACY_CHARGE_QC3_MIN_OFFSET * 200);
    max_mv = LEGACY_CHARGE_QC2_DEFAULT_MV + (LEGACY_CHARGE_QC3_MAX_OFFSET * 200);

    if (target_mv < min_mv)
    {
        return min_mv;
    }
    if (target_mv > max_mv)
    {
        return max_mv;
    }

    return target_mv;
}

static int8_t service_legacy_charge_qc3_offset_for_voltage(int32_t target_mv)
{
    return charge_protocol_qc3_offset_for_voltage_mv(service_legacy_charge_clamp_qc3_voltage(target_mv),
                                                     LEGACY_CHARGE_QC2_DEFAULT_MV,
                                                     LEGACY_CHARGE_QC3_MIN_OFFSET,
                                                     LEGACY_CHARGE_QC3_MAX_OFFSET);
}

static void service_legacy_charge_apply_qc3_target(void)
{
    int16_t delta;

    delta = (int16_t)g_request.qc3_step_offset - (int16_t)g_runtime.qc3_applied_step_offset;
    if (delta == 0)
    {
        bsp_dpdm_apply_qc3_pulse(0);
        return;
    }

    while (delta > 0)
    {
        bsp_dpdm_apply_qc3_pulse(1);
        ++g_runtime.qc3_applied_step_offset;
        --delta;
    }

    while (delta < 0)
    {
        bsp_dpdm_apply_qc3_pulse(-1);
        --g_runtime.qc3_applied_step_offset;
        ++delta;
    }
}

static uint8_t service_legacy_charge_voltage_reached(int32_t measured_vbus_mv, int32_t target_mv)
{
    int32_t diff_mv;

    if ((measured_vbus_mv <= 0) || (target_mv <= 0))
    {
        return 0U;
    }

    diff_mv = measured_vbus_mv - target_mv;
    if (diff_mv < 0)
    {
        diff_mv = -diff_mv;
    }

    return (diff_mv <= LEGACY_CHARGE_READY_TOLERANCE_MV) ? 1U : 0U;
}

static void service_legacy_charge_release_active_request(void)
{
    g_requested_protocol = LEGACY_PROTOCOL_NONE;
    g_runtime.request_elapsed_ms = 0U;
    g_runtime.qc3_applied_step_offset = 0;
    bsp_dpdm_set_mode(BSP_DPDM_MODE_HIZ);
}

static void service_legacy_charge_clear_failed_hold(void)
{
    g_runtime.failed_hold_elapsed_ms = 0U;
    g_runtime.failed_visible = 0U;
}

static void service_legacy_charge_mark_failed_visible(void)
{
    g_runtime.failed_hold_elapsed_ms = 0U;
    g_runtime.failed_visible = 1U;
}

static void service_legacy_charge_clear_request_state(void)
{
    g_detected_protocol = LEGACY_PROTOCOL_NONE;
    g_passive_dpdm_present = 0U;
    g_request.protocol = LEGACY_PROTOCOL_NONE;
    g_request.target_mv = 0;
    g_request.qc3_step_offset = 0;
    g_request.status = LEGACY_CHARGE_STATUS_IDLE;
    g_runtime.request_elapsed_ms = 0U;
    service_legacy_charge_clear_failed_hold();
}

static void service_legacy_charge_apply_request(void)
{
    if (service_legacy_charge_typec_cc_attached() != 0U)
    {
        bsp_dpdm_set_mode(BSP_DPDM_MODE_HIZ);
        return;
    }

    if (g_request.protocol == LEGACY_PROTOCOL_QC2)
    {
        bsp_dpdm_level_t dp_level;
        bsp_dpdm_level_t dm_level;
        int32_t actual_mv;

        if (charge_protocol_qc2_levels_for_voltage_mv(g_request.target_mv,
                                                      &dp_level,
                                                      &dm_level,
                                                      &actual_mv) != 0U)
        {
            g_request.target_mv = actual_mv;
            bsp_dpdm_set_levels(dp_level, dm_level);
        }
        return;
    }

    if (g_request.protocol == LEGACY_PROTOCOL_QC3)
    {
        service_legacy_charge_apply_qc3_target();
        return;
    }

    bsp_dpdm_set_mode(service_legacy_charge_mode_for_protocol(g_request.protocol));
}

void service_legacy_charge_init(void)
{
    g_detected_protocol = LEGACY_PROTOCOL_NONE;
    g_requested_protocol = LEGACY_PROTOCOL_NONE;
    g_request.protocol = LEGACY_PROTOCOL_NONE;
    g_request.target_mv = 0;
    g_request.qc3_step_offset = 0;
    g_request.status = LEGACY_CHARGE_STATUS_IDLE;
    g_request.dp_mv = 0;
    g_request.dm_mv = 0;
    g_runtime.request_elapsed_ms = 0U;
    g_runtime.failed_hold_elapsed_ms = 0U;
    g_runtime.qc3_applied_step_offset = 0;
    g_runtime.failed_visible = 0U;
    g_passive_dpdm_present = 0U;
    bsp_cc_ext_rd_init();
    bsp_cc_ext_rd_set(0U);
    bsp_dpdm_init();
    bsp_dpdm_set_mode(BSP_DPDM_MODE_HIZ);
}

void service_legacy_charge_request_protocol(legacy_protocol_t protocol)
{
    g_requested_protocol = protocol;
    g_request.protocol = protocol;
    g_request.qc3_step_offset = 0;
    g_request.status = (protocol == LEGACY_PROTOCOL_NONE) ? LEGACY_CHARGE_STATUS_IDLE : LEGACY_CHARGE_STATUS_REQUESTING;
    g_runtime.request_elapsed_ms = 0U;
    service_legacy_charge_clear_failed_hold();
    g_passive_dpdm_present = 0U;
    if (protocol != LEGACY_PROTOCOL_QC3)
    {
        g_runtime.qc3_applied_step_offset = 0;
    }

    switch (protocol)
    {
        case LEGACY_PROTOCOL_QC2:
            g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV;
            break;
        case LEGACY_PROTOCOL_QC3:
            g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV;
            break;
        case LEGACY_PROTOCOL_AFC:
        case LEGACY_PROTOCOL_FCP:
            g_request.target_mv = 9000;
            break;
        case LEGACY_PROTOCOL_NONE:
        default:
            g_request.target_mv = 0;
            break;
    }

    service_legacy_charge_apply_request();
}

void service_legacy_charge_request_voltage_mv(legacy_protocol_t protocol, int32_t target_mv)
{
    if (g_request.protocol != protocol)
    {
        service_legacy_charge_request_protocol(protocol);
    }
    else
    {
        g_requested_protocol = protocol;
        g_request.status = (protocol == LEGACY_PROTOCOL_NONE) ? LEGACY_CHARGE_STATUS_IDLE : LEGACY_CHARGE_STATUS_REQUESTING;
        g_runtime.request_elapsed_ms = 0U;
        service_legacy_charge_clear_failed_hold();
    }

    if (protocol == LEGACY_PROTOCOL_QC2)
    {
        g_request.target_mv = service_legacy_charge_quantize_qc2_voltage(target_mv);
    }
    else if (protocol == LEGACY_PROTOCOL_QC3)
    {
        g_request.qc3_step_offset = service_legacy_charge_qc3_offset_for_voltage(target_mv);
        g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV + ((int32_t)g_request.qc3_step_offset * 200);
    }
    else if (protocol != LEGACY_PROTOCOL_NONE)
    {
        g_request.target_mv = target_mv;
    }

    g_request.status = (protocol == LEGACY_PROTOCOL_NONE) ? LEGACY_CHARGE_STATUS_IDLE : LEGACY_CHARGE_STATUS_REQUESTING;
    g_runtime.request_elapsed_ms = 0U;
    service_legacy_charge_clear_failed_hold();
    service_legacy_charge_apply_request();
}

void service_legacy_charge_request_qc3_step(int8_t step_delta)
{
    if (g_request.protocol != LEGACY_PROTOCOL_QC3)
    {
        service_legacy_charge_request_protocol(LEGACY_PROTOCOL_QC3);
    }

    if (step_delta > 0)
    {
        if (g_request.qc3_step_offset < LEGACY_CHARGE_QC3_MAX_OFFSET)
        {
            ++g_request.qc3_step_offset;
        }
    }
    else if (step_delta < 0)
    {
        if (g_request.qc3_step_offset > LEGACY_CHARGE_QC3_MIN_OFFSET)
        {
            --g_request.qc3_step_offset;
        }
    }

    g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV + ((int32_t)g_request.qc3_step_offset * 200);
    g_request.status = LEGACY_CHARGE_STATUS_REQUESTING;
    g_runtime.request_elapsed_ms = 0U;
    service_legacy_charge_clear_failed_hold();
    service_legacy_charge_apply_request();
}

legacy_protocol_t service_legacy_charge_detect(void)
{
    if (g_requested_protocol != LEGACY_PROTOCOL_NONE)
    {
        g_detected_protocol = g_requested_protocol;
        return g_detected_protocol;
    }

    g_detected_protocol = LEGACY_PROTOCOL_NONE;

    return g_detected_protocol;
}

uint8_t service_legacy_charge_poll(int32_t measured_vbus_mv,
                                   uint32_t elapsed_ms,
                                   protocol_snapshot_t *snapshot)
{
    bsp_dpdm_sample_t sample;
    legacy_protocol_t protocol;
    protocol_kind_t kind;
    int32_t display_mv;

    if (snapshot == 0)
    {
        return 0U;
    }

    if (bsp_dpdm_sample_lines(&sample) == 0U)
    {
        sample = (bsp_dpdm_sample_t){ 0U, 0U, 0U, 0, 0 };
    }

    if (sample.voltage_valid != 0U)
    {
        g_request.dp_mv = (int16_t)sample.dp_mv;
        g_request.dm_mv = (int16_t)sample.dm_mv;
    }
    else
    {
        g_request.dp_mv = 0;
        g_request.dm_mv = 0;
    }

    if (g_requested_protocol != LEGACY_PROTOCOL_NONE)
    {
        protocol = g_requested_protocol;

        if (g_runtime.request_elapsed_ms <= (0xFFFFFFFFUL - elapsed_ms))
        {
            g_runtime.request_elapsed_ms += elapsed_ms;
        }
        else
        {
            g_runtime.request_elapsed_ms = 0xFFFFFFFFUL;
        }

        if (service_legacy_charge_voltage_reached(measured_vbus_mv, g_request.target_mv) != 0U)
        {
            g_request.status = LEGACY_CHARGE_STATUS_READY;
        }
        else if ((g_runtime.request_elapsed_ms >= LEGACY_CHARGE_REQUEST_TIMEOUT_MS) &&
            (g_request.target_mv > LEGACY_CHARGE_QC2_DEFAULT_MV))
        {
            g_request.status = LEGACY_CHARGE_STATUS_FAILED;
            service_legacy_charge_release_active_request();
            service_legacy_charge_mark_failed_visible();
        }
        else
        {
            g_request.status = LEGACY_CHARGE_STATUS_REQUESTING;
        }
    }
    else if ((g_runtime.failed_visible != 0U) &&
             (g_request.protocol != LEGACY_PROTOCOL_NONE) &&
             (g_request.status == LEGACY_CHARGE_STATUS_FAILED))
    {
        protocol = g_request.protocol;
        if (g_runtime.failed_hold_elapsed_ms <= (0xFFFFFFFFUL - elapsed_ms))
        {
            g_runtime.failed_hold_elapsed_ms += elapsed_ms;
        }
        else
        {
            g_runtime.failed_hold_elapsed_ms = 0xFFFFFFFFUL;
        }

        if (g_runtime.failed_hold_elapsed_ms >= LEGACY_CHARGE_FAILED_HOLD_MS)
        {
            service_legacy_charge_clear_request_state();
            protocol_snapshot_reset(snapshot);
            return 1U;
        }
    }
    else if (((sample.voltage_valid != 0U) &&
              ((sample.dp_mv >= LEGACY_CHARGE_DPDM_PRESENT_MV) || (sample.dm_mv >= LEGACY_CHARGE_DPDM_PRESENT_MV))))
    {
        g_passive_dpdm_present = 1U;
        g_detected_protocol = LEGACY_PROTOCOL_NONE;
        g_request.protocol = LEGACY_PROTOCOL_NONE;
        g_request.target_mv = 0;
        g_request.qc3_step_offset = 0;
        g_request.status = LEGACY_CHARGE_STATUS_AVAILABLE;
        protocol_snapshot_set_other(snapshot);
        snapshot->request_state = PROTOCOL_REQUEST_AVAILABLE;
        snapshot->dp_mv = g_request.dp_mv;
        snapshot->dm_mv = g_request.dm_mv;
        return 1U;
    }
    else
    {
        if ((g_detected_protocol != LEGACY_PROTOCOL_NONE) ||
            (g_passive_dpdm_present != 0U) ||
            ((g_requested_protocol == LEGACY_PROTOCOL_NONE) &&
             (g_request.protocol != LEGACY_PROTOCOL_NONE)))
        {
            service_legacy_charge_clear_request_state();
            protocol_snapshot_reset(snapshot);
            return 1U;
        }

        return 0U;
    }

    kind = service_legacy_charge_protocol_kind(protocol);
    if (kind == PROTOCOL_KIND_NONE)
    {
        return 0U;
    }

    display_mv = (measured_vbus_mv > 0) ? measured_vbus_mv : g_request.target_mv;
    protocol_snapshot_set_legacy(snapshot, kind, display_mv, g_request.qc3_step_offset);
    snapshot->target_mv = g_request.target_mv;
    snapshot->request_state = service_legacy_charge_request_state(g_request.status);
    snapshot->dp_mv = g_request.dp_mv;
    snapshot->dm_mv = g_request.dm_mv;

    return 1U;
}

void service_legacy_charge_copy_request(legacy_charge_request_t *request)
{
    if (request == 0)
    {
        return;
    }

    *request = g_request;
}

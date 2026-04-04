#include "service_legacy_charge.h"

#include "bsp_cc_ext_rd.h"
#include "bsp_dpdm.h"

#define LEGACY_CHARGE_QC2_DEFAULT_MV 5000
#define LEGACY_CHARGE_QC2_MIN_MV 5000
#define LEGACY_CHARGE_QC2_MAX_MV 12000
#define LEGACY_CHARGE_QC2_9V_MV 9000
#define LEGACY_CHARGE_QC2_12V_MV 12000

static legacy_protocol_t g_detected_protocol = LEGACY_PROTOCOL_NONE;
static legacy_protocol_t g_requested_protocol = LEGACY_PROTOCOL_NONE;
static legacy_charge_request_t g_request = { LEGACY_PROTOCOL_NONE, 0, 0 };

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

static void service_legacy_charge_apply_request(void)
{
    bsp_dpdm_set_mode(service_legacy_charge_mode_for_protocol(g_request.protocol));
}

void service_legacy_charge_init(void)
{
    g_detected_protocol = LEGACY_PROTOCOL_NONE;
    g_requested_protocol = LEGACY_PROTOCOL_NONE;
    g_request.protocol = LEGACY_PROTOCOL_NONE;
    g_request.target_mv = 0;
    g_request.qc3_step_offset = 0;
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
    service_legacy_charge_request_protocol(protocol);

    if (protocol == LEGACY_PROTOCOL_QC2)
    {
        g_request.target_mv = service_legacy_charge_clamp_qc2_voltage(target_mv);

        /*
         * PX1 currently exposes the BC/DPDM bias controls but not yet the full
         * charger-side analog state machine needed to distinguish every QC2.0
         * fixed-voltage combination here. Keep the target recorded for UI/app
         * logic while reusing the current DPDM bias profile underneath.
         */
        if (g_request.target_mv >= LEGACY_CHARGE_QC2_12V_MV)
        {
            g_request.target_mv = LEGACY_CHARGE_QC2_12V_MV;
        }
        else if (g_request.target_mv >= LEGACY_CHARGE_QC2_9V_MV)
        {
            g_request.target_mv = LEGACY_CHARGE_QC2_9V_MV;
        }
        else
        {
            g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV;
        }
    }
    else if (protocol != LEGACY_PROTOCOL_NONE)
    {
        g_request.target_mv = target_mv;
    }

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
        if (g_request.qc3_step_offset < 20)
        {
            ++g_request.qc3_step_offset;
        }
    }
    else if (step_delta < 0)
    {
        if (g_request.qc3_step_offset > -20)
        {
            --g_request.qc3_step_offset;
        }
    }

    g_request.target_mv = LEGACY_CHARGE_QC2_DEFAULT_MV + ((int32_t)g_request.qc3_step_offset * 200);
    service_legacy_charge_apply_request();
}

legacy_protocol_t service_legacy_charge_detect(void)
{
    if (g_requested_protocol != LEGACY_PROTOCOL_NONE)
    {
        g_detected_protocol = g_requested_protocol;
        return g_detected_protocol;
    }

    /*
     * Passive legacy-protocol detection is not trustworthy until the PX1 DP/DM
     * analog bias/sense network is fully characterized. Do not infer a charger
     * protocol from the commanded GPIO state alone.
     */
    (void)bsp_dpdm_get_mode();
    g_detected_protocol = LEGACY_PROTOCOL_NONE;

    return g_detected_protocol;
}

void service_legacy_charge_copy_request(legacy_charge_request_t *request)
{
    if (request == 0)
    {
        return;
    }

    *request = g_request;
}

#include "service_pd.h"

#include <stddef.h>
#include <string.h>

#include "bsp_usbpd_port.h"
#include "service_emark.h"
#include "service_pd_objects.h"
#if defined(__riscv)
#include "ch32l103_usbpd.h"
#else
#define DEF_TYPE_REQUEST 0x02U
#define DEF_TYPE_VENDOR_DEFINED 0x0FU
#endif

#define SERVICE_PD_MAX_PDOS 7U
#define SERVICE_PD_DEFAULT_TARGET_MV 5000
#define SERVICE_PD_TIMEOUT_MS 500U
#define SERVICE_PD_SOURCE_CAP_RETRY_MS 200U
#define SERVICE_PD_SOURCE_CAP_MAX_ATTEMPTS 8U
#define SERVICE_PD_VBUS_READY_TOLERANCE_MV 900
#define SERVICE_PD_SVDM_DISCOVER_IDENTITY 0xFF008001UL
#define SERVICE_PD_REQUEST_KIND_NONE 0U
#define SERVICE_PD_REQUEST_KIND_FIXED 1U
#define SERVICE_PD_REQUEST_KIND_PPS 2U
#define SERVICE_PD_CONTROL_GET_SOURCE_CAP 0x07U

typedef enum
{
    SERVICE_PD_STATE_DETACHED = 0,
    SERVICE_PD_STATE_WAIT_SRC_CAP,
    SERVICE_PD_STATE_WAIT_ACCEPT,
    SERVICE_PD_STATE_WAIT_PS_RDY,
    SERVICE_PD_STATE_VERIFY_VBUS,
    SERVICE_PD_STATE_CONTRACT_READY,
} service_pd_state_t;

static protocol_snapshot_t g_protocol_snapshot;
static emark_summary_t g_emark_summary;
static service_pd_state_t g_pd_state;
static int32_t g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
static uint8_t g_preferred_pdo_position;
static uint8_t g_sink_hold_enabled;
static uint8_t g_message_id;
static uint8_t g_selected_pdo_index;
static uint8_t g_request_pending;
static uint8_t g_source_cap_request_pending;
static uint8_t g_source_cap_query_in_flight;
static uint8_t g_source_cap_retry_count;
static uint8_t g_emark_identity_pending;
static uint8_t g_emark_message_id;
static int32_t g_requested_mv;
static int32_t g_requested_ma;
static uint32_t g_source_pdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_source_pdo_positions[SERVICE_PD_MAX_PDOS];
static uint8_t g_source_pdo_count;
static uint32_t g_pdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_pdo_positions[SERVICE_PD_MAX_PDOS];
static uint8_t g_pdo_count;
static uint32_t g_pps_apdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_pps_apdo_positions[SERVICE_PD_MAX_PDOS];
static uint8_t g_pps_apdo_count;
static uint32_t g_state_elapsed_ms;
static uint32_t g_source_cap_elapsed_ms;

static uint8_t service_pd_pdo_is_fixed(uint32_t pdo_word);
static int32_t service_pd_pdo_voltage_mv(uint32_t pdo_word);
static int32_t service_pd_pdo_current_ma(uint32_t pdo_word);
static int32_t service_pd_pps_min_mv(uint32_t apdo_word);
static int32_t service_pd_pps_max_mv(uint32_t apdo_word);
static int32_t service_pd_pps_max_ma(uint32_t apdo_word);
static int32_t service_pd_limit_current_by_cable_ma(int32_t source_ma);
static uint8_t service_pd_voltage_reached(int32_t measured_mv, int32_t target_mv);
static void service_pd_fill_source_pdo_snapshot(service_pd_source_pdo_t *target, const pd_object_t *object);

__attribute__((weak)) void bsp_usbpd_port_set_sink_hold(uint8_t enabled)
{
    (void)enabled;
}

static uint8_t service_pd_request_is_in_flight(void)
{
    return ((g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT) ||
            (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY) ||
            (g_pd_state == SERVICE_PD_STATE_VERIFY_VBUS)) ? 1U : 0U;
}

static void service_pd_apply_emark_summary_to_snapshot(void)
{
    if (g_emark_summary.present == 0U)
    {
        g_protocol_snapshot.emark_present = 0U;
        g_protocol_snapshot.emark_current_a = 0U;
        g_protocol_snapshot.emark_max_voltage_v = 0U;
        g_protocol_snapshot.emark_cable_length_m = 0U;
        g_protocol_snapshot.emark_epr_capable = 0U;
        g_protocol_snapshot.emark_usb_speed_grade = 0U;
        g_protocol_snapshot.emark_cable_type = 0U;
        g_protocol_snapshot.emark_vdo_version = 0U;
        g_protocol_snapshot.emark_firmware_version = 0U;
        g_protocol_snapshot.emark_hardware_version = 0U;
        return;
    }

    g_protocol_snapshot.emark_present = 1U;
    g_protocol_snapshot.emark_current_a = (uint8_t)g_emark_summary.current_capacity_a;
    g_protocol_snapshot.emark_max_voltage_v = g_emark_summary.max_voltage_v;
    g_protocol_snapshot.emark_cable_length_m = g_emark_summary.cable_length_m;
    g_protocol_snapshot.emark_epr_capable = g_emark_summary.epr_capable;
    g_protocol_snapshot.emark_usb_speed_grade = g_emark_summary.usb_speed_grade;
    g_protocol_snapshot.emark_cable_type = g_emark_summary.cable_type;
    g_protocol_snapshot.emark_vdo_version = g_emark_summary.vdo_version;
    g_protocol_snapshot.emark_firmware_version = g_emark_summary.firmware_version;
    g_protocol_snapshot.emark_hardware_version = g_emark_summary.hardware_version;
}

static uint16_t service_pd_clamp_u16(int32_t value)
{
    if (value <= 0)
    {
        return 0U;
    }

    if (value > 65535)
    {
        return 65535U;
    }

    return (uint16_t)value;
}

static uint16_t service_pd_power_deci_w_from_mv_ma(int32_t mv, int32_t ma)
{
    int32_t deci_w;

    if ((mv <= 0) || (ma <= 0))
    {
        return 0U;
    }

    deci_w = (mv * ma + 50000) / 100000;
    return service_pd_clamp_u16(deci_w);
}

static void service_pd_fill_source_pdo_snapshot(service_pd_source_pdo_t *target, const pd_object_t *object)
{
    if ((target == NULL) || (object == NULL))
    {
        return;
    }

    *target = (service_pd_source_pdo_t){ 0 };
    target->position = object->position;

    switch (object->type)
    {
        case PD_OBJECT_TYPE_FIXED:
            target->type = SERVICE_PD_SOURCE_PDO_FIXED;
            target->min_mv = service_pd_clamp_u16(object->fixed.voltage_mv);
            target->max_mv = service_pd_clamp_u16(object->fixed.voltage_mv);
            target->current_ma = service_pd_clamp_u16(object->fixed.current_ma);
            target->power_deci_w = service_pd_power_deci_w_from_mv_ma(object->fixed.voltage_mv, object->fixed.current_ma);
            break;
        case PD_OBJECT_TYPE_BATTERY:
            target->type = SERVICE_PD_SOURCE_PDO_BATTERY;
            target->min_mv = service_pd_clamp_u16(object->battery.min_mv);
            target->max_mv = service_pd_clamp_u16(object->battery.max_mv);
            target->current_ma = (object->battery.max_mv > 0) ?
                                 service_pd_clamp_u16((object->battery.power_mw * 1000) /
                                                      object->battery.max_mv) : 0U;
            target->power_deci_w = service_pd_clamp_u16((object->battery.power_mw + 50) / 100);
            break;
        case PD_OBJECT_TYPE_VARIABLE:
            target->type = SERVICE_PD_SOURCE_PDO_VARIABLE;
            target->min_mv = service_pd_clamp_u16(object->variable.min_mv);
            target->max_mv = service_pd_clamp_u16(object->variable.max_mv);
            target->current_ma = service_pd_clamp_u16(object->variable.current_ma);
            target->power_deci_w = service_pd_power_deci_w_from_mv_ma(object->variable.max_mv,
                                                                      object->variable.current_ma);
            break;
        case PD_OBJECT_TYPE_APDO:
            if (object->apdo_subtype == PD_APDO_SUBTYPE_SPR_PPS)
            {
                target->type = SERVICE_PD_SOURCE_PDO_PPS;
                target->min_mv = service_pd_clamp_u16(object->pps.min_mv);
                target->max_mv = service_pd_clamp_u16(object->pps.max_mv);
                target->current_ma = service_pd_clamp_u16(object->pps.current_ma);
                target->power_deci_w = service_pd_power_deci_w_from_mv_ma(object->pps.max_mv, object->pps.current_ma);
            }
            else if (object->apdo_subtype == PD_APDO_SUBTYPE_SPR_AVS)
            {
                int32_t current_9v_15v_ma;
                int32_t current_15v_20v_ma;
                uint16_t power_low_deci_w;
                uint16_t power_high_deci_w;

                current_9v_15v_ma = object->spr_avs.current_9v_15v_ma;
                current_15v_20v_ma = object->spr_avs.current_15v_20v_ma;
                power_low_deci_w = service_pd_power_deci_w_from_mv_ma(15000, current_9v_15v_ma);
                power_high_deci_w = service_pd_power_deci_w_from_mv_ma(20000, current_15v_20v_ma);
                target->type = SERVICE_PD_SOURCE_PDO_AVS;
                target->min_mv = 9000;
                target->max_mv = 20000;
                target->current_ma = service_pd_clamp_u16((current_9v_15v_ma > current_15v_20v_ma) ?
                                                          current_9v_15v_ma : current_15v_20v_ma);
                target->power_deci_w = (power_low_deci_w > power_high_deci_w) ? power_low_deci_w : power_high_deci_w;
            }
            else if (object->apdo_subtype == PD_APDO_SUBTYPE_EPR_AVS)
            {
                target->type = SERVICE_PD_SOURCE_PDO_AVS;
                target->min_mv = service_pd_clamp_u16(object->epr_avs.min_mv);
                target->max_mv = service_pd_clamp_u16(object->epr_avs.max_mv);
                target->current_ma = (object->epr_avs.max_mv > 0) ?
                                     service_pd_clamp_u16((object->epr_avs.pdp_w * 1000000) /
                                                          object->epr_avs.max_mv) : 0U;
                target->power_deci_w = service_pd_clamp_u16(object->epr_avs.pdp_w * 10);
            }
            break;
        default:
            target->type = SERVICE_PD_SOURCE_PDO_NONE;
            break;
    }
}

static void service_pd_copy_capabilities_to_snapshot(void)
{
    uint8_t index;

    g_protocol_snapshot.source_fixed_count = 0U;
    memset(g_protocol_snapshot.source_fixed_mv, 0, sizeof(g_protocol_snapshot.source_fixed_mv));
    memset(g_protocol_snapshot.source_fixed_ma, 0, sizeof(g_protocol_snapshot.source_fixed_ma));
    g_protocol_snapshot.pps_present = (g_pps_apdo_count != 0U) ? 1U : 0U;
    g_protocol_snapshot.pps_min_mv = 0;
    g_protocol_snapshot.pps_max_mv = 0;
    g_protocol_snapshot.pps_max_ma = 0;

    for (index = 0U; (index < g_pdo_count) && (index < PROTOCOL_MAX_FIXED_PDOS); ++index)
    {
        if (service_pd_pdo_is_fixed(g_pdo_words[index]) != 0U)
        {
            uint8_t out_index;

            out_index = g_protocol_snapshot.source_fixed_count;
            g_protocol_snapshot.source_fixed_mv[out_index] = service_pd_pdo_voltage_mv(g_pdo_words[index]);
            g_protocol_snapshot.source_fixed_ma[out_index] = service_pd_pdo_current_ma(g_pdo_words[index]);
            g_protocol_snapshot.source_fixed_count++;
        }
    }

    for (index = 0U; index < g_pps_apdo_count; ++index)
    {
        int32_t min_mv;
        int32_t max_mv;
        int32_t max_ma;

        min_mv = service_pd_pps_min_mv(g_pps_apdo_words[index]);
        max_mv = service_pd_pps_max_mv(g_pps_apdo_words[index]);
        max_ma = service_pd_pps_max_ma(g_pps_apdo_words[index]);

        if ((g_protocol_snapshot.pps_min_mv == 0) || (min_mv < g_protocol_snapshot.pps_min_mv))
        {
            g_protocol_snapshot.pps_min_mv = min_mv;
        }
        if (max_mv > g_protocol_snapshot.pps_max_mv)
        {
            g_protocol_snapshot.pps_max_mv = max_mv;
        }
        if (max_ma > g_protocol_snapshot.pps_max_ma)
        {
            g_protocol_snapshot.pps_max_ma = max_ma;
        }
    }
}

static void service_pd_mark_request_state(protocol_request_state_t state)
{
    g_protocol_snapshot.request_state = state;
    if (((state == PROTOCOL_REQUEST_ACCEPTED) || (state == PROTOCOL_REQUEST_READY)) &&
        (g_requested_mv > 0))
    {
        g_protocol_snapshot.target_mv = g_requested_mv;
    }
    else
    {
        g_protocol_snapshot.target_mv = g_preferred_voltage_mv;
    }
    g_protocol_snapshot.selected_pdo_index = g_selected_pdo_index;
    service_pd_copy_capabilities_to_snapshot();
}

static void service_pd_mark_capabilities_available(void)
{
    g_protocol_snapshot.kind = PROTOCOL_KIND_PD;
    g_protocol_snapshot.contract_mv = 0;
    g_protocol_snapshot.contract_ma = 0;
    g_protocol_snapshot.request_state = PROTOCOL_REQUEST_AVAILABLE;
    g_protocol_snapshot.target_mv = g_preferred_voltage_mv;
    g_protocol_snapshot.selected_pdo_index = 0U;
    service_pd_copy_capabilities_to_snapshot();
    service_pd_apply_emark_summary_to_snapshot();
}

static void service_pd_mark_request_failed(void)
{
    int32_t failed_target_mv;

    failed_target_mv = (g_requested_mv > 0) ? g_requested_mv : g_preferred_voltage_mv;
    g_request_pending = 0U;
    g_pd_state = SERVICE_PD_STATE_WAIT_SRC_CAP;
    g_state_elapsed_ms = 0U;
    service_pd_mark_request_state(PROTOCOL_REQUEST_FAILED);
    g_protocol_snapshot.target_mv = failed_target_mv;
    g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
    g_preferred_pdo_position = 0U;
}

static uint16_t service_pd_get_u16_le(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8);
}

static uint32_t service_pd_get_u32_le(const uint8_t *bytes)
{
    return (uint32_t)bytes[0] |
           ((uint32_t)bytes[1] << 8) |
           ((uint32_t)bytes[2] << 16) |
           ((uint32_t)bytes[3] << 24);
}

static void service_pd_put_u32_le(uint8_t *bytes, uint32_t value)
{
    bytes[0] = (uint8_t)(value & 0xFFU);
    bytes[1] = (uint8_t)((value >> 8) & 0xFFU);
    bytes[2] = (uint8_t)((value >> 16) & 0xFFU);
    bytes[3] = (uint8_t)((value >> 24) & 0xFFU);
}

static uint8_t service_pd_num_data_objects(uint16_t header)
{
    return (uint8_t)((header >> 12) & 0x07U);
}

static uint8_t service_pd_control_type(uint16_t header)
{
    return (uint8_t)(header & 0x1FU);
}

static uint8_t service_pd_data_type(uint16_t header)
{
    return (uint8_t)(header & 0x0FU);
}

static void service_pd_reset_runtime(uint8_t reset_preference)
{
    protocol_snapshot_reset(&g_protocol_snapshot);
    service_emark_reset(&g_emark_summary);
    g_pd_state = SERVICE_PD_STATE_WAIT_SRC_CAP;
    g_message_id = 0U;
    g_selected_pdo_index = 0U;
    g_request_pending = 0U;
    g_preferred_pdo_position = 0U;
    g_source_cap_request_pending = 0U;
    g_source_cap_query_in_flight = 0U;
    g_source_cap_retry_count = 0U;
    g_emark_identity_pending = 0U;
    g_emark_message_id = 0U;
    g_requested_mv = 0;
    g_requested_ma = 0;
    g_source_pdo_count = 0U;
    g_pdo_count = 0U;
    g_pps_apdo_count = 0U;
    g_state_elapsed_ms = 0U;
    g_source_cap_elapsed_ms = 0U;
    memset(g_source_pdo_words, 0, sizeof(g_source_pdo_words));
    memset(g_source_pdo_positions, 0, sizeof(g_source_pdo_positions));
    memset(g_pdo_words, 0, sizeof(g_pdo_words));
    memset(g_pdo_positions, 0, sizeof(g_pdo_positions));
    memset(g_pps_apdo_words, 0, sizeof(g_pps_apdo_words));
    memset(g_pps_apdo_positions, 0, sizeof(g_pps_apdo_positions));
    if (reset_preference != 0U)
    {
        g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
    }
}

static uint8_t service_pd_pdo_is_fixed(uint32_t pdo_word)
{
    pd_object_t object;

    return ((pd_object_parse_source_pdo(0U, pdo_word, &object) != 0U) &&
            (pd_object_is_fixed(&object) != 0U)) ? 1U : 0U;
}

static int32_t service_pd_pdo_voltage_mv(uint32_t pdo_word)
{
    pd_object_t object;

    if ((pd_object_parse_source_pdo(0U, pdo_word, &object) == 0U) ||
        (pd_object_is_fixed(&object) == 0U))
    {
        return 0;
    }

    return object.fixed.voltage_mv;
}

static int32_t service_pd_pdo_current_ma(uint32_t pdo_word)
{
    pd_object_t object;

    if ((pd_object_parse_source_pdo(0U, pdo_word, &object) == 0U) ||
        (pd_object_is_fixed(&object) == 0U))
    {
        return 0;
    }

    return object.fixed.current_ma;
}

static int32_t service_pd_pps_min_mv(uint32_t apdo_word)
{
    pd_object_t object;

    if ((pd_object_parse_source_pdo(0U, apdo_word, &object) == 0U) ||
        (pd_object_is_pps(&object) == 0U))
    {
        return 0;
    }

    return object.pps.min_mv;
}

static int32_t service_pd_pps_max_mv(uint32_t apdo_word)
{
    pd_object_t object;

    if ((pd_object_parse_source_pdo(0U, apdo_word, &object) == 0U) ||
        (pd_object_is_pps(&object) == 0U))
    {
        return 0;
    }

    return object.pps.max_mv;
}

static int32_t service_pd_pps_max_ma(uint32_t apdo_word)
{
    pd_object_t object;

    if ((pd_object_parse_source_pdo(0U, apdo_word, &object) == 0U) ||
        (pd_object_is_pps(&object) == 0U))
    {
        return 0;
    }

    return object.pps.current_ma;
}

static int32_t service_pd_limit_current_by_cable_ma(int32_t source_ma)
{
    int32_t cable_limit_ma;

    cable_limit_ma = 3000;
    if ((g_emark_summary.present != 0U) && (g_emark_summary.current_capacity_a >= 5U))
    {
        cable_limit_ma = 5000;
    }

    return (source_ma > cable_limit_ma) ? cable_limit_ma : source_ma;
}

static uint8_t service_pd_voltage_reached(int32_t measured_mv, int32_t target_mv)
{
    int32_t diff_mv;

    if ((measured_mv <= 0) || (target_mv <= 0))
    {
        return 0U;
    }

    diff_mv = measured_mv - target_mv;
    if (diff_mv < 0)
    {
        diff_mv = -diff_mv;
    }

    return (diff_mv <= SERVICE_PD_VBUS_READY_TOLERANCE_MV) ? 1U : 0U;
}

static uint8_t service_pd_select_fixed_pdo(void)
{
    uint8_t index;
    uint8_t best_index;
    int32_t best_delta;

    best_index = 0U;
    best_delta = 0x7FFFFFFF;

    for (index = 0U; index < g_pdo_count; ++index)
    {
        int32_t voltage_mv;
        int32_t delta_mv;

        if (service_pd_pdo_is_fixed(g_pdo_words[index]) == 0U)
        {
            continue;
        }

        voltage_mv = service_pd_pdo_voltage_mv(g_pdo_words[index]);
        if (voltage_mv > g_preferred_voltage_mv)
        {
            continue;
        }

        delta_mv = g_preferred_voltage_mv - voltage_mv;
        if ((best_index == 0U) || (delta_mv < best_delta))
        {
            best_index = (uint8_t)(index + 1U);
            best_delta = delta_mv;
        }
    }

    if (best_index == 0U)
    {
        for (index = 0U; index < g_pdo_count; ++index)
        {
            if (service_pd_pdo_is_fixed(g_pdo_words[index]) != 0U)
            {
                best_index = (uint8_t)(index + 1U);
                break;
            }
        }
    }

    return best_index;
}

static uint8_t service_pd_select_pps_apdo(void)
{
    uint8_t index;
    uint8_t best_index;
    int32_t best_current_ma;

    best_index = 0U;
    best_current_ma = 0;

    for (index = 0U; index < g_pps_apdo_count; ++index)
    {
        int32_t min_mv;
        int32_t max_mv;
        int32_t max_ma;

        min_mv = service_pd_pps_min_mv(g_pps_apdo_words[index]);
        max_mv = service_pd_pps_max_mv(g_pps_apdo_words[index]);
        max_ma = service_pd_pps_max_ma(g_pps_apdo_words[index]);

        if ((min_mv <= 0) || (max_mv <= 0) || (max_ma <= 0))
        {
            continue;
        }
        if ((g_preferred_voltage_mv < min_mv) || (g_preferred_voltage_mv > max_mv))
        {
            continue;
        }
        if ((best_index == 0U) || (max_ma > best_current_ma))
        {
            best_index = (uint8_t)(index + 1U);
            best_current_ma = max_ma;
        }
    }

    return best_index;
}

static uint8_t service_pd_select_request_candidate(uint8_t *request_kind)
{
    uint8_t fixed_index;
    uint8_t pps_index;

    if (request_kind != NULL)
    {
        *request_kind = SERVICE_PD_REQUEST_KIND_NONE;
    }

    fixed_index = service_pd_select_fixed_pdo();
    if (fixed_index != 0U)
    {
        int32_t fixed_mv;

        fixed_mv = service_pd_pdo_voltage_mv(g_pdo_words[fixed_index - 1U]);
        if (fixed_mv == g_preferred_voltage_mv)
        {
            if (request_kind != NULL)
            {
                *request_kind = SERVICE_PD_REQUEST_KIND_FIXED;
            }
            return fixed_index;
        }
    }

    pps_index = service_pd_select_pps_apdo();
    if (pps_index != 0U)
    {
        if (request_kind != NULL)
        {
            *request_kind = SERVICE_PD_REQUEST_KIND_PPS;
        }
        return pps_index;
    }

    if (fixed_index != 0U)
    {
        if (request_kind != NULL)
        {
            *request_kind = SERVICE_PD_REQUEST_KIND_FIXED;
        }
        return fixed_index;
    }

    return 0U;
}

static int32_t service_pd_clamp_mv_to_range(int32_t target_mv, int32_t min_mv, int32_t max_mv)
{
    if (target_mv <= 0)
    {
        target_mv = min_mv;
    }
    if (target_mv < min_mv)
    {
        target_mv = min_mv;
    }
    if (target_mv > max_mv)
    {
        target_mv = max_mv;
    }
    return target_mv;
}

static uint8_t service_pd_find_source_pdo(uint8_t position, pd_object_t *object)
{
    uint8_t index;

    if ((position == 0U) || (object == NULL))
    {
        return 0U;
    }

    for (index = 0U; index < g_source_pdo_count; ++index)
    {
        if (g_source_pdo_positions[index] == position)
        {
            return pd_object_parse_source_pdo(position, g_source_pdo_words[index], object);
        }
    }

    return 0U;
}

static uint8_t service_pd_build_current_request_rdo(const pd_object_t *object,
                                                    int32_t operating_ma,
                                                    uint32_t *rdo)
{
    uint32_t current_units;

    if ((object == NULL) || (rdo == NULL) || (object->position == 0U))
    {
        return 0U;
    }

    current_units = (uint32_t)(operating_ma / 10);
    if (current_units > 0x03FFU)
    {
        current_units = 0x03FFU;
    }
    *rdo = ((uint32_t)object->position << 28) |
           (1UL << 25) |
           (1UL << 24) |
           (current_units << 10) |
           current_units;
    return 1U;
}

static void service_pd_emit_request_packet(uint32_t request_word,
                                           uint8_t *tx_packet,
                                           uint8_t *tx_length)
{
    tx_packet[0] = 0x80U | DEF_TYPE_REQUEST;
    tx_packet[1] = (uint8_t)((g_message_id & 0x0EU) | 0x10U);
    service_pd_put_u32_le(&tx_packet[2], request_word);
    g_message_id = (uint8_t)((g_message_id + 2U) & 0x0EU);
    g_pd_state = SERVICE_PD_STATE_WAIT_ACCEPT;
    g_state_elapsed_ms = 0U;
    protocol_snapshot_set_pd(&g_protocol_snapshot,
                             g_requested_mv,
                             g_requested_ma,
                             g_emark_summary.present);
    service_pd_copy_capabilities_to_snapshot();
    service_pd_apply_emark_summary_to_snapshot();
    service_pd_mark_request_state(PROTOCOL_REQUEST_REQUESTING);
    *tx_length = 6U;
}

static uint8_t service_pd_prepare_position_request_packet(uint8_t position,
                                                          int32_t target_mv,
                                                          uint8_t *tx_packet,
                                                          uint8_t *tx_length)
{
    pd_object_t object;
    uint32_t request_word;

    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }
    if ((tx_packet == NULL) || (tx_length == NULL) ||
        (service_pd_find_source_pdo(position, &object) == 0U))
    {
        return 0U;
    }

    request_word = 0U;
    g_selected_pdo_index = object.position;
    switch (object.type)
    {
        case PD_OBJECT_TYPE_FIXED:
            g_requested_mv = object.fixed.voltage_mv;
            g_requested_ma = service_pd_limit_current_by_cable_ma(object.fixed.current_ma);
            if (pd_object_build_fixed_rdo(&object, g_requested_ma, &request_word) == 0U)
            {
                return 0U;
            }
            break;
        case PD_OBJECT_TYPE_VARIABLE:
            g_requested_mv = service_pd_clamp_mv_to_range(target_mv,
                                                          object.variable.min_mv,
                                                          object.variable.max_mv);
            g_requested_ma = service_pd_limit_current_by_cable_ma(object.variable.current_ma);
            if (service_pd_build_current_request_rdo(&object, g_requested_ma, &request_word) == 0U)
            {
                return 0U;
            }
            break;
        case PD_OBJECT_TYPE_APDO:
            if (object.apdo_subtype == PD_APDO_SUBTYPE_SPR_PPS)
            {
                g_requested_mv = service_pd_clamp_mv_to_range(target_mv,
                                                              object.pps.min_mv,
                                                              object.pps.max_mv);
                g_requested_ma = service_pd_limit_current_by_cable_ma(object.pps.current_ma);
                if (pd_object_build_pps_rdo(&object, g_requested_mv, g_requested_ma, &request_word) == 0U)
                {
                    return 0U;
                }
            }
            else if (object.apdo_subtype == PD_APDO_SUBTYPE_SPR_AVS)
            {
                g_requested_mv = service_pd_clamp_mv_to_range(target_mv, 9000, 20000);
                g_requested_ma = (g_requested_mv <= 15000) ?
                                 object.spr_avs.current_9v_15v_ma :
                                 object.spr_avs.current_15v_20v_ma;
                g_requested_ma = service_pd_limit_current_by_cable_ma(g_requested_ma);
                if (pd_object_build_avs_rdo(&object, g_requested_mv, g_requested_ma, &request_word) == 0U)
                {
                    return 0U;
                }
            }
            else if (object.apdo_subtype == PD_APDO_SUBTYPE_EPR_AVS)
            {
                g_requested_mv = service_pd_clamp_mv_to_range(target_mv,
                                                              object.epr_avs.min_mv,
                                                              object.epr_avs.max_mv);
                g_requested_ma = (g_requested_mv > 0) ?
                                 (object.epr_avs.pdp_w * 1000000) / g_requested_mv : 0;
                g_requested_ma = service_pd_limit_current_by_cable_ma(g_requested_ma);
                if (pd_object_build_avs_rdo(&object, g_requested_mv, g_requested_ma, &request_word) == 0U)
                {
                    return 0U;
                }
            }
            else
            {
                return 0U;
            }
            break;
        default:
            return 0U;
    }

    if (g_requested_ma <= 0)
    {
        return 0U;
    }

    service_pd_emit_request_packet(request_word, tx_packet, tx_length);
    return (*tx_length != 0U) ? 1U : 0U;
}

static void service_pd_prepare_fixed_request_packet(uint8_t pdo_index,
                                                    uint8_t *tx_packet,
                                                    uint8_t *tx_length)
{
    uint32_t selected_pdo;
    uint8_t object_position;
    uint32_t request_word;
    pd_object_t object;

    selected_pdo = g_pdo_words[pdo_index - 1U];
    object_position = g_pdo_positions[pdo_index - 1U];
    if (pd_object_parse_source_pdo(object_position, selected_pdo, &object) == 0U)
    {
        *tx_length = 0U;
        return;
    }

    g_selected_pdo_index = object_position;
    g_requested_mv = object.fixed.voltage_mv;
    g_requested_ma = service_pd_limit_current_by_cable_ma(object.fixed.current_ma);
    if (pd_object_build_fixed_rdo(&object, g_requested_ma, &request_word) == 0U)
    {
        *tx_length = 0U;
        return;
    }

    service_pd_emit_request_packet(request_word, tx_packet, tx_length);
}

static void service_pd_prepare_pps_request_packet(uint8_t pps_index,
                                                  uint8_t *tx_packet,
                                                  uint8_t *tx_length)
{
    uint32_t selected_apdo;
    uint8_t object_position;
    uint32_t current_units;
    uint32_t request_word;
    pd_object_t object;

    selected_apdo = g_pps_apdo_words[pps_index - 1U];
    object_position = g_pps_apdo_positions[pps_index - 1U];
    if (pd_object_parse_source_pdo(object_position, selected_apdo, &object) == 0U)
    {
        *tx_length = 0U;
        return;
    }

    g_selected_pdo_index = object_position;
    g_requested_mv = g_preferred_voltage_mv;
    g_requested_ma = service_pd_limit_current_by_cable_ma(object.pps.current_ma);

    current_units = (uint32_t)(g_requested_ma / 50);
    if (current_units > 0x7FU)
    {
        current_units = 0x7FU;
        g_requested_ma = (int32_t)(current_units * 50U);
    }
    if (pd_object_build_pps_rdo(&object, g_requested_mv, g_requested_ma, &request_word) == 0U)
    {
        *tx_length = 0U;
        return;
    }

    service_pd_emit_request_packet(request_word, tx_packet, tx_length);
}

static void service_pd_update_selected_contract(uint8_t pdo_index)
{
    uint32_t selected_pdo;

    if ((pdo_index == 0U) || (pdo_index > g_pdo_count))
    {
        return;
    }

    selected_pdo = g_pdo_words[pdo_index - 1U];
    g_selected_pdo_index = g_pdo_positions[pdo_index - 1U];
    g_requested_mv = service_pd_pdo_voltage_mv(selected_pdo);
    g_requested_ma = service_pd_limit_current_by_cable_ma(service_pd_pdo_current_ma(selected_pdo));
}

static void service_pd_update_selected_pps_contract(uint8_t pps_index)
{
    uint32_t selected_apdo;

    if ((pps_index == 0U) || (pps_index > g_pps_apdo_count))
    {
        return;
    }

    selected_apdo = g_pps_apdo_words[pps_index - 1U];
    g_selected_pdo_index = g_pps_apdo_positions[pps_index - 1U];
    g_requested_mv = g_preferred_voltage_mv;
    g_requested_ma = service_pd_limit_current_by_cable_ma(service_pd_pps_max_ma(selected_apdo));
}

static void service_pd_update_contract(uint8_t attached, protocol_request_state_t request_state)
{
    if ((attached == 0U) || (g_selected_pdo_index == 0U) || (g_requested_mv <= 0) || (g_requested_ma <= 0))
    {
        protocol_snapshot_reset(&g_protocol_snapshot);
        service_pd_apply_emark_summary_to_snapshot();
        return;
    }

    protocol_snapshot_set_pd(&g_protocol_snapshot,
                             g_requested_mv,
                             g_requested_ma,
                             g_emark_summary.present);
    service_pd_apply_emark_summary_to_snapshot();
    if ((request_state == PROTOCOL_REQUEST_READY) && (g_emark_summary.present == 0U))
    {
        g_emark_identity_pending = 1U;
    }
    service_pd_mark_request_state(request_state);
}

void service_pd_init(void)
{
    service_pd_reset_runtime(1U);
    if (g_sink_hold_enabled != 0U)
    {
        bsp_usbpd_port_set_sink_hold(1U);
    }
}

void service_pd_handle_detach(void)
{
    service_pd_reset_runtime(1U);
}

void service_pd_handle_timeout_ms(uint32_t elapsed_ms)
{
    if (g_source_cap_query_in_flight != 0U)
    {
        if (g_source_cap_elapsed_ms <= (0xFFFFFFFFUL - elapsed_ms))
        {
            g_source_cap_elapsed_ms += elapsed_ms;
        }
        else
        {
            g_source_cap_elapsed_ms = 0xFFFFFFFFUL;
        }
    }

    if ((g_source_cap_query_in_flight != 0U) &&
        (g_source_pdo_count == 0U) &&
        (g_source_cap_elapsed_ms >= SERVICE_PD_SOURCE_CAP_RETRY_MS))
    {
        g_source_cap_query_in_flight = 0U;
        g_source_cap_elapsed_ms = 0U;
        if (g_source_cap_retry_count < SERVICE_PD_SOURCE_CAP_MAX_ATTEMPTS)
        {
            g_source_cap_request_pending = 1U;
        }
    }

    if ((g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT) || (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY))
    {
        if (g_state_elapsed_ms <= (0xFFFFFFFFUL - elapsed_ms))
        {
            g_state_elapsed_ms += elapsed_ms;
        }
        else
        {
            g_state_elapsed_ms = 0xFFFFFFFFUL;
        }
    }

    if (((g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT) || (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY)) &&
        (g_state_elapsed_ms >= SERVICE_PD_TIMEOUT_MS))
    {
        service_pd_mark_request_failed();
    }
}

void service_pd_handle_vbus_measurement(int32_t measured_vbus_mv, uint32_t elapsed_ms)
{
    if (g_pd_state != SERVICE_PD_STATE_VERIFY_VBUS)
    {
        return;
    }

    if (service_pd_voltage_reached(measured_vbus_mv, g_requested_mv) != 0U)
    {
        g_pd_state = SERVICE_PD_STATE_CONTRACT_READY;
        g_state_elapsed_ms = 0U;
        service_pd_update_contract(1U, PROTOCOL_REQUEST_READY);
        return;
    }

    if (g_state_elapsed_ms <= (0xFFFFFFFFUL - elapsed_ms))
    {
        g_state_elapsed_ms += elapsed_ms;
    }
    else
    {
        g_state_elapsed_ms = 0xFFFFFFFFUL;
    }

    if (g_state_elapsed_ms >= SERVICE_PD_TIMEOUT_MS)
    {
        service_pd_mark_request_failed();
    }
}

void service_pd_copy_snapshot(protocol_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    *snapshot = g_protocol_snapshot;
}

void service_pd_copy_source_caps(service_pd_source_caps_snapshot_t *snapshot)
{
    uint8_t index;

    if (snapshot == NULL)
    {
        return;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    for (index = 0U; (index < g_source_pdo_count) && (index < SERVICE_PD_SOURCE_PDO_MAX); ++index)
    {
        pd_object_t object;

        if (pd_object_parse_source_pdo(g_source_pdo_positions[index], g_source_pdo_words[index], &object) != 0U)
        {
            service_pd_fill_source_pdo_snapshot(&snapshot->pdos[snapshot->count], &object);
            snapshot->count++;
        }
    }
}

void service_pd_set_preferred_voltage_mv(int32_t target_mv)
{
    g_preferred_pdo_position = 0U;
    if (target_mv <= 0)
    {
        g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
        return;
    }

    g_preferred_voltage_mv = target_mv;
    g_request_pending = 1U;
    if (service_pd_request_is_in_flight() == 0U)
    {
        g_protocol_snapshot.target_mv = g_preferred_voltage_mv;
        if ((g_pdo_count != 0U) || (g_pps_apdo_count != 0U))
        {
            g_protocol_snapshot.request_state = PROTOCOL_REQUEST_REQUESTING;
        }
    }
}

void service_pd_set_sink_hold(uint8_t enabled)
{
    g_sink_hold_enabled = (enabled != 0U) ? 1U : 0U;
    bsp_usbpd_port_set_sink_hold(g_sink_hold_enabled);
}

uint8_t service_pd_sink_hold_enabled(void)
{
    return g_sink_hold_enabled;
}

uint8_t service_pd_request_pdo_position(uint8_t position, int32_t target_mv)
{
    if (position == 0U)
    {
        return 0U;
    }

    g_preferred_pdo_position = position;
    g_preferred_voltage_mv = (target_mv > 0) ? target_mv : SERVICE_PD_DEFAULT_TARGET_MV;
    g_request_pending = 1U;
    if (service_pd_request_is_in_flight() == 0U)
    {
        g_protocol_snapshot.target_mv = g_preferred_voltage_mv;
        if (g_source_pdo_count != 0U)
        {
            g_protocol_snapshot.request_state = PROTOCOL_REQUEST_REQUESTING;
        }
    }
    return 1U;
}

void service_pd_request_source_capabilities(void)
{
    g_source_cap_request_pending = 1U;
    g_source_cap_query_in_flight = 0U;
    g_source_cap_retry_count = 0U;
    g_source_cap_elapsed_ms = 0U;
}

void service_pd_request_emark_identity(void)
{
    g_emark_identity_pending = 1U;
}

uint8_t service_pd_prepare_pending_request(uint8_t *tx_packet, uint8_t *tx_length)
{
    uint8_t request_kind;
    uint8_t selected_index;

    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }

    if ((tx_packet == NULL) || (tx_length == NULL) ||
        (g_request_pending == 0U) || (g_source_pdo_count == 0U) ||
        (service_pd_request_is_in_flight() != 0U))
    {
        return 0U;
    }

    if (g_preferred_pdo_position != 0U)
    {
        g_request_pending = 0U;
        if (service_pd_prepare_position_request_packet(g_preferred_pdo_position,
                                                       g_preferred_voltage_mv,
                                                       tx_packet,
                                                       tx_length) != 0U)
        {
            return 1U;
        }
        g_protocol_snapshot.request_state = PROTOCOL_REQUEST_FAILED;
        return 0U;
    }

    selected_index = service_pd_select_request_candidate(&request_kind);
    if (selected_index == 0U)
    {
        g_request_pending = 0U;
        g_protocol_snapshot.request_state = PROTOCOL_REQUEST_FAILED;
        return 0U;
    }

    g_request_pending = 0U;
    if (request_kind == SERVICE_PD_REQUEST_KIND_PPS)
    {
        service_pd_prepare_pps_request_packet(selected_index, tx_packet, tx_length);
    }
    else
    {
        service_pd_prepare_fixed_request_packet(selected_index, tx_packet, tx_length);
    }

    return (*tx_length != 0U) ? 1U : 0U;
}

uint8_t service_pd_prepare_source_cap_request(uint8_t *tx_packet, uint8_t *tx_length)
{
    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }

    if ((tx_packet == NULL) || (tx_length == NULL) ||
        (g_source_cap_request_pending == 0U) ||
        (g_pd_state == SERVICE_PD_STATE_DETACHED) ||
        (service_pd_request_is_in_flight() != 0U))
    {
        return 0U;
    }

    tx_packet[0] = (uint8_t)(0x80U | SERVICE_PD_CONTROL_GET_SOURCE_CAP);
    tx_packet[1] = (uint8_t)(g_message_id & 0x0EU);
    g_message_id = (uint8_t)((g_message_id + 2U) & 0x0EU);
    g_source_cap_request_pending = 0U;
    g_source_cap_query_in_flight = 1U;
    g_source_cap_elapsed_ms = 0U;
    if (g_source_cap_retry_count < 0xFFU)
    {
        g_source_cap_retry_count++;
    }
    *tx_length = 2U;
    return 1U;
}

uint8_t service_pd_prepare_emark_identity_request(uint8_t *tx_packet, uint8_t *tx_length)
{
    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }

    if ((tx_packet == NULL) || (tx_length == NULL) ||
        (g_emark_identity_pending == 0U) ||
        (g_pd_state != SERVICE_PD_STATE_CONTRACT_READY))
    {
        return 0U;
    }

    tx_packet[0] = 0x80U | DEF_TYPE_VENDOR_DEFINED;
    tx_packet[1] = (uint8_t)((g_emark_message_id & 0x0EU) | 0x10U);
    service_pd_put_u32_le(&tx_packet[2], SERVICE_PD_SVDM_DISCOVER_IDENTITY);

    g_emark_message_id = (uint8_t)((g_emark_message_id + 2U) & 0x0EU);
    g_emark_identity_pending = 0U;
    *tx_length = 6U;
    return 1U;
}

void protocol_snapshot_reset(protocol_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->kind = PROTOCOL_KIND_NONE;
    snapshot->contract_mv = 0;
    snapshot->contract_ma = 0;
    snapshot->emark_present = 0U;
    snapshot->emark_current_a = 0U;
    snapshot->emark_max_voltage_v = 0U;
    snapshot->emark_cable_length_m = 0U;
    snapshot->emark_epr_capable = 0U;
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
    snapshot->emark_vdo_version = 0U;
    snapshot->emark_firmware_version = 0U;
    snapshot->emark_hardware_version = 0U;
    snapshot->legacy_step_offset = 0;
    snapshot->request_state = PROTOCOL_REQUEST_IDLE;
    snapshot->target_mv = 0;
    snapshot->selected_pdo_index = 0U;
    snapshot->source_fixed_count = 0U;
    memset(snapshot->source_fixed_mv, 0, sizeof(snapshot->source_fixed_mv));
    memset(snapshot->source_fixed_ma, 0, sizeof(snapshot->source_fixed_ma));
    snapshot->dp_mv = 0;
    snapshot->dm_mv = 0;
    snapshot->cc_orientation = 0U;
    snapshot->cc_attached = 0U;
    snapshot->cc_active_mask = 0U;
    snapshot->pps_present = 0U;
    snapshot->pps_min_mv = 0;
    snapshot->pps_max_mv = 0;
    snapshot->pps_max_ma = 0;
}

void protocol_snapshot_set_pd(protocol_snapshot_t *snapshot,
                              int32_t contract_mv,
                              int32_t contract_ma,
                              uint8_t emark_present)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->kind = PROTOCOL_KIND_PD;
    snapshot->contract_mv = contract_mv;
    snapshot->contract_ma = contract_ma;
    snapshot->emark_present = (emark_present != 0U) ? 1U : 0U;
    snapshot->emark_current_a = 0U;
    snapshot->emark_max_voltage_v = 0U;
    snapshot->emark_cable_length_m = 0U;
    snapshot->emark_epr_capable = 0U;
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
    snapshot->emark_vdo_version = 0U;
    snapshot->emark_firmware_version = 0U;
    snapshot->emark_hardware_version = 0U;
    snapshot->legacy_step_offset = 0;
    snapshot->request_state = PROTOCOL_REQUEST_READY;
    snapshot->target_mv = contract_mv;
    snapshot->selected_pdo_index = 0U;
    snapshot->source_fixed_count = 0U;
    memset(snapshot->source_fixed_mv, 0, sizeof(snapshot->source_fixed_mv));
    memset(snapshot->source_fixed_ma, 0, sizeof(snapshot->source_fixed_ma));
    snapshot->dp_mv = 0;
    snapshot->dm_mv = 0;
    snapshot->cc_orientation = 0U;
    snapshot->cc_attached = 0U;
    snapshot->cc_active_mask = 0U;
    snapshot->pps_present = 0U;
    snapshot->pps_min_mv = 0;
    snapshot->pps_max_mv = 0;
    snapshot->pps_max_ma = 0;
}

void protocol_snapshot_set_legacy(protocol_snapshot_t *snapshot,
                                  protocol_kind_t kind,
                                  int32_t target_mv,
                                  int8_t step_offset)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->kind = kind;
    snapshot->contract_mv = target_mv;
    snapshot->contract_ma = 0;
    snapshot->emark_present = 0U;
    snapshot->emark_current_a = 0U;
    snapshot->emark_max_voltage_v = 0U;
    snapshot->emark_cable_length_m = 0U;
    snapshot->emark_epr_capable = 0U;
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
    snapshot->emark_vdo_version = 0U;
    snapshot->emark_firmware_version = 0U;
    snapshot->emark_hardware_version = 0U;
    snapshot->legacy_step_offset = step_offset;
    snapshot->request_state = PROTOCOL_REQUEST_READY;
    snapshot->target_mv = target_mv;
    snapshot->selected_pdo_index = 0U;
    snapshot->source_fixed_count = 0U;
    memset(snapshot->source_fixed_mv, 0, sizeof(snapshot->source_fixed_mv));
    memset(snapshot->source_fixed_ma, 0, sizeof(snapshot->source_fixed_ma));
    snapshot->dp_mv = 0;
    snapshot->dm_mv = 0;
    snapshot->cc_orientation = 0U;
    snapshot->cc_attached = 0U;
    snapshot->cc_active_mask = 0U;
    snapshot->pps_present = 0U;
    snapshot->pps_min_mv = 0;
    snapshot->pps_max_mv = 0;
    snapshot->pps_max_ma = 0;
}

void protocol_snapshot_set_other(protocol_snapshot_t *snapshot)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->kind = PROTOCOL_KIND_OTHER;
    snapshot->contract_mv = 0;
    snapshot->contract_ma = 0;
    snapshot->emark_present = 0U;
    snapshot->emark_current_a = 0U;
    snapshot->emark_max_voltage_v = 0U;
    snapshot->emark_cable_length_m = 0U;
    snapshot->emark_epr_capable = 0U;
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
    snapshot->emark_vdo_version = 0U;
    snapshot->emark_firmware_version = 0U;
    snapshot->emark_hardware_version = 0U;
    snapshot->legacy_step_offset = 0;
    snapshot->request_state = PROTOCOL_REQUEST_IDLE;
    snapshot->target_mv = 0;
    snapshot->selected_pdo_index = 0U;
    snapshot->source_fixed_count = 0U;
    memset(snapshot->source_fixed_mv, 0, sizeof(snapshot->source_fixed_mv));
    memset(snapshot->source_fixed_ma, 0, sizeof(snapshot->source_fixed_ma));
    snapshot->dp_mv = 0;
    snapshot->dm_mv = 0;
    snapshot->cc_orientation = 0U;
    snapshot->cc_attached = 0U;
    snapshot->cc_active_mask = 0U;
    snapshot->pps_present = 0U;
    snapshot->pps_min_mv = 0;
    snapshot->pps_max_mv = 0;
    snapshot->pps_max_ma = 0;
}

void protocol_snapshot_set_cc_orientation(protocol_snapshot_t *snapshot, uint8_t cc_orientation)
{
    if (snapshot == NULL)
    {
        return;
    }

    snapshot->cc_orientation = (cc_orientation <= 2U) ? cc_orientation : 0U;
    snapshot->cc_attached = (snapshot->cc_orientation != 0U) ? 1U : 0U;
    snapshot->cc_active_mask = (snapshot->cc_orientation == 1U) ? 0x01U :
                               ((snapshot->cc_orientation == 2U) ? 0x02U : 0x00U);
}

uint8_t service_pd_handle_rx_packet(const uint8_t *packet,
                                    uint8_t byte_count,
                                    uint8_t *tx_packet,
                                    uint8_t *tx_length)
{
    uint16_t header;
    uint8_t ndo;

    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }

    if ((packet == NULL) || (byte_count < 2U) || (tx_packet == NULL) || (tx_length == NULL))
    {
        return 0U;
    }

    header = service_pd_get_u16_le(packet);
    ndo = service_pd_num_data_objects(header);

    if (ndo == 0U)
    {
        switch (service_pd_control_type(header))
        {
#if defined(__riscv)
            case DEF_TYPE_ACCEPT:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT)
                {
                    g_pd_state = SERVICE_PD_STATE_WAIT_PS_RDY;
                    g_state_elapsed_ms = 0U;
                    service_pd_mark_request_state(PROTOCOL_REQUEST_ACCEPTED);
                }
                break;
            case DEF_TYPE_PS_RDY:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY)
                {
                    g_pd_state = SERVICE_PD_STATE_VERIFY_VBUS;
                    g_state_elapsed_ms = 0U;
                    service_pd_update_contract(1U, PROTOCOL_REQUEST_ACCEPTED);
                }
                break;
            case DEF_TYPE_REJECT:
            case DEF_TYPE_WAIT:
                service_pd_mark_request_failed();
                break;
            case DEF_TYPE_SOFT_RESET:
                service_pd_handle_detach();
                break;
#else
            case 0x03U:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT)
                {
                    g_pd_state = SERVICE_PD_STATE_WAIT_PS_RDY;
                    g_state_elapsed_ms = 0U;
                    service_pd_mark_request_state(PROTOCOL_REQUEST_ACCEPTED);
                }
                break;
            case 0x06U:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY)
                {
                    g_pd_state = SERVICE_PD_STATE_VERIFY_VBUS;
                    g_state_elapsed_ms = 0U;
                    service_pd_update_contract(1U, PROTOCOL_REQUEST_ACCEPTED);
                }
                break;
            case 0x04U:
            case 0x0CU:
                service_pd_mark_request_failed();
                break;
            case 0x0DU:
                service_pd_handle_detach();
                break;
#endif
            default:
                break;
        }

        return 0U;
    }

    if (byte_count < (uint8_t)(2U + (uint8_t)(ndo * 4U)))
    {
        return 0U;
    }

    switch (service_pd_data_type(header))
    {
        case 0x01U:
        {
            uint8_t index;
            uint8_t request_kind;
            uint8_t selected_index;
            uint8_t source_cap_query_response;

            source_cap_query_response = g_source_cap_query_in_flight;
            g_source_cap_query_in_flight = 0U;
            g_source_cap_request_pending = 0U;
            g_source_cap_retry_count = 0U;
            g_source_cap_elapsed_ms = 0U;
            g_source_pdo_count = 0U;
            g_pdo_count = 0U;
            g_pps_apdo_count = 0U;
            memset(g_source_pdo_words, 0, sizeof(g_source_pdo_words));
            memset(g_source_pdo_positions, 0, sizeof(g_source_pdo_positions));
            memset(g_pdo_words, 0, sizeof(g_pdo_words));
            memset(g_pdo_positions, 0, sizeof(g_pdo_positions));
            memset(g_pps_apdo_words, 0, sizeof(g_pps_apdo_words));
            memset(g_pps_apdo_positions, 0, sizeof(g_pps_apdo_positions));
            for (index = 0U; (index < ndo) && (index < SERVICE_PD_MAX_PDOS); ++index)
            {
                uint32_t pdo_word;
                pd_object_t object;

                pdo_word = service_pd_get_u32_le(&packet[2U + (index * 4U)]);
                if (pd_object_parse_source_pdo((uint8_t)(index + 1U), pdo_word, &object) == 0U)
                {
                    continue;
                }

                g_source_pdo_positions[g_source_pdo_count] = object.position;
                g_source_pdo_words[g_source_pdo_count++] = pdo_word;

                if (pd_object_is_pps(&object) != 0U)
                {
                    g_pps_apdo_positions[g_pps_apdo_count] = object.position;
                    g_pps_apdo_words[g_pps_apdo_count++] = pdo_word;
                    continue;
                }

                if (pd_object_is_fixed(&object) != 0U)
                {
                    g_pdo_positions[g_pdo_count] = object.position;
                    g_pdo_words[g_pdo_count++] = pdo_word;
                }
            }

            service_pd_copy_capabilities_to_snapshot();
            if (service_pd_request_is_in_flight() != 0U)
            {
                return 0U;
            }

            if ((source_cap_query_response != 0U) && (g_pd_state == SERVICE_PD_STATE_CONTRACT_READY))
            {
                return 0U;
            }

            if (g_request_pending == 0U)
            {
                service_pd_mark_capabilities_available();
                return 0U;
            }

            if (g_preferred_pdo_position != 0U)
            {
                g_request_pending = 0U;
                if (service_pd_prepare_position_request_packet(g_preferred_pdo_position,
                                                               g_preferred_voltage_mv,
                                                               tx_packet,
                                                               tx_length) != 0U)
                {
                    return 1U;
                }
                g_protocol_snapshot.request_state = PROTOCOL_REQUEST_FAILED;
                return 0U;
            }

            if ((g_pdo_count != 0U) || (g_pps_apdo_count != 0U))
            {
                selected_index = service_pd_select_request_candidate(&request_kind);
                if (selected_index != 0U)
                {
                    if (request_kind == SERVICE_PD_REQUEST_KIND_PPS)
                    {
                        service_pd_update_selected_pps_contract(selected_index);
                    }
                    else
                    {
                        service_pd_update_selected_contract(selected_index);
                    }
                    protocol_snapshot_set_pd(&g_protocol_snapshot,
                                             g_requested_mv,
                                             g_requested_ma,
                                             g_emark_summary.present);
                    service_pd_copy_capabilities_to_snapshot();
                    service_pd_apply_emark_summary_to_snapshot();
                    g_request_pending = 0U;
                    if (request_kind == SERVICE_PD_REQUEST_KIND_PPS)
                    {
                        service_pd_prepare_pps_request_packet(selected_index, tx_packet, tx_length);
                    }
                    else
                    {
                        service_pd_prepare_fixed_request_packet(selected_index, tx_packet, tx_length);
                    }
                    return 1U;
                }
            }
            break;
        }
        case 0x0FU:
        {
            uint32_t identity_words[SERVICE_PD_MAX_PDOS];
            uint32_t vdm_header;
            uint8_t identity_count;
            uint8_t index;

            vdm_header = service_pd_get_u32_le(&packet[2]);
            if ((vdm_header & 0x1FU) == 0x01U)
            {
                identity_count = (uint8_t)(ndo - 1U);
                if (identity_count > SERVICE_PD_MAX_PDOS)
                {
                    identity_count = SERVICE_PD_MAX_PDOS;
                }

                for (index = 0U; index < identity_count; ++index)
                {
                    identity_words[index] = service_pd_get_u32_le(&packet[6U + (index * 4U)]);
                }

                if (service_emark_summarize_identity(identity_words,
                                                     identity_count,
                                                     &g_emark_summary) != 0U)
                {
                    service_pd_apply_emark_summary_to_snapshot();
                }
            }
            break;
        }
        default:
            break;
    }

    return 0U;
}

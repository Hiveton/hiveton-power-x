#include "service_pd.h"

#include <stddef.h>
#include <string.h>

#include "service_emark.h"
#if defined(__riscv)
#include "ch32l103_usbpd.h"
#else
#define DEF_TYPE_REQUEST 0x02U
#define DEF_TYPE_VENDOR_DEFINED 0x0FU
#endif

#define SERVICE_PD_MAX_PDOS 7U
#define SERVICE_PD_DEFAULT_TARGET_MV 5000
#define SERVICE_PD_TIMEOUT_MS 500U
#define SERVICE_PD_VBUS_READY_TOLERANCE_MV 900
#define SERVICE_PD_SVDM_DISCOVER_IDENTITY 0xFF008001UL
#define SERVICE_PD_REQUEST_KIND_NONE 0U
#define SERVICE_PD_REQUEST_KIND_FIXED 1U
#define SERVICE_PD_REQUEST_KIND_PPS 2U

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
static uint8_t g_message_id;
static uint8_t g_selected_pdo_index;
static uint8_t g_request_pending;
static uint8_t g_emark_identity_pending;
static uint8_t g_emark_message_id;
static int32_t g_requested_mv;
static int32_t g_requested_ma;
static uint32_t g_pdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_pdo_positions[SERVICE_PD_MAX_PDOS];
static uint8_t g_pdo_count;
static uint32_t g_pps_apdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_pps_apdo_positions[SERVICE_PD_MAX_PDOS];
static uint8_t g_pps_apdo_count;
static uint32_t g_state_elapsed_ms;

static uint8_t service_pd_pdo_is_fixed(uint32_t pdo_word);
static uint8_t service_pd_pdo_is_pps_apdo(uint32_t pdo_word);
static int32_t service_pd_pdo_voltage_mv(uint32_t pdo_word);
static int32_t service_pd_pdo_current_ma(uint32_t pdo_word);
static int32_t service_pd_pps_min_mv(uint32_t apdo_word);
static int32_t service_pd_pps_max_mv(uint32_t apdo_word);
static int32_t service_pd_pps_max_ma(uint32_t apdo_word);
static int32_t service_pd_limit_current_by_cable_ma(int32_t source_ma);
static uint8_t service_pd_voltage_reached(int32_t measured_mv, int32_t target_mv);

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
        g_protocol_snapshot.emark_usb_speed_grade = 0U;
        g_protocol_snapshot.emark_cable_type = 0U;
        return;
    }

    g_protocol_snapshot.emark_present = 1U;
    g_protocol_snapshot.emark_current_a = (uint8_t)g_emark_summary.current_capacity_a;
    g_protocol_snapshot.emark_usb_speed_grade = g_emark_summary.usb_speed_grade;
    g_protocol_snapshot.emark_cable_type = g_emark_summary.cable_type;
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
    g_emark_identity_pending = 0U;
    g_emark_message_id = 0U;
    g_requested_mv = 0;
    g_requested_ma = 0;
    g_pdo_count = 0U;
    g_pps_apdo_count = 0U;
    g_state_elapsed_ms = 0U;
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
    return (uint8_t)(((pdo_word >> 30) & 0x03U) == 0U);
}

static uint8_t service_pd_pdo_is_pps_apdo(uint32_t pdo_word)
{
    return (uint8_t)((((pdo_word >> 30) & 0x03U) == 0x03U) &&
                     (((pdo_word >> 28) & 0x03U) == 0x00U));
}

static int32_t service_pd_pdo_voltage_mv(uint32_t pdo_word)
{
    return (int32_t)(((pdo_word >> 10) & 0x03FFU) * 50U);
}

static int32_t service_pd_pdo_current_ma(uint32_t pdo_word)
{
    return (int32_t)((pdo_word & 0x03FFU) * 10U);
}

static int32_t service_pd_pps_min_mv(uint32_t apdo_word)
{
    return (int32_t)(((apdo_word >> 8) & 0xFFU) * 100U);
}

static int32_t service_pd_pps_max_mv(uint32_t apdo_word)
{
    return (int32_t)(((apdo_word >> 17) & 0xFFU) * 100U);
}

static int32_t service_pd_pps_max_ma(uint32_t apdo_word)
{
    return (int32_t)((apdo_word & 0x7FU) * 50U);
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

static void service_pd_prepare_fixed_request_packet(uint8_t pdo_index,
                                                    uint8_t *tx_packet,
                                                    uint8_t *tx_length)
{
    uint32_t selected_pdo;
    uint8_t object_position;
    uint32_t current_units;
    uint32_t request_word;

    selected_pdo = g_pdo_words[pdo_index - 1U];
    object_position = g_pdo_positions[pdo_index - 1U];
    g_selected_pdo_index = object_position;
    g_requested_mv = service_pd_pdo_voltage_mv(selected_pdo);
    g_requested_ma = service_pd_limit_current_by_cable_ma(service_pd_pdo_current_ma(selected_pdo));
    current_units = (uint32_t)(g_requested_ma / 10);

    tx_packet[0] = 0x80U | DEF_TYPE_REQUEST;
    tx_packet[1] = (uint8_t)((g_message_id & 0x0EU) | 0x10U);

    request_word = ((uint32_t)object_position << 28) |
                   (1UL << 25) |
                   (1UL << 24) |
                   (current_units << 10) |
                   current_units;
    service_pd_put_u32_le(&tx_packet[2], request_word);

    g_message_id = (uint8_t)((g_message_id + 2U) & 0x0EU);
    g_pd_state = SERVICE_PD_STATE_WAIT_ACCEPT;
    g_state_elapsed_ms = 0U;
    service_pd_mark_request_state(PROTOCOL_REQUEST_REQUESTING);
    *tx_length = 6U;
}

static void service_pd_prepare_pps_request_packet(uint8_t pps_index,
                                                  uint8_t *tx_packet,
                                                  uint8_t *tx_length)
{
    uint32_t selected_apdo;
    uint8_t object_position;
    uint32_t voltage_units;
    uint32_t current_units;
    uint32_t request_word;

    selected_apdo = g_pps_apdo_words[pps_index - 1U];
    object_position = g_pps_apdo_positions[pps_index - 1U];
    g_selected_pdo_index = object_position;
    g_requested_mv = g_preferred_voltage_mv;
    g_requested_ma = service_pd_limit_current_by_cable_ma(service_pd_pps_max_ma(selected_apdo));

    voltage_units = (uint32_t)(g_requested_mv / 20);
    current_units = (uint32_t)(g_requested_ma / 50);
    if (current_units > 0x7FU)
    {
        current_units = 0x7FU;
        g_requested_ma = (int32_t)(current_units * 50U);
    }

    tx_packet[0] = 0x80U | DEF_TYPE_REQUEST;
    tx_packet[1] = (uint8_t)((g_message_id & 0x0EU) | 0x10U);

    request_word = ((uint32_t)object_position << 28) |
                   (1UL << 25) |
                   (1UL << 24) |
                   ((voltage_units & 0x0FFFUL) << 9) |
                   (current_units & 0x7FUL);
    service_pd_put_u32_le(&tx_packet[2], request_word);

    g_message_id = (uint8_t)((g_message_id + 2U) & 0x0EU);
    g_pd_state = SERVICE_PD_STATE_WAIT_ACCEPT;
    g_state_elapsed_ms = 0U;
    service_pd_mark_request_state(PROTOCOL_REQUEST_REQUESTING);
    *tx_length = 6U;
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
}

void service_pd_handle_detach(void)
{
    service_pd_reset_runtime(1U);
}

void service_pd_handle_timeout_ms(uint32_t elapsed_ms)
{
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

void service_pd_set_preferred_voltage_mv(int32_t target_mv)
{
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

uint8_t service_pd_prepare_pending_request(uint8_t *tx_packet, uint8_t *tx_length)
{
    uint8_t request_kind;
    uint8_t selected_index;

    if (tx_length != NULL)
    {
        *tx_length = 0U;
    }

    if ((tx_packet == NULL) || (tx_length == NULL) ||
        (g_request_pending == 0U) || ((g_pdo_count == 0U) && (g_pps_apdo_count == 0U)) ||
        (service_pd_request_is_in_flight() != 0U))
    {
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
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
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
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
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
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
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
    snapshot->emark_usb_speed_grade = 0U;
    snapshot->emark_cable_type = 0U;
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

            g_pdo_count = 0U;
            g_pps_apdo_count = 0U;
            memset(g_pdo_words, 0, sizeof(g_pdo_words));
            memset(g_pdo_positions, 0, sizeof(g_pdo_positions));
            memset(g_pps_apdo_words, 0, sizeof(g_pps_apdo_words));
            memset(g_pps_apdo_positions, 0, sizeof(g_pps_apdo_positions));
            for (index = 0U; (index < ndo) && (index < SERVICE_PD_MAX_PDOS); ++index)
            {
                uint32_t pdo_word;

                pdo_word = service_pd_get_u32_le(&packet[2U + (index * 4U)]);
                if (service_pd_pdo_is_pps_apdo(pdo_word) != 0U)
                {
                    g_pps_apdo_positions[g_pps_apdo_count] = (uint8_t)(index + 1U);
                    g_pps_apdo_words[g_pps_apdo_count++] = pdo_word;
                    continue;
                }

                if (service_pd_pdo_is_fixed(pdo_word) != 0U)
                {
                    g_pdo_positions[g_pdo_count] = (uint8_t)(index + 1U);
                    g_pdo_words[g_pdo_count++] = pdo_word;
                }
            }

            service_pd_copy_capabilities_to_snapshot();
            if (service_pd_request_is_in_flight() != 0U)
            {
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

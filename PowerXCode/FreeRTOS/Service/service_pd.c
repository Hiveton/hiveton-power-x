#include "service_pd.h"

#include <stddef.h>
#include <string.h>

#include "service_emark.h"
#if defined(__riscv)
#include "ch32l103_usbpd.h"
#else
#define DEF_TYPE_REQUEST 0x02U
#endif

#define SERVICE_PD_MAX_PDOS 7U
#define SERVICE_PD_DEFAULT_TARGET_MV 5000
#define SERVICE_PD_TIMEOUT_MS 500U

typedef enum
{
    SERVICE_PD_STATE_DETACHED = 0,
    SERVICE_PD_STATE_WAIT_SRC_CAP,
    SERVICE_PD_STATE_WAIT_ACCEPT,
    SERVICE_PD_STATE_WAIT_PS_RDY,
    SERVICE_PD_STATE_CONTRACT_READY,
} service_pd_state_t;

static protocol_snapshot_t g_protocol_snapshot;
static emark_summary_t g_emark_summary;
static service_pd_state_t g_pd_state;
static int32_t g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
static uint8_t g_message_id;
static uint8_t g_selected_pdo_index;
static int32_t g_requested_mv;
static int32_t g_requested_ma;
static uint32_t g_pdo_words[SERVICE_PD_MAX_PDOS];
static uint8_t g_pdo_count;
static uint32_t g_state_elapsed_ms;

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
    g_requested_mv = 0;
    g_requested_ma = 0;
    g_pdo_count = 0U;
    g_state_elapsed_ms = 0U;
    memset(g_pdo_words, 0, sizeof(g_pdo_words));
    if (reset_preference != 0U)
    {
        g_preferred_voltage_mv = SERVICE_PD_DEFAULT_TARGET_MV;
    }
}

static uint8_t service_pd_pdo_is_fixed(uint32_t pdo_word)
{
    return (uint8_t)(((pdo_word >> 30) & 0x03U) == 0U);
}

static int32_t service_pd_pdo_voltage_mv(uint32_t pdo_word)
{
    return (int32_t)(((pdo_word >> 10) & 0x03FFU) * 50U);
}

static int32_t service_pd_pdo_current_ma(uint32_t pdo_word)
{
    return (int32_t)((pdo_word & 0x03FFU) * 10U);
}

static uint8_t service_pd_select_pdo(void)
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

static void service_pd_prepare_request_packet(uint8_t pdo_index,
                                              uint8_t *tx_packet,
                                              uint8_t *tx_length)
{
    uint32_t selected_pdo;
    uint32_t current_units;
    uint32_t request_word;

    selected_pdo = g_pdo_words[pdo_index - 1U];
    g_selected_pdo_index = pdo_index;
    g_requested_mv = service_pd_pdo_voltage_mv(selected_pdo);
    g_requested_ma = service_pd_pdo_current_ma(selected_pdo);
    current_units = (uint32_t)(g_requested_ma / 10);

    tx_packet[0] = 0x80U | DEF_TYPE_REQUEST;
    tx_packet[1] = (uint8_t)((g_message_id & 0x0EU) | 0x10U);

    request_word = ((uint32_t)pdo_index << 28) |
                   (1UL << 25) |
                   (1UL << 24) |
                   (current_units << 10) |
                   current_units;
    service_pd_put_u32_le(&tx_packet[2], request_word);

    g_message_id = (uint8_t)((g_message_id + 2U) & 0x0EU);
    g_pd_state = SERVICE_PD_STATE_WAIT_ACCEPT;
    g_state_elapsed_ms = 0U;
    *tx_length = 6U;
}

static void service_pd_update_contract(uint8_t attached)
{
    if ((attached == 0U) || (g_selected_pdo_index == 0U) || (g_requested_mv <= 0) || (g_requested_ma <= 0))
    {
        protocol_snapshot_reset(&g_protocol_snapshot);
        g_protocol_snapshot.emark_present = g_emark_summary.present;
        return;
    }

    protocol_snapshot_set_pd(&g_protocol_snapshot,
                             g_requested_mv,
                             g_requested_ma,
                             g_emark_summary.present);
}

void service_pd_init(void)
{
    service_pd_reset_runtime(1U);
}

void service_pd_handle_detach(void)
{
    service_pd_reset_runtime(0U);
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
        service_pd_handle_detach();
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
    snapshot->legacy_step_offset = 0;
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
    snapshot->legacy_step_offset = 0;
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
    snapshot->legacy_step_offset = step_offset;
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
                }
                break;
            case DEF_TYPE_PS_RDY:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY)
                {
                    g_pd_state = SERVICE_PD_STATE_CONTRACT_READY;
                    g_state_elapsed_ms = 0U;
                    service_pd_update_contract(1U);
                }
                break;
            case DEF_TYPE_REJECT:
            case DEF_TYPE_WAIT:
            case DEF_TYPE_SOFT_RESET:
                service_pd_handle_detach();
                break;
#else
            case 0x03U:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_ACCEPT)
                {
                    g_pd_state = SERVICE_PD_STATE_WAIT_PS_RDY;
                    g_state_elapsed_ms = 0U;
                }
                break;
            case 0x06U:
                if (g_pd_state == SERVICE_PD_STATE_WAIT_PS_RDY)
                {
                    g_pd_state = SERVICE_PD_STATE_CONTRACT_READY;
                    g_state_elapsed_ms = 0U;
                    service_pd_update_contract(1U);
                }
                break;
            case 0x04U:
            case 0x0CU:
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

            g_pdo_count = 0U;
            memset(g_pdo_words, 0, sizeof(g_pdo_words));
            for (index = 0U; (index < ndo) && (index < SERVICE_PD_MAX_PDOS); ++index)
            {
                uint32_t pdo_word;

                pdo_word = service_pd_get_u32_le(&packet[2U + (index * 4U)]);
                if ((pdo_word & 0xC0000000UL) == 0xC0000000UL)
                {
                    break;
                }

                g_pdo_words[g_pdo_count++] = pdo_word;
            }

            if (g_pdo_count != 0U)
            {
                uint8_t pdo_index;

                pdo_index = service_pd_select_pdo();
                if (pdo_index != 0U)
                {
                    service_pd_prepare_request_packet(pdo_index, tx_packet, tx_length);
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
                    g_protocol_snapshot.emark_present = g_emark_summary.present;
                }
            }
            break;
        }
        default:
            break;
    }

    return 0U;
}

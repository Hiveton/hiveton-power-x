#include "service_emark.h"

#include <stddef.h>

#define SERVICE_EMARK_VDO_USB_SPEED_MASK 0x07UL
#define SERVICE_EMARK_VDO_CURRENT_SHIFT 5U
#define SERVICE_EMARK_VDO_CURRENT_MASK 0x03UL
#define SERVICE_EMARK_VDO_MAX_VOLTAGE_SHIFT 9U
#define SERVICE_EMARK_VDO_MAX_VOLTAGE_MASK 0x03UL
#define SERVICE_EMARK_VDO_LATENCY_SHIFT 13U
#define SERVICE_EMARK_VDO_LATENCY_MASK 0x0FUL
#define SERVICE_EMARK_VDO_EPR_SHIFT 17U
#define SERVICE_EMARK_VDO_CONNECTOR_SHIFT 18U
#define SERVICE_EMARK_VDO_CONNECTOR_MASK 0x03UL
#define SERVICE_EMARK_VDO_VERSION_SHIFT 21U
#define SERVICE_EMARK_VDO_VERSION_MASK 0x07UL
#define SERVICE_EMARK_VDO_FW_SHIFT 24U
#define SERVICE_EMARK_VDO_HW_SHIFT 28U
#define SERVICE_EMARK_VDO_NIBBLE_MASK 0x0FUL
#define SERVICE_EMARK_CURRENT_3A 1U
#define SERVICE_EMARK_CURRENT_5A 2U
#define SERVICE_EMARK_CABLE_VDO_INDEX 3U

void service_emark_reset(emark_summary_t *summary)
{
    if (summary == NULL)
    {
        return;
    }

    *summary = (emark_summary_t){ 0 };
}

static uint8_t service_emark_current_to_amps(uint32_t current_bits)
{
    switch (current_bits)
    {
        case SERVICE_EMARK_CURRENT_3A:
            return 3U;
        case SERVICE_EMARK_CURRENT_5A:
            return 5U;
        default:
            return 0U;
    }
}

static uint8_t service_emark_voltage_to_volts(uint32_t voltage_bits)
{
    static const uint8_t voltages[] = { 20U, 30U, 40U, 50U };

    return voltages[voltage_bits & SERVICE_EMARK_VDO_MAX_VOLTAGE_MASK];
}

static uint8_t service_emark_latency_to_length_m(uint32_t latency_bits)
{
    if ((latency_bits == 0U) || (latency_bits > 8U))
    {
        return 0U;
    }

    return (uint8_t)latency_bits;
}

uint8_t service_emark_summarize_identity(const uint32_t *identity_vdos,
                                         uint8_t vdo_count,
                                         emark_summary_t *summary)
{
    if (summary == NULL)
    {
        return 0U;
    }

    service_emark_reset(summary);
    if ((identity_vdos == NULL) || (vdo_count <= SERVICE_EMARK_CABLE_VDO_INDEX))
    {
        return 0U;
    }

    {
        uint32_t vdo;
        uint32_t current_bits;
        uint32_t voltage_bits;
        uint32_t latency_bits;
        uint8_t current_a;

        vdo = identity_vdos[SERVICE_EMARK_CABLE_VDO_INDEX];
        current_bits = (vdo >> SERVICE_EMARK_VDO_CURRENT_SHIFT) &
                       SERVICE_EMARK_VDO_CURRENT_MASK;
        voltage_bits = (vdo >> SERVICE_EMARK_VDO_MAX_VOLTAGE_SHIFT) &
                       SERVICE_EMARK_VDO_MAX_VOLTAGE_MASK;
        latency_bits = (vdo >> SERVICE_EMARK_VDO_LATENCY_SHIFT) &
                       SERVICE_EMARK_VDO_LATENCY_MASK;
        current_a = service_emark_current_to_amps(current_bits);
        if (current_a == 0U)
        {
            return 0U;
        }

        summary->present = 1U;
        summary->current_capacity_a = current_a;
        summary->max_voltage_v = service_emark_voltage_to_volts(voltage_bits);
        summary->cable_length_m = service_emark_latency_to_length_m(latency_bits);
        summary->epr_capable = (uint8_t)((vdo >> SERVICE_EMARK_VDO_EPR_SHIFT) & 0x01UL);
        summary->usb_speed_grade = (uint8_t)(vdo & SERVICE_EMARK_VDO_USB_SPEED_MASK);
        summary->cable_type = (uint8_t)((vdo >> SERVICE_EMARK_VDO_CONNECTOR_SHIFT) &
                                        SERVICE_EMARK_VDO_CONNECTOR_MASK);
        summary->vdo_version = (uint8_t)((vdo >> SERVICE_EMARK_VDO_VERSION_SHIFT) &
                                         SERVICE_EMARK_VDO_VERSION_MASK);
        summary->firmware_version = (uint8_t)((vdo >> SERVICE_EMARK_VDO_FW_SHIFT) &
                                              SERVICE_EMARK_VDO_NIBBLE_MASK);
        summary->hardware_version = (uint8_t)((vdo >> SERVICE_EMARK_VDO_HW_SHIFT) &
                                             SERVICE_EMARK_VDO_NIBBLE_MASK);
        return 1U;
    }
}

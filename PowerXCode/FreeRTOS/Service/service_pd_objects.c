#include "service_pd_objects.h"

#include <stddef.h>

static uint32_t pd_object_clamp_u32(int32_t value, uint32_t max_value)
{
    if (value <= 0)
    {
        return 0U;
    }

    if ((uint32_t)value > max_value)
    {
        return max_value;
    }

    return (uint32_t)value;
}

uint8_t pd_object_parse_source_pdo(uint8_t position, uint32_t raw, pd_object_t *object)
{
    uint8_t type;

    if (object == NULL)
    {
        return 0U;
    }

    *object = (pd_object_t){ 0 };
    object->raw = raw;
    object->position = position;
    type = (uint8_t)((raw >> 30) & 0x03U);
    object->type = (pd_object_type_t)type;
    object->apdo_subtype = PD_APDO_SUBTYPE_RESERVED;

    if (type == (uint8_t)PD_OBJECT_TYPE_FIXED)
    {
        object->fixed.voltage_mv = (int32_t)(((raw >> 10) & 0x03FFU) * 50U);
        object->fixed.current_ma = (int32_t)((raw & 0x03FFU) * 10U);
        object->fixed.epr_capable = ((raw & (1UL << 17)) != 0U) ? 1U : 0U;
        return 1U;
    }

    if (type == (uint8_t)PD_OBJECT_TYPE_BATTERY)
    {
        object->battery.max_mv = (int32_t)(((raw >> 20) & 0x03FFU) * 50U);
        object->battery.min_mv = (int32_t)(((raw >> 10) & 0x03FFU) * 50U);
        object->battery.power_mw = (int32_t)((raw & 0x03FFU) * 250U);
        return 1U;
    }

    if (type == (uint8_t)PD_OBJECT_TYPE_VARIABLE)
    {
        object->variable.max_mv = (int32_t)(((raw >> 20) & 0x03FFU) * 50U);
        object->variable.min_mv = (int32_t)(((raw >> 10) & 0x03FFU) * 50U);
        object->variable.current_ma = (int32_t)((raw & 0x03FFU) * 10U);
        return 1U;
    }

    if (type == (uint8_t)PD_OBJECT_TYPE_APDO)
    {
        uint8_t subtype;

        subtype = (uint8_t)((raw >> 28) & 0x03U);
        object->apdo_subtype = (pd_apdo_subtype_t)subtype;
        if (subtype == (uint8_t)PD_APDO_SUBTYPE_SPR_PPS)
        {
            object->pps.current_ma = (int32_t)((raw & 0x7FU) * 50U);
            object->pps.min_mv = (int32_t)(((raw >> 8) & 0xFFU) * 100U);
            object->pps.max_mv = (int32_t)(((raw >> 17) & 0xFFU) * 100U);
        }
        else if (subtype == (uint8_t)PD_APDO_SUBTYPE_SPR_AVS)
        {
            object->spr_avs.current_15v_20v_ma = (int32_t)((raw & 0x03FFU) * 10U);
            object->spr_avs.current_9v_15v_ma = (int32_t)(((raw >> 10) & 0x03FFU) * 10U);
        }
        else if (subtype == (uint8_t)PD_APDO_SUBTYPE_EPR_AVS)
        {
            object->epr_avs.pdp_w = (int32_t)(raw & 0xFFU);
            object->epr_avs.min_mv = (int32_t)(((raw >> 8) & 0xFFU) * 100U);
            object->epr_avs.max_mv = (int32_t)(((raw >> 17) & 0x1FFU) * 100U);
        }
        return 1U;
    }

    return 1U;
}

uint8_t pd_object_is_fixed(const pd_object_t *object)
{
    return ((object != NULL) && (object->type == PD_OBJECT_TYPE_FIXED)) ? 1U : 0U;
}

uint8_t pd_object_is_pps(const pd_object_t *object)
{
    return ((object != NULL) &&
            (object->type == PD_OBJECT_TYPE_APDO) &&
            (object->apdo_subtype == PD_APDO_SUBTYPE_SPR_PPS)) ? 1U : 0U;
}

uint8_t pd_object_build_fixed_rdo(const pd_object_t *object, int32_t operating_ma, uint32_t *rdo)
{
    uint32_t current_units;

    if ((object == NULL) || (rdo == NULL) || (pd_object_is_fixed(object) == 0U))
    {
        return 0U;
    }

    current_units = pd_object_clamp_u32(operating_ma / 10, 0x03FFU);
    *rdo = ((uint32_t)object->position << 28) |
           (1UL << 25) |
           (1UL << 24) |
           (current_units << 10) |
           current_units;
    return 1U;
}

uint8_t pd_object_build_pps_rdo(const pd_object_t *object, int32_t voltage_mv, int32_t operating_ma, uint32_t *rdo)
{
    uint32_t voltage_units;
    uint32_t current_units;

    if ((object == NULL) || (rdo == NULL) || (pd_object_is_pps(object) == 0U))
    {
        return 0U;
    }

    if ((voltage_mv < object->pps.min_mv) || (voltage_mv > object->pps.max_mv))
    {
        return 0U;
    }

    voltage_units = pd_object_clamp_u32(voltage_mv / 20, 0x0FFFU);
    current_units = pd_object_clamp_u32(operating_ma / 50, 0x7FU);
    *rdo = ((uint32_t)object->position << 28) |
           (1UL << 25) |
           (1UL << 24) |
           ((voltage_units & 0x0FFFUL) << 9) |
           (current_units & 0x7FUL);
    return 1U;
}

uint8_t pd_object_build_avs_rdo(const pd_object_t *object, int32_t voltage_mv, int32_t operating_ma, uint32_t *rdo)
{
    uint32_t voltage_units;
    uint32_t current_units;

    if ((object == NULL) || (rdo == NULL) ||
        (object->type != PD_OBJECT_TYPE_APDO) ||
        ((object->apdo_subtype != PD_APDO_SUBTYPE_SPR_AVS) &&
         (object->apdo_subtype != PD_APDO_SUBTYPE_EPR_AVS)))
    {
        return 0U;
    }

    voltage_units = pd_object_clamp_u32(voltage_mv / 25, 0x1FFFU) & ~0x03UL;
    current_units = pd_object_clamp_u32(operating_ma / 50, 0x7FU);
    *rdo = ((uint32_t)object->position << 28) |
           (1UL << 25) |
           (1UL << 24) |
           ((voltage_units & 0x1FFFUL) << 9) |
           (current_units & 0x7FUL);
    return 1U;
}

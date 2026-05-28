#include "service_charge_protocols.h"

#include <stddef.h>

#define CHARGE_QC2_MIN_MV 5000
#define CHARGE_QC2_DEFAULT_MV 5000
#define CHARGE_QC2_9V_MV 9000
#define CHARGE_QC2_12V_MV 12000
#define CHARGE_QC2_20V_MV 20000

int32_t charge_protocol_qc2_quantize_voltage_mv(int32_t target_mv)
{
    if (target_mv < CHARGE_QC2_MIN_MV)
    {
        target_mv = CHARGE_QC2_MIN_MV;
    }
    if (target_mv > CHARGE_QC2_20V_MV)
    {
        target_mv = CHARGE_QC2_20V_MV;
    }

    if (target_mv >= 19000)
    {
        return CHARGE_QC2_20V_MV;
    }
    if (target_mv >= 11500)
    {
        return CHARGE_QC2_12V_MV;
    }
    if (target_mv >= 8500)
    {
        return CHARGE_QC2_9V_MV;
    }

    return CHARGE_QC2_DEFAULT_MV;
}

uint8_t charge_protocol_qc2_levels_for_voltage_mv(int32_t target_mv,
                                                  bsp_dpdm_level_t *dp_level,
                                                  bsp_dpdm_level_t *dm_level,
                                                  int32_t *actual_mv)
{
    int32_t quantized_mv;
    bsp_dpdm_level_t dp;
    bsp_dpdm_level_t dm;

    quantized_mv = charge_protocol_qc2_quantize_voltage_mv(target_mv);
    switch (quantized_mv)
    {
        case CHARGE_QC2_20V_MV:
            dp = BSP_DPDM_LEVEL_3300MV;
            dm = BSP_DPDM_LEVEL_3300MV;
            break;
        case CHARGE_QC2_12V_MV:
            dp = BSP_DPDM_LEVEL_600MV;
            dm = BSP_DPDM_LEVEL_600MV;
            break;
        case CHARGE_QC2_9V_MV:
            dp = BSP_DPDM_LEVEL_3300MV;
            dm = BSP_DPDM_LEVEL_600MV;
            break;
        case CHARGE_QC2_DEFAULT_MV:
        default:
            dp = BSP_DPDM_LEVEL_600MV;
            dm = BSP_DPDM_LEVEL_LOW;
            break;
    }

    if (dp_level != NULL)
    {
        *dp_level = dp;
    }
    if (dm_level != NULL)
    {
        *dm_level = dm;
    }
    if (actual_mv != NULL)
    {
        *actual_mv = quantized_mv;
    }

    return 1U;
}

int8_t charge_protocol_qc3_offset_for_voltage_mv(int32_t target_mv,
                                                 int32_t base_mv,
                                                 int8_t min_offset,
                                                 int8_t max_offset)
{
    int32_t min_mv;
    int32_t max_mv;
    int32_t offset;

    min_mv = base_mv + ((int32_t)min_offset * 200);
    max_mv = base_mv + ((int32_t)max_offset * 200);

    if (target_mv < min_mv)
    {
        target_mv = min_mv;
    }
    if (target_mv > max_mv)
    {
        target_mv = max_mv;
    }

    offset = target_mv - base_mv;
    if (offset >= 0)
    {
        offset = (offset + 100) / 200;
    }
    else
    {
        offset = (offset - 100) / 200;
    }

    if (offset < min_offset)
    {
        offset = min_offset;
    }
    if (offset > max_offset)
    {
        offset = max_offset;
    }

    return (int8_t)offset;
}

charge_bc12_type_t charge_protocol_bc12_type_from_probe(uint8_t dm_high_after_dp_src,
                                                        uint8_t dp_high_after_dm_src)
{
    if (dm_high_after_dp_src == 0U)
    {
        return CHARGE_BC12_TYPE_SDP;
    }

    if (dp_high_after_dm_src == 0U)
    {
        return CHARGE_BC12_TYPE_CDP;
    }

    return CHARGE_BC12_TYPE_DCP;
}

#ifndef SERVICE_CHARGE_PROTOCOLS_H
#define SERVICE_CHARGE_PROTOCOLS_H

#include <stdint.h>

#include "bsp_dpdm.h"

typedef enum
{
    CHARGE_BC12_TYPE_UNKNOWN = 0,
    CHARGE_BC12_TYPE_SDP,
    CHARGE_BC12_TYPE_CDP,
    CHARGE_BC12_TYPE_DCP
} charge_bc12_type_t;

uint8_t charge_protocol_qc2_levels_for_voltage_mv(int32_t target_mv,
                                                  bsp_dpdm_level_t *dp_level,
                                                  bsp_dpdm_level_t *dm_level,
                                                  int32_t *actual_mv);
int32_t charge_protocol_qc2_quantize_voltage_mv(int32_t target_mv);
int8_t charge_protocol_qc3_offset_for_voltage_mv(int32_t target_mv,
                                                 int32_t base_mv,
                                                 int8_t min_offset,
                                                 int8_t max_offset);
charge_bc12_type_t charge_protocol_bc12_type_from_probe(uint8_t dm_high_after_dp_src,
                                                        uint8_t dp_high_after_dm_src);

#endif /* SERVICE_CHARGE_PROTOCOLS_H */

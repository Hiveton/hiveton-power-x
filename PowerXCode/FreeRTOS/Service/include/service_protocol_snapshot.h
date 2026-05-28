#ifndef SERVICE_PROTOCOL_SNAPSHOT_H
#define SERVICE_PROTOCOL_SNAPSHOT_H

#include <stdint.h>

#define PROTOCOL_MAX_FIXED_PDOS 7U

typedef enum
{
    PROTOCOL_KIND_NONE = 0,
    PROTOCOL_KIND_PD,
    PROTOCOL_KIND_QC,
    PROTOCOL_KIND_AFC,
    PROTOCOL_KIND_FCP,
    PROTOCOL_KIND_OTHER,
} protocol_kind_t;

typedef enum
{
    PROTOCOL_REQUEST_IDLE = 0,
    PROTOCOL_REQUEST_AVAILABLE,
    PROTOCOL_REQUEST_REQUESTING,
    PROTOCOL_REQUEST_ACCEPTED,
    PROTOCOL_REQUEST_READY,
    PROTOCOL_REQUEST_FAILED,
} protocol_request_state_t;

typedef struct
{
    protocol_kind_t kind;
    int32_t contract_mv;
    int32_t contract_ma;
    uint8_t emark_present;
    uint8_t emark_current_a;
    uint8_t emark_max_voltage_v;
    uint8_t emark_cable_length_m;
    uint8_t emark_epr_capable;
    uint8_t emark_usb_speed_grade;
    uint8_t emark_cable_type;
    uint8_t emark_vdo_version;
    uint8_t emark_firmware_version;
    uint8_t emark_hardware_version;
    int8_t legacy_step_offset;
    protocol_request_state_t request_state;
    int32_t target_mv;
    uint8_t selected_pdo_index;
    uint8_t source_fixed_count;
    int32_t source_fixed_mv[PROTOCOL_MAX_FIXED_PDOS];
    int32_t source_fixed_ma[PROTOCOL_MAX_FIXED_PDOS];
    int16_t dp_mv;
    int16_t dm_mv;
    uint8_t cc_orientation;
    uint8_t cc_attached;
    uint8_t cc_active_mask;
    uint8_t pps_present;
    int32_t pps_min_mv;
    int32_t pps_max_mv;
    int32_t pps_max_ma;
} protocol_snapshot_t;

void protocol_snapshot_reset(protocol_snapshot_t *snapshot);
void protocol_snapshot_set_pd(protocol_snapshot_t *snapshot,
                              int32_t contract_mv,
                              int32_t contract_ma,
                              uint8_t emark_present);
void protocol_snapshot_set_legacy(protocol_snapshot_t *snapshot,
                                  protocol_kind_t kind,
                                  int32_t target_mv,
                                  int8_t step_offset);
void protocol_snapshot_set_other(protocol_snapshot_t *snapshot);
void protocol_snapshot_set_cc_orientation(protocol_snapshot_t *snapshot, uint8_t cc_orientation);

#endif /* SERVICE_PROTOCOL_SNAPSHOT_H */

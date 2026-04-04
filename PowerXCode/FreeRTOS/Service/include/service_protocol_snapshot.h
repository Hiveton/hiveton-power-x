#ifndef SERVICE_PROTOCOL_SNAPSHOT_H
#define SERVICE_PROTOCOL_SNAPSHOT_H

#include <stdint.h>

typedef enum
{
    PROTOCOL_KIND_NONE = 0,
    PROTOCOL_KIND_PD,
    PROTOCOL_KIND_QC,
    PROTOCOL_KIND_AFC,
    PROTOCOL_KIND_FCP,
} protocol_kind_t;

typedef struct
{
    protocol_kind_t kind;
    int32_t contract_mv;
    int32_t contract_ma;
    uint8_t emark_present;
    int8_t legacy_step_offset;
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

#endif /* SERVICE_PROTOCOL_SNAPSHOT_H */

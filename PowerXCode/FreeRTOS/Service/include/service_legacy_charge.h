#ifndef SERVICE_LEGACY_CHARGE_H
#define SERVICE_LEGACY_CHARGE_H

#include <stdint.h>

#include "service_protocol_snapshot.h"

typedef enum
{
    LEGACY_PROTOCOL_NONE = 0,
    LEGACY_PROTOCOL_QC2,
    LEGACY_PROTOCOL_QC3,
    LEGACY_PROTOCOL_AFC,
    LEGACY_PROTOCOL_FCP
} legacy_protocol_t;

typedef enum
{
    LEGACY_CHARGE_STATUS_IDLE = 0,
    LEGACY_CHARGE_STATUS_AVAILABLE,
    LEGACY_CHARGE_STATUS_REQUESTING,
    LEGACY_CHARGE_STATUS_READY,
    LEGACY_CHARGE_STATUS_FAILED
} legacy_charge_status_t;

typedef struct
{
    legacy_protocol_t protocol;
    int32_t target_mv;
    int8_t qc3_step_offset;
    legacy_charge_status_t status;
    int16_t dp_mv;
    int16_t dm_mv;
} legacy_charge_request_t;

void service_legacy_charge_init(void);
void service_legacy_charge_request_protocol(legacy_protocol_t protocol);
void service_legacy_charge_request_voltage_mv(legacy_protocol_t protocol, int32_t target_mv);
void service_legacy_charge_request_qc3_step(int8_t step_delta);
legacy_protocol_t service_legacy_charge_detect(void);
uint8_t service_legacy_charge_poll(int32_t measured_vbus_mv,
                                   uint32_t elapsed_ms,
                                   protocol_snapshot_t *snapshot);
void service_legacy_charge_copy_request(legacy_charge_request_t *request);

#endif /* SERVICE_LEGACY_CHARGE_H */

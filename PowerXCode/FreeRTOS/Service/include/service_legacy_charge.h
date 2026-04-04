#ifndef SERVICE_LEGACY_CHARGE_H
#define SERVICE_LEGACY_CHARGE_H

#include <stdint.h>

typedef enum
{
    LEGACY_PROTOCOL_NONE = 0,
    LEGACY_PROTOCOL_QC2,
    LEGACY_PROTOCOL_QC3,
    LEGACY_PROTOCOL_AFC,
    LEGACY_PROTOCOL_FCP
} legacy_protocol_t;

typedef struct
{
    legacy_protocol_t protocol;
    int32_t target_mv;
    int8_t qc3_step_offset;
} legacy_charge_request_t;

void service_legacy_charge_init(void);
void service_legacy_charge_request_protocol(legacy_protocol_t protocol);
void service_legacy_charge_request_voltage_mv(legacy_protocol_t protocol, int32_t target_mv);
void service_legacy_charge_request_qc3_step(int8_t step_delta);
legacy_protocol_t service_legacy_charge_detect(void);
void service_legacy_charge_copy_request(legacy_charge_request_t *request);

#endif /* SERVICE_LEGACY_CHARGE_H */

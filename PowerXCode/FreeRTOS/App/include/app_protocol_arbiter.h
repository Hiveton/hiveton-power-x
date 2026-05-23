#ifndef APP_PROTOCOL_ARBITER_H
#define APP_PROTOCOL_ARBITER_H

#include <stdint.h>

#include "service_protocol_snapshot.h"

typedef enum
{
    APP_PROTOCOL_SOURCE_PD = 0,
    APP_PROTOCOL_SOURCE_LEGACY
} app_protocol_source_t;

typedef struct
{
    protocol_snapshot_t pd_snapshot;
    protocol_snapshot_t legacy_snapshot;
    uint8_t pd_ready;
    uint8_t legacy_ready;
    uint8_t pd_was_active;
} app_protocol_arbiter_t;

void app_protocol_arbiter_init(app_protocol_arbiter_t *arbiter);
void app_protocol_arbiter_publish(app_protocol_arbiter_t *arbiter,
                                  app_protocol_source_t source,
                                  const protocol_snapshot_t *snapshot);
void app_protocol_arbiter_copy(const app_protocol_arbiter_t *arbiter,
                               protocol_snapshot_t *snapshot);

#endif /* APP_PROTOCOL_ARBITER_H */

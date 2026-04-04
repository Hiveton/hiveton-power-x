#ifndef APP_TRIGGER_CONTROL_H
#define APP_TRIGGER_CONTROL_H

#include "service_protocol_snapshot.h"
#include "ui_model.h"

void app_trigger_control_apply(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol);

#endif /* APP_TRIGGER_CONTROL_H */

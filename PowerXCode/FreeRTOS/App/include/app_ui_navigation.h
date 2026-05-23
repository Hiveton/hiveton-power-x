#ifndef APP_UI_NAVIGATION_H
#define APP_UI_NAVIGATION_H

#include <stdint.h>

#include "bsp_keys.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"

uint8_t app_ui_navigation_apply(ui_model_state_t *state,
                                const protocol_snapshot_t *protocol,
                                const bsp_keys_event_t *keys);

#endif /* APP_UI_NAVIGATION_H */

#ifndef UI_PAGES_H
#define UI_PAGES_H

#include "service_measure.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"

void ui_pages_draw(const ui_model_state_t *state,
                   const measure_snapshot_t *measure,
                   const protocol_snapshot_t *protocol);

#endif /* UI_PAGES_H */

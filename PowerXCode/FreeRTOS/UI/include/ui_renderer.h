#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include "service_measure.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"

void ui_renderer_init(void);
void ui_renderer_draw_boot_screen(void);
void ui_renderer_draw_placeholder_page(ui_page_t page);
void ui_renderer_draw_main_page(const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol);
void ui_renderer_draw_protocol_page(const protocol_snapshot_t *protocol);
void ui_renderer_draw_trigger_page(const ui_model_state_t *state,
                                   const protocol_snapshot_t *protocol);

#endif /* UI_RENDERER_H */

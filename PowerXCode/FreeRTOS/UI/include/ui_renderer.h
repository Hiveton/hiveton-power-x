#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <stdint.h>

#include "service_measure.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"

void ui_renderer_init(void);
void ui_renderer_draw_boot_screen(void);
void ui_renderer_draw_main_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol);
void ui_renderer_draw_scope_page(const measure_snapshot_t *measure);
void ui_renderer_draw_protocol_page(const measure_snapshot_t *measure,
                                    const protocol_snapshot_t *protocol);
void ui_renderer_draw_trigger_page(const ui_model_state_t *state,
                                   const protocol_snapshot_t *protocol);
void ui_renderer_draw_pdo_page(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol);
void ui_renderer_draw_qc_page(const ui_model_state_t *state,
                              const protocol_snapshot_t *protocol);
void ui_renderer_draw_cc_page(const measure_snapshot_t *measure,
                              const protocol_snapshot_t *protocol);
void ui_renderer_draw_cable_page(const measure_snapshot_t *measure,
                                 const protocol_snapshot_t *protocol);
void ui_renderer_draw_settings_page(const ui_model_state_t *state);
#if (defined(PX1_ENABLE_KEY_DEBUG_RENDERER) && (PX1_ENABLE_KEY_DEBUG_RENDERER != 0)) || defined(PX1_HOST_TEST)
void ui_renderer_draw_key_debug_page(uint8_t raw_high_mask,
                                     uint8_t active_mask,
                                     uint8_t pending_irq_mask,
                                     uint8_t seen_irq_mask,
                                     uint8_t event_mask,
                                     ui_page_t page,
                                     uint16_t event_count,
                                     uint16_t heartbeat);
#endif

#endif /* UI_RENDERER_H */

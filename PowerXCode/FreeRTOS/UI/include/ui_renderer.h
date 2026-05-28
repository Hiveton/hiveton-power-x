#ifndef UI_RENDERER_H
#define UI_RENDERER_H

#include <stdint.h>

#include "service_measure.h"
#include "service_protocol_snapshot.h"
#include "ui_model.h"

void ui_renderer_init(void);
void ui_renderer_begin_frame_stream(void);
void ui_renderer_end_frame_stream(void);
void ui_renderer_update_scope_history(const measure_snapshot_t *measure);
void ui_renderer_update_ripple_history(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure);
void ui_renderer_draw_boot_screen(void);
void ui_renderer_draw_main_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol);
void ui_renderer_draw_dpdm_page(const ui_model_state_t *state,
                                const measure_snapshot_t *measure,
                                const protocol_snapshot_t *protocol);
void ui_renderer_draw_power_stats_page(const ui_model_state_t *state,
                                       const measure_snapshot_t *measure);
void ui_renderer_draw_capacity_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure);
void ui_renderer_draw_scope_page(const measure_snapshot_t *measure);
void ui_renderer_draw_ripple_page(const ui_model_state_t *state,
                                  const measure_snapshot_t *measure);
void ui_renderer_draw_trigger_select_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol);
void ui_renderer_draw_trigger_adjust_page(const ui_model_state_t *state,
                                          const protocol_snapshot_t *protocol);
void ui_renderer_draw_protocol_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure,
                                    const protocol_snapshot_t *protocol);
void ui_renderer_draw_protocol_warning_page(const ui_model_state_t *state);
void ui_renderer_draw_pdo_page(const ui_model_state_t *state,
                               const protocol_snapshot_t *protocol);
void ui_renderer_draw_emark_page(const measure_snapshot_t *measure,
                                 const protocol_snapshot_t *protocol);
void ui_renderer_draw_menu_page(const ui_model_state_t *state);
void ui_renderer_draw_settings_page(const ui_model_state_t *state,
                                    const measure_snapshot_t *measure);
#if defined(PX1_HOST_TEST)
const char *ui_renderer_infer_dpdm_protocol_for_test(const protocol_snapshot_t *protocol);
#endif
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

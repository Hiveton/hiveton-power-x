#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <stdint.h>

typedef enum
{
    UI_PAGE_MAIN = 0,
    UI_PAGE_SCOPE,
    UI_PAGE_PROTOCOL,
    UI_PAGE_TRIGGER,
    UI_PAGE_PDO,
    UI_PAGE_QC,
    UI_PAGE_CC,
    UI_PAGE_CABLE,
    UI_PAGE_SETTINGS
} ui_page_t;

typedef struct
{
    ui_page_t page;
    unsigned char trigger_preset_index;
    unsigned char qc_preset_index;
    unsigned char settings_selected_index;
    unsigned char brightness_index;
    unsigned char rotation_index;
    unsigned char trigger_manual;
    unsigned char action_mode;
    unsigned char liveness_frame;
    uint16_t pdo_target_mv;
} ui_model_state_t;

void ui_model_init(ui_model_state_t *state);
void ui_model_next_page(ui_model_state_t *state);
void ui_model_prev_page(ui_model_state_t *state);
void ui_model_trigger_next(ui_model_state_t *state);
void ui_model_trigger_prev(ui_model_state_t *state);
int ui_model_trigger_selected_mv(const ui_model_state_t *state);
unsigned char ui_model_trigger_selected_index(const ui_model_state_t *state);
unsigned char ui_model_trigger_preset_count(void);
int ui_model_trigger_preset_mv_at(unsigned char index);
int ui_model_pdo_target_mv(const ui_model_state_t *state);
void ui_model_pdo_target_set_mv(ui_model_state_t *state, int target_mv);
void ui_model_pdo_target_next(ui_model_state_t *state);
void ui_model_pdo_target_prev(ui_model_state_t *state);
void ui_model_qc_next(ui_model_state_t *state);
void ui_model_qc_prev(ui_model_state_t *state);
int ui_model_qc_selected_mv(const ui_model_state_t *state);
unsigned char ui_model_qc_selected_index(const ui_model_state_t *state);
unsigned char ui_model_enter_action_mode(ui_model_state_t *state);
void ui_model_exit_action_mode(ui_model_state_t *state);
unsigned char ui_model_action_mode(const ui_model_state_t *state);
unsigned char ui_model_page_is_actionable(ui_page_t page);
void ui_model_settings_next(ui_model_state_t *state);
void ui_model_settings_prev(ui_model_state_t *state);
unsigned char ui_model_settings_selected_index(const ui_model_state_t *state);
void ui_model_settings_activate(ui_model_state_t *state);
uint8_t ui_model_brightness_percent(const ui_model_state_t *state);
uint16_t ui_model_rotation_degrees(const ui_model_state_t *state);
unsigned char ui_model_trigger_manual(const ui_model_state_t *state);
void ui_model_advance_liveness(ui_model_state_t *state);
unsigned char ui_model_liveness_frame(const ui_model_state_t *state);

#endif /* UI_MODEL_H */

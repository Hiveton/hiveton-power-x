#ifndef UI_MODEL_H
#define UI_MODEL_H

#include <stdint.h>

typedef enum
{
    UI_PAGE_MAIN = 0,
    UI_PAGE_DPDM,
    UI_PAGE_POWER_STATS,
    UI_PAGE_CAPACITY,
    UI_PAGE_SCOPE,
    UI_PAGE_RIPPLE,
    UI_PAGE_TRIGGER_SELECT,
    UI_PAGE_TRIGGER_ADJUST,
    UI_PAGE_PROTOCOL_WARNING,
    UI_PAGE_PROTOCOL,
    UI_PAGE_PDO,
    UI_PAGE_EMARK,
    UI_PAGE_MENU,
    UI_PAGE_SETTINGS
} ui_page_t;

typedef struct
{
    ui_page_t page;
    unsigned char menu_selected_index;
    ui_page_t menu_return_page;
    unsigned char settings_selected_index;
    unsigned char brightness_index;
    unsigned char rotation_index;
    unsigned char power_stats_average;
    unsigned char capacity_show_wh;
    unsigned char protocol_warning_confirm;
    unsigned char protocol_scroll;
    unsigned char pdo_scroll;
    unsigned char trigger_selected_index;
    unsigned char trigger_scroll;
    unsigned char ripple_paused;
    unsigned char ripple_frequency_index;
    unsigned char action_mode;
    unsigned char liveness_frame;
    uint16_t pdo_target_mv;
    uint16_t trigger_target_mv;
    uint16_t trigger_min_mv;
    uint16_t trigger_max_mv;
    uint16_t trigger_step_mv;
} ui_model_state_t;

void ui_model_init(ui_model_state_t *state);
void ui_model_next_page(ui_model_state_t *state);
void ui_model_prev_page(ui_model_state_t *state);
void ui_model_open_menu(ui_model_state_t *state);
void ui_model_menu_back(ui_model_state_t *state);
void ui_model_menu_next(ui_model_state_t *state);
void ui_model_menu_prev(ui_model_state_t *state);
unsigned char ui_model_menu_selected_index(const ui_model_state_t *state);
unsigned char ui_model_menu_scroll(const ui_model_state_t *state);
unsigned char ui_model_menu_item_count(void);
void ui_model_menu_activate(ui_model_state_t *state);
int ui_model_pdo_target_mv(const ui_model_state_t *state);
void ui_model_pdo_target_set_mv(ui_model_state_t *state, int target_mv);
void ui_model_pdo_target_next(ui_model_state_t *state);
void ui_model_pdo_target_prev(ui_model_state_t *state);
unsigned char ui_model_enter_action_mode(ui_model_state_t *state);
void ui_model_exit_action_mode(ui_model_state_t *state);
unsigned char ui_model_action_mode(const ui_model_state_t *state);
unsigned char ui_model_page_is_actionable(ui_page_t page);
void ui_model_power_stats_toggle(ui_model_state_t *state);
unsigned char ui_model_power_stats_average(const ui_model_state_t *state);
void ui_model_capacity_toggle(ui_model_state_t *state);
unsigned char ui_model_capacity_show_wh(const ui_model_state_t *state);
void ui_model_protocol_warning_toggle(ui_model_state_t *state);
unsigned char ui_model_protocol_warning_confirm_selected(const ui_model_state_t *state);
void ui_model_protocol_warning_accept(ui_model_state_t *state);
void ui_model_protocol_scroll_next(ui_model_state_t *state);
void ui_model_protocol_scroll_prev(ui_model_state_t *state);
unsigned char ui_model_protocol_scroll(const ui_model_state_t *state);
void ui_model_pdo_scroll_next(ui_model_state_t *state);
void ui_model_pdo_scroll_prev(ui_model_state_t *state);
unsigned char ui_model_pdo_scroll(const ui_model_state_t *state);
void ui_model_open_trigger(ui_model_state_t *state);
void ui_model_trigger_select_next(ui_model_state_t *state, unsigned char count);
void ui_model_trigger_select_prev(ui_model_state_t *state, unsigned char count);
unsigned char ui_model_trigger_selected_index(const ui_model_state_t *state);
unsigned char ui_model_trigger_scroll(const ui_model_state_t *state);
void ui_model_trigger_enter_adjust(ui_model_state_t *state,
                                   int min_mv,
                                   int max_mv,
                                   int step_mv,
                                   int default_mv);
void ui_model_trigger_back_to_select(ui_model_state_t *state);
int ui_model_trigger_target_mv(const ui_model_state_t *state);
int ui_model_trigger_min_mv(const ui_model_state_t *state);
int ui_model_trigger_max_mv(const ui_model_state_t *state);
int ui_model_trigger_step_mv(const ui_model_state_t *state);
void ui_model_trigger_voltage_next(ui_model_state_t *state);
void ui_model_trigger_voltage_prev(ui_model_state_t *state);
void ui_model_ripple_toggle_pause(ui_model_state_t *state);
unsigned char ui_model_ripple_paused(const ui_model_state_t *state);
void ui_model_ripple_frequency_next(ui_model_state_t *state);
void ui_model_ripple_frequency_prev(ui_model_state_t *state);
uint32_t ui_model_ripple_frequency_hz(const ui_model_state_t *state);
void ui_model_settings_next(ui_model_state_t *state);
void ui_model_settings_prev(ui_model_state_t *state);
unsigned char ui_model_settings_selected_index(const ui_model_state_t *state);
void ui_model_settings_activate(ui_model_state_t *state);
uint8_t ui_model_brightness_percent(const ui_model_state_t *state);
uint16_t ui_model_rotation_degrees(const ui_model_state_t *state);
void ui_model_advance_liveness(ui_model_state_t *state);
unsigned char ui_model_liveness_frame(const ui_model_state_t *state);

#endif /* UI_MODEL_H */

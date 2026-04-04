#ifndef UI_MODEL_H
#define UI_MODEL_H

typedef enum
{
    UI_PAGE_MAIN = 0,
    UI_PAGE_PROTOCOL,
    UI_PAGE_TRIGGER,
    UI_PAGE_STATS
} ui_page_t;

typedef struct
{
    ui_page_t page;
    unsigned char trigger_preset_index;
} ui_model_state_t;

void ui_model_init(ui_model_state_t *state);
void ui_model_next_page(ui_model_state_t *state);
void ui_model_prev_page(ui_model_state_t *state);
void ui_model_trigger_next(ui_model_state_t *state);
void ui_model_trigger_prev(ui_model_state_t *state);
int ui_model_trigger_selected_mv(const ui_model_state_t *state);

#endif /* UI_MODEL_H */

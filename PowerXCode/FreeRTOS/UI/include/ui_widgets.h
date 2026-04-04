#ifndef UI_WIDGETS_H
#define UI_WIDGETS_H

#include <stdint.h>

#include "service_protocol_snapshot.h"

void ui_widgets_fill_line(uint16_t *line, uint16_t width, uint16_t color);
uint16_t ui_widgets_scale_u16(int32_t value,
                              int32_t min_value,
                              int32_t max_value,
                              uint16_t limit);
uint16_t ui_widgets_protocol_color(protocol_kind_t kind, uint8_t emark_present);

#endif /* UI_WIDGETS_H */

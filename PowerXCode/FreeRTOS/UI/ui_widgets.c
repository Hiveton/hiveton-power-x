#include "ui_widgets.h"

#include <stddef.h>

void ui_widgets_fill_line(uint16_t *line, uint16_t width, uint16_t color)
{
    uint16_t i;

    if (line == NULL)
    {
        return;
    }

    for (i = 0U; i < width; ++i)
    {
        line[i] = color;
    }
}

uint16_t ui_widgets_scale_u16(int32_t value,
                              int32_t min_value,
                              int32_t max_value,
                              uint16_t limit)
{
    uint32_t numerator;
    uint32_t denominator;
    uint32_t scaled;

    if (limit == 0U)
    {
        return 0U;
    }

    if (max_value <= min_value)
    {
        return 0U;
    }

    if (value <= min_value)
    {
        return 0U;
    }

    if (value >= max_value)
    {
        return limit;
    }

    denominator = (uint32_t)(max_value - min_value);
    numerator = (uint32_t)(value - min_value);
    scaled = (numerator * (uint32_t)limit) / denominator;
    if (scaled > limit)
    {
        return limit;
    }

    return (uint16_t)scaled;
}

uint16_t ui_widgets_protocol_color(protocol_kind_t kind, uint8_t emark_present)
{
    switch (kind)
    {
        case PROTOCOL_KIND_PD:
            return (emark_present != 0U) ? 0x07E0U : 0x05A0U;
        case PROTOCOL_KIND_QC:
            return 0xFD20U;
        case PROTOCOL_KIND_AFC:
            return 0x07FFU;
        case PROTOCOL_KIND_FCP:
            return 0xF81FU;
        case PROTOCOL_KIND_OTHER:
            return 0xFFFFU;
        case PROTOCOL_KIND_NONE:
        default:
            return 0xFFFFU;
    }
}

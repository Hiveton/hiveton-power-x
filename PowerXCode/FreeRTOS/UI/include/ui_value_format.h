#ifndef UI_VALUE_FORMAT_H
#define UI_VALUE_FORMAT_H

#include <stdint.h>

static inline void ui_value_format_append_uint(char *out,
                                               uint8_t *pos,
                                               uint8_t limit,
                                               uint32_t value,
                                               uint8_t min_digits)
{
    char temp[10];
    uint8_t count;

    count = 0U;
    do
    {
        temp[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (count < sizeof(temp)));

    while ((count < min_digits) && (count < sizeof(temp)))
    {
        temp[count++] = '0';
    }

    while ((count != 0U) && (*pos < limit))
    {
        out[(*pos)++] = temp[--count];
    }
}

static inline void ui_value_format_meter_5half(char *out, uint8_t limit, int32_t milli_value)
{
    uint8_t pos;
    uint32_t value;

    if (limit == 0U)
    {
        return;
    }

    pos = 0U;
    if (milli_value < 0)
    {
        milli_value = 0;
    }
    value = (uint32_t)milli_value;

    if (value >= 100000U)
    {
        uint32_t centi;

        centi = (value + 5U) / 10U;
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), centi / 100U, 1U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), centi % 100U, 2U);
    }
    else if (value >= 10000U)
    {
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), value / 1000U, 1U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), value % 1000U, 3U);
    }
    else
    {
        uint32_t ten_thousandths;

        ten_thousandths = value * 10U;
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), ten_thousandths / 10000U, 1U);
        if (pos < (uint8_t)(limit - 1U))
        {
            out[pos++] = '.';
        }
        ui_value_format_append_uint(out, &pos, (uint8_t)(limit - 1U), ten_thousandths % 10000U, 4U);
    }

    out[pos] = '\0';
}

#endif /* UI_VALUE_FORMAT_H */

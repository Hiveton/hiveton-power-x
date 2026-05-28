#include <assert.h>
#include <string.h>

#include "ui_value_format.h"

static void assert_format(int32_t milli_value, const char *expected)
{
    char out[8];

    ui_value_format_meter_5half(out, sizeof(out), milli_value);
    assert(strcmp(out, expected) == 0);
}

static void assert_power_format(int32_t deci_mw_value, const char *expected)
{
    char out[8];

    ui_value_format_power_5digits_deci_mw(out, sizeof(out), deci_mw_value);
    assert(strcmp(out, expected) == 0);
}

int main(void)
{
    assert_format(0, "0.0000");
    assert_format(5000, "5.0000");
    assert_format(12345, "12.345");
    assert_format(123456, "123.46");
    assert_format(-10, "0.0100");
    assert_format(-12345, "12.345");
    assert_power_format(50050, "5.0050");
    assert_power_format(50055, "5.0055");
    assert_power_format(123456, "12.345");
    assert_power_format(-15, "0.0015");
    return 0;
}

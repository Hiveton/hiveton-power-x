#include <assert.h>
#include <stdint.h>

#include "service_pd_objects.h"

static void test_fixed_pdo_parse_and_rdo(void)
{
    pd_object_t object;
    uint32_t rdo;

    assert(pd_object_parse_source_pdo(1U, 0x0001912CU, &object) == 1U);
    assert(object.position == 1U);
    assert(object.type == PD_OBJECT_TYPE_FIXED);
    assert(object.fixed.voltage_mv == 5000);
    assert(object.fixed.current_ma == 3000);

    assert(pd_object_build_fixed_rdo(&object, 3000, &rdo) == 1U);
    assert(((rdo >> 28) & 0x07U) == 1U);
    assert((rdo & 0x03FFU) == 300U);
    assert(((rdo >> 10) & 0x03FFU) == 300U);
}

static void test_pps_apdo_parse_and_rdo(void)
{
    pd_object_t object;
    uint32_t rdo;

    assert(pd_object_parse_source_pdo(3U, 0xC0DC213CU, &object) == 1U);
    assert(object.position == 3U);
    assert(object.type == PD_OBJECT_TYPE_APDO);
    assert(object.apdo_subtype == PD_APDO_SUBTYPE_SPR_PPS);
    assert(object.pps.min_mv == 3300);
    assert(object.pps.max_mv == 11000);
    assert(object.pps.current_ma == 3000);

    assert(pd_object_build_pps_rdo(&object, 9000, 3000, &rdo) == 1U);
    assert(((rdo >> 28) & 0x07U) == 3U);
    assert(((rdo >> 9) & 0x0FFFU) == 450U);
    assert((rdo & 0x7FU) == 60U);
}

static void test_battery_and_variable_pdo_parse(void)
{
    pd_object_t object;
    uint32_t battery;
    uint32_t variable;

    battery = (1UL << 30) | (400UL << 20) | (100UL << 10) | 240UL;
    assert(pd_object_parse_source_pdo(2U, battery, &object) == 1U);
    assert(object.position == 2U);
    assert(object.type == PD_OBJECT_TYPE_BATTERY);
    assert(object.battery.min_mv == 5000);
    assert(object.battery.max_mv == 20000);
    assert(object.battery.power_mw == 60000);

    variable = (2UL << 30) | (400UL << 20) | (100UL << 10) | 300UL;
    assert(pd_object_parse_source_pdo(3U, variable, &object) == 1U);
    assert(object.position == 3U);
    assert(object.type == PD_OBJECT_TYPE_VARIABLE);
    assert(object.variable.min_mv == 5000);
    assert(object.variable.max_mv == 20000);
    assert(object.variable.current_ma == 3000);
}

static void test_avs_apdo_is_preserved_for_future_requests(void)
{
    pd_object_t object;

    assert(pd_object_parse_source_pdo(4U, 0xD2309664U, &object) == 1U);
    assert(object.position == 4U);
    assert(object.type == PD_OBJECT_TYPE_APDO);
    assert(object.apdo_subtype == PD_APDO_SUBTYPE_EPR_AVS);
    assert(object.epr_avs.min_mv == 15000);
    assert(object.epr_avs.max_mv == 28000);
    assert(object.epr_avs.pdp_w == 100);
}

int main(void)
{
    test_fixed_pdo_parse_and_rdo();
    test_pps_apdo_parse_and_rdo();
    test_battery_and_variable_pdo_parse();
    test_avs_apdo_is_preserved_for_future_requests();
    return 0;
}

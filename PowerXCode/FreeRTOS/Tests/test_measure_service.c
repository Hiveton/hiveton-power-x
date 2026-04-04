#include <assert.h>
#include <limits.h>
#include <stdint.h>

#include "service_measure.h"

static void assert_snapshot_zeroed(const measure_snapshot_t *snapshot)
{
    assert(snapshot->voltage_avg_mv == 0);
    assert(snapshot->current_avg_ma == 0);
    assert(snapshot->power_mw == 0);
    assert(snapshot->voltage_min_mv == 0);
    assert(snapshot->voltage_max_mv == 0);
    assert(snapshot->current_min_ma == 0);
    assert(snapshot->current_max_ma == 0);
    assert(snapshot->ripple_pp_est_mv == 0U);
    assert(snapshot->ripple_level == 0U);
    assert(snapshot->voltage_valid == 0U);
    assert(snapshot->current_valid == 0U);
    assert(snapshot->power_valid == 0U);
}

static void test_nominal_case(void)
{
    const int32_t voltage_mv[] = { 12000, 12100, 11950, 12050 };
    const int32_t current_ma[] = { 1500, 1600, 1550, 1450 };
    measure_snapshot_t snapshot = { 0 };

    measure_service_reset();
    measure_service_process_samples(voltage_mv, current_ma, 4U, &snapshot);

    assert(snapshot.voltage_avg_mv == 12025);
    assert(snapshot.current_avg_ma == 1525);
    assert(snapshot.power_mw == 18338);
    assert(snapshot.voltage_min_mv == 11950);
    assert(snapshot.voltage_max_mv == 12100);
    assert(snapshot.current_min_ma == 1450);
    assert(snapshot.current_max_ma == 1600);
    assert(snapshot.ripple_pp_est_mv == 150);
    assert(snapshot.ripple_level > 0);
    assert(snapshot.voltage_valid == 1U);
    assert(snapshot.current_valid == 1U);
    assert(snapshot.power_valid == 1U);
}

static void test_invalid_input_and_zero_sample_count(void)
{
    const int32_t current_ma[] = { 1500, 1600, 1550, 1450 };
    measure_snapshot_t snapshot = {
        .voltage_avg_mv = 7,
        .current_avg_ma = 8,
        .power_mw = 9,
        .voltage_min_mv = 10,
        .voltage_max_mv = 11,
        .current_min_ma = 12,
        .current_max_ma = 13,
        .ripple_pp_est_mv = 14U,
        .ripple_level = 15U,
    };

    measure_service_process_samples(NULL, current_ma, 4U, &snapshot);
    assert_snapshot_zeroed(&snapshot);

    snapshot.voltage_avg_mv = 1;
    snapshot.current_avg_ma = 2;
    snapshot.power_mw = 3;
    snapshot.voltage_min_mv = 4;
    snapshot.voltage_max_mv = 5;
    snapshot.current_min_ma = 6;
    snapshot.current_max_ma = 7;
    snapshot.ripple_pp_est_mv = 8U;
    snapshot.ripple_level = 9U;

    measure_service_process_samples(current_ma, current_ma, 0U, &snapshot);
    assert_snapshot_zeroed(&snapshot);
}

static void test_ripple_remains_sane(void)
{
    const int32_t voltage_mv[] = { 0, 5000000 };
    const int32_t current_ma[] = { 100, 100 };
    measure_snapshot_t snapshot = { 0 };

    measure_service_process_samples(voltage_mv, current_ma, 2U, &snapshot);

    assert(snapshot.voltage_avg_mv == 2500000);
    assert(snapshot.ripple_pp_est_mv == 5000000U);
    assert(snapshot.ripple_level == 2000U);
    assert(snapshot.power_mw == 250000U);
    assert(snapshot.voltage_min_mv == 0);
    assert(snapshot.voltage_max_mv == 5000000);
    assert(snapshot.current_min_ma == 100);
    assert(snapshot.current_max_ma == 100);
    assert(snapshot.voltage_valid == 1U);
    assert(snapshot.current_valid == 1U);
    assert(snapshot.power_valid == 1U);
}

int main(void)
{
    test_nominal_case();
    test_invalid_input_and_zero_sample_count();
    test_ripple_remains_sane();
    return 0;
}

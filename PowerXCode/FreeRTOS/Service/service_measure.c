#include "service_measure.h"

#include <limits.h>

static void measure_snapshot_clear(measure_snapshot_t *snapshot)
{
    *snapshot = (measure_snapshot_t){ 0 };
}

static int32_t clamp_int32_from_int64(int64_t value)
{
    if (value > (int64_t)INT32_MAX)
    {
        return INT32_MAX;
    }

    if (value < (int64_t)INT32_MIN)
    {
        return INT32_MIN;
    }

    return (int32_t)value;
}

static uint32_t clamp_uint32_from_uint64(uint64_t value)
{
    if (value > (uint64_t)UINT32_MAX)
    {
        return UINT32_MAX;
    }

    return (uint32_t)value;
}

void measure_service_reset(void)
{
    /* Stateless processor: nothing to reset. */
}

void measure_service_process_samples(const int32_t *voltage_mv,
                                     const int32_t *current_ma,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot)
{
    size_t index;
    int64_t voltage_sum = 0;
    int64_t current_sum = 0;
    int32_t voltage_min;
    int32_t voltage_max;
    int32_t current_min;
    int32_t current_max;
    int64_t voltage_avg_mv;
    int64_t current_avg_ma;
    int64_t power_mw;
    uint64_t ripple_span;
    uint64_t ripple_level;

    if (snapshot == NULL)
    {
        return;
    }

    if ((voltage_mv == NULL) || (current_ma == NULL) || (sample_count == 0U))
    {
        measure_snapshot_clear(snapshot);
        return;
    }

    voltage_min = voltage_mv[0];
    voltage_max = voltage_mv[0];
    current_min = current_ma[0];
    current_max = current_ma[0];

    for (index = 0U; index < sample_count; ++index)
    {
        const int32_t voltage = voltage_mv[index];
        const int32_t current = current_ma[index];

        voltage_sum += (int64_t)voltage;
        current_sum += (int64_t)current;

        if (voltage < voltage_min)
        {
            voltage_min = voltage;
        }

        if (voltage > voltage_max)
        {
            voltage_max = voltage;
        }

        if (current < current_min)
        {
            current_min = current;
        }

        if (current > current_max)
        {
            current_max = current;
        }
    }

    voltage_avg_mv = voltage_sum / (int64_t)sample_count;
    current_avg_ma = current_sum / (int64_t)sample_count;
    power_mw = (voltage_avg_mv * current_avg_ma) / 1000LL;

    ripple_span = (uint64_t)((int64_t)voltage_max - (int64_t)voltage_min);
    ripple_level = 0U;

    if (voltage_avg_mv > 0)
    {
        ripple_level = ((uint64_t)ripple_span * 1000ULL) / (uint64_t)voltage_avg_mv;
    }

    snapshot->voltage_avg_mv = clamp_int32_from_int64(voltage_avg_mv);
    snapshot->current_avg_ma = clamp_int32_from_int64(current_avg_ma);
    snapshot->power_mw = clamp_int32_from_int64(power_mw);
    snapshot->voltage_min_mv = voltage_min;
    snapshot->voltage_max_mv = voltage_max;
    snapshot->current_min_ma = current_min;
    snapshot->current_max_ma = current_max;
    snapshot->ripple_pp_est_mv = clamp_uint32_from_uint64(ripple_span);
    snapshot->ripple_level = clamp_uint32_from_uint64(ripple_level);
    snapshot->voltage_valid = 1U;
    snapshot->current_valid = 1U;
    snapshot->power_valid = 1U;
}

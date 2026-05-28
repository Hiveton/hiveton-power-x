#include "service_measure.h"

#include <limits.h>

#define MEASURE_SERVICE_WINDOW_MS 10U
#define MEASURE_RIPPLE_HISTORY_COUNT 32U

typedef struct
{
    uint64_t elapsed_ms;
    uint64_t voltage_mv_ms;
    uint64_t current_ma_ms;
    uint64_t current_deci_ma_ms;
    uint64_t power_mw_ms;
    uint64_t charge_ma_ms;
    uint64_t energy_mw_ms;
    int32_t voltage_max_mv;
    int32_t current_max_ma;
    int32_t current_max_deci_ma;
    int32_t power_max_mw;
    uint8_t initialized;
} measure_stats_t;

static measure_stats_t g_measure_stats;

typedef struct
{
    int32_t voltage_mv[MEASURE_RIPPLE_HISTORY_COUNT];
    uint8_t count;
    uint8_t next_index;
} measure_ripple_history_t;

static measure_ripple_history_t g_measure_ripple;

static void measure_snapshot_clear(measure_snapshot_t *snapshot)
{
    *snapshot = (measure_snapshot_t){ 0 };
}

static int32_t clamp_int32_from_uint64(uint64_t value)
{
    if (value > (uint64_t)INT32_MAX)
    {
        return INT32_MAX;
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

static uint32_t measure_ripple_history_update(int32_t voltage_mv)
{
    uint8_t index;
    int32_t min_mv;
    int32_t max_mv;

    g_measure_ripple.voltage_mv[g_measure_ripple.next_index] = voltage_mv;
    g_measure_ripple.next_index = (uint8_t)((g_measure_ripple.next_index + 1U) % MEASURE_RIPPLE_HISTORY_COUNT);
    if (g_measure_ripple.count < MEASURE_RIPPLE_HISTORY_COUNT)
    {
        ++g_measure_ripple.count;
    }

    min_mv = g_measure_ripple.voltage_mv[0];
    max_mv = min_mv;
    for (index = 1U; index < g_measure_ripple.count; ++index)
    {
        if (g_measure_ripple.voltage_mv[index] < min_mv)
        {
            min_mv = g_measure_ripple.voltage_mv[index];
        }
        if (g_measure_ripple.voltage_mv[index] > max_mv)
        {
            max_mv = g_measure_ripple.voltage_mv[index];
        }
    }

    return (uint32_t)(max_mv - min_mv);
}

static int32_t measure_abs_int32(int32_t value)
{
    if (value == INT32_MIN)
    {
        return INT32_MAX;
    }

    return (value < 0) ? -value : value;
}

static int32_t measure_mul_div_i32(int32_t lhs, int32_t rhs, uint32_t divisor)
{
    uint8_t negative;
    uint32_t left;
    uint32_t right;
    uint32_t result;

    if (divisor == 0U)
    {
        return 0;
    }

    negative = 0U;
    if (lhs < 0)
    {
        negative ^= 1U;
        left = (uint32_t)measure_abs_int32(lhs);
    }
    else
    {
        left = (uint32_t)lhs;
    }

    if (rhs < 0)
    {
        negative ^= 1U;
        right = (uint32_t)measure_abs_int32(rhs);
    }
    else
    {
        right = (uint32_t)rhs;
    }

    result = (left / divisor) * right;
    result += ((left % divisor) * right) / divisor;
    if (result > (uint32_t)INT32_MAX)
    {
        return (negative != 0U) ? INT32_MIN : INT32_MAX;
    }

    return (negative != 0U) ? -(int32_t)result : (int32_t)result;
}

static uint32_t measure_mul_div_u32(uint32_t lhs, uint32_t rhs, uint32_t divisor)
{
    if (divisor == 0U)
    {
        return 0U;
    }

    return (lhs / divisor) * rhs + ((lhs % divisor) * rhs) / divisor;
}

static void measure_stats_update(measure_snapshot_t *snapshot, uint32_t elapsed_ms)
{
    uint64_t elapsed_s;
    int32_t current_abs_ma;
    int32_t current_abs_deci_ma;
    int32_t power_abs_mw;
    uint64_t window_ms;

    if (snapshot == NULL)
    {
        return;
    }

    window_ms = (elapsed_ms != 0U) ? (uint64_t)elapsed_ms : MEASURE_SERVICE_WINDOW_MS;
    current_abs_ma = measure_abs_int32(snapshot->current_avg_ma);
    current_abs_deci_ma = measure_abs_int32(snapshot->current_avg_deci_ma);
    power_abs_mw = measure_abs_int32(snapshot->power_mw);

    if (g_measure_stats.initialized == 0U)
    {
        g_measure_stats.voltage_max_mv = snapshot->voltage_avg_mv;
        g_measure_stats.current_max_ma = current_abs_ma;
        g_measure_stats.current_max_deci_ma = current_abs_deci_ma;
        g_measure_stats.power_max_mw = power_abs_mw;
        g_measure_stats.initialized = 1U;
    }

    if (snapshot->voltage_avg_mv > g_measure_stats.voltage_max_mv)
    {
        g_measure_stats.voltage_max_mv = snapshot->voltage_avg_mv;
    }
    if (current_abs_ma > g_measure_stats.current_max_ma)
    {
        g_measure_stats.current_max_ma = current_abs_ma;
    }
    if (current_abs_deci_ma > g_measure_stats.current_max_deci_ma)
    {
        g_measure_stats.current_max_deci_ma = current_abs_deci_ma;
    }
    if (power_abs_mw > g_measure_stats.power_max_mw)
    {
        g_measure_stats.power_max_mw = power_abs_mw;
    }

    g_measure_stats.elapsed_ms += window_ms;
    g_measure_stats.voltage_mv_ms += (uint64_t)measure_abs_int32(snapshot->voltage_avg_mv) * window_ms;
    g_measure_stats.current_ma_ms += (uint64_t)current_abs_ma * window_ms;
    g_measure_stats.current_deci_ma_ms += (uint64_t)current_abs_deci_ma * window_ms;
    g_measure_stats.power_mw_ms += (uint64_t)power_abs_mw * window_ms;
    g_measure_stats.charge_ma_ms += (uint64_t)current_abs_ma * window_ms;
    g_measure_stats.energy_mw_ms += (uint64_t)power_abs_mw * window_ms;

    elapsed_s = g_measure_stats.elapsed_ms / 1000ULL;
    snapshot->stat_voltage_max_mv = g_measure_stats.voltage_max_mv;
    snapshot->stat_current_max_ma = g_measure_stats.current_max_ma;
    snapshot->stat_current_max_deci_ma = g_measure_stats.current_max_deci_ma;
    snapshot->stat_power_max_mw = g_measure_stats.power_max_mw;
    snapshot->stat_voltage_avg_mv = clamp_int32_from_uint64(g_measure_stats.voltage_mv_ms / g_measure_stats.elapsed_ms);
    snapshot->stat_current_avg_ma = clamp_int32_from_uint64(g_measure_stats.current_ma_ms / g_measure_stats.elapsed_ms);
    snapshot->stat_current_avg_deci_ma = clamp_int32_from_uint64(g_measure_stats.current_deci_ma_ms / g_measure_stats.elapsed_ms);
    snapshot->stat_power_avg_mw = clamp_int32_from_uint64(g_measure_stats.power_mw_ms / g_measure_stats.elapsed_ms);
    snapshot->stat_elapsed_ms = clamp_uint32_from_uint64(g_measure_stats.elapsed_ms);
    snapshot->stat_elapsed_s = clamp_uint32_from_uint64(elapsed_s);
    snapshot->stat_capacity_mah = clamp_uint32_from_uint64(g_measure_stats.charge_ma_ms / 3600000ULL);
    snapshot->stat_energy_mwh = clamp_uint32_from_uint64(g_measure_stats.energy_mw_ms / 3600000ULL);
    snapshot->stat_charge_ma_ms = g_measure_stats.charge_ma_ms;
    snapshot->stat_energy_mw_ms = g_measure_stats.energy_mw_ms;
}

void measure_service_reset(void)
{
    g_measure_stats = (measure_stats_t){ 0 };
    g_measure_ripple = (measure_ripple_history_t){ 0 };
}

void measure_service_process_samples(const int32_t *voltage_mv,
                                     const int32_t *current_ma,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot)
{
    measure_service_process_samples_precise(voltage_mv, current_ma, NULL, sample_count, snapshot);
}

void measure_service_process_samples_precise(const int32_t *voltage_mv,
                                             const int32_t *current_ma,
                                             const int32_t *current_deci_ma,
                                             size_t sample_count,
                                             measure_snapshot_t *snapshot)
{
    measure_service_process_samples_precise_timed(voltage_mv,
                                                  current_ma,
                                                  current_deci_ma,
                                                  sample_count,
                                                  (uint32_t)MEASURE_SERVICE_WINDOW_MS,
                                                  snapshot);
}

void measure_service_process_samples_precise_timed(const int32_t *voltage_mv,
                                                   const int32_t *current_ma,
                                                   const int32_t *current_deci_ma,
                                                   size_t sample_count,
                                                   uint32_t elapsed_ms,
                                                   measure_snapshot_t *snapshot)
{
    size_t index;
    int32_t voltage_sum = 0;
    int32_t current_sum = 0;
    int32_t current_deci_sum = 0;
    int32_t voltage_min;
    int32_t voltage_max;
    int32_t current_min;
    int32_t current_max;
    int32_t current_deci_min;
    int32_t current_deci_max;
    int32_t voltage_avg_mv;
    int32_t current_avg_ma;
    int32_t current_avg_deci_ma;
    int32_t power_mw;
    int32_t power_deci_mw;
    uint32_t ripple_span;
    uint32_t ripple_level;
    int32_t sample_divisor;

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
    current_deci_min = (current_deci_ma != NULL) ? current_deci_ma[0] : current_ma[0] * 10;
    current_deci_max = current_deci_min;

    for (index = 0U; index < sample_count; ++index)
    {
        const int32_t voltage = voltage_mv[index];
        const int32_t current = current_ma[index];
        const int32_t current_deci = (current_deci_ma != NULL) ? current_deci_ma[index] : current * 10;

        voltage_sum += voltage;
        current_sum += current;
        current_deci_sum += current_deci;

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

        if (current_deci < current_deci_min)
        {
            current_deci_min = current_deci;
        }

        if (current_deci > current_deci_max)
        {
            current_deci_max = current_deci;
        }
    }

    sample_divisor = (sample_count > (size_t)INT32_MAX) ? INT32_MAX : (int32_t)sample_count;
    voltage_avg_mv = voltage_sum / sample_divisor;
    current_avg_ma = current_sum / sample_divisor;
    current_avg_deci_ma = current_deci_sum / sample_divisor;
    power_mw = measure_mul_div_i32(voltage_avg_mv, current_avg_deci_ma, 10000U);
    power_deci_mw = measure_mul_div_i32(voltage_avg_mv, current_avg_deci_ma, 1000U);

    ripple_span = (uint32_t)(voltage_max - voltage_min);
    {
        uint32_t rolling_ripple_span;

        rolling_ripple_span = measure_ripple_history_update((int32_t)voltage_avg_mv);
        if (rolling_ripple_span > ripple_span)
        {
            ripple_span = rolling_ripple_span;
        }
    }
    ripple_level = 0U;

    if (voltage_avg_mv > 0)
    {
        ripple_level = measure_mul_div_u32(ripple_span, 1000U, (uint32_t)voltage_avg_mv);
    }

    snapshot->voltage_avg_mv = voltage_avg_mv;
    snapshot->current_avg_ma = current_avg_ma;
    snapshot->current_avg_deci_ma = current_avg_deci_ma;
    snapshot->power_mw = power_mw;
    snapshot->power_deci_mw = power_deci_mw;
    snapshot->voltage_min_mv = voltage_min;
    snapshot->voltage_max_mv = voltage_max;
    snapshot->current_min_ma = current_min;
    snapshot->current_max_ma = current_max;
    snapshot->current_min_deci_ma = current_deci_min;
    snapshot->current_max_deci_ma = current_deci_max;
    snapshot->ripple_pp_est_mv = ripple_span;
    snapshot->ripple_level = ripple_level;
    snapshot->ripple_sample_count = (sample_count > MEASURE_RIPPLE_SAMPLE_CAPACITY) ?
                                    MEASURE_RIPPLE_SAMPLE_CAPACITY :
                                    (uint8_t)sample_count;
    for (index = 0U; index < snapshot->ripple_sample_count; ++index)
    {
        snapshot->ripple_sample_mv[index] = voltage_mv[index];
    }
    snapshot->voltage_valid = 1U;
    snapshot->current_valid = 1U;
    snapshot->power_valid = 1U;
    measure_stats_update(snapshot, elapsed_ms);
}

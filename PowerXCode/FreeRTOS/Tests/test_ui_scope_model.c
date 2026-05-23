#include <assert.h>

#include "ui_scope_model.h"

int main(void)
{
    measure_snapshot_t measure = {
        .voltage_min_mv = 8950,
        .voltage_max_mv = 9050,
        .current_min_ma = 1900,
        .current_max_ma = 2100,
        .ripple_pp_est_mv = 100U,
        .voltage_valid = 1U,
        .current_valid = 1U,
    };
    ui_scope_metrics_t metrics;

    ui_scope_metrics_from_measure(&measure, &metrics);

    assert(metrics.voltage_min_mv == 8950);
    assert(metrics.voltage_max_mv == 9050);
    assert(metrics.current_min_ma == 1900);
    assert(metrics.current_max_ma == 2100);
    assert(metrics.ripple_pp_est_mv == 100U);
    assert(metrics.voltage_valid == 1U);
    assert(metrics.current_valid == 1U);

    ui_scope_metrics_from_measure(0, &metrics);
    assert(metrics.voltage_valid == 0U);
    assert(metrics.current_valid == 0U);
    assert(metrics.ripple_pp_est_mv == 0U);

    return 0;
}

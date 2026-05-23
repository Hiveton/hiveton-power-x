#ifndef UI_SCOPE_MODEL_H
#define UI_SCOPE_MODEL_H

#include <stdint.h>

#include "service_measure.h"

typedef struct
{
    int32_t voltage_min_mv;
    int32_t voltage_max_mv;
    int32_t current_min_ma;
    int32_t current_max_ma;
    uint32_t ripple_pp_est_mv;
    uint8_t voltage_valid;
    uint8_t current_valid;
} ui_scope_metrics_t;

static inline void ui_scope_metrics_from_measure(const measure_snapshot_t *measure,
                                                 ui_scope_metrics_t *metrics)
{
    if (metrics == 0)
    {
        return;
    }

    *metrics = (ui_scope_metrics_t){ 0 };
    if (measure == 0)
    {
        return;
    }

    metrics->voltage_min_mv = measure->voltage_min_mv;
    metrics->voltage_max_mv = measure->voltage_max_mv;
    metrics->current_min_ma = measure->current_min_ma;
    metrics->current_max_ma = measure->current_max_ma;
    metrics->ripple_pp_est_mv = measure->ripple_pp_est_mv;
    metrics->voltage_valid = measure->voltage_valid;
    metrics->current_valid = measure->current_valid;
}

#endif /* UI_SCOPE_MODEL_H */

#ifndef SERVICE_MEASURE_H
#define SERVICE_MEASURE_H

#include <stddef.h>
#include <stdint.h>

typedef struct
{
    int32_t voltage_avg_mv;
    int32_t current_avg_ma;
    int32_t power_mw;
    int32_t voltage_min_mv;
    int32_t voltage_max_mv;
    int32_t current_min_ma;
    int32_t current_max_ma;
    uint32_t ripple_pp_est_mv;
    uint32_t ripple_level;
    uint8_t voltage_valid;
    uint8_t current_valid;
    uint8_t power_valid;
} measure_snapshot_t;

void measure_service_reset(void);
void measure_service_process_samples(const int32_t *voltage_mv,
                                     const int32_t *current_ma,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot);

#endif /* SERVICE_MEASURE_H */

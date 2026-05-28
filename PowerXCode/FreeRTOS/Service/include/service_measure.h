#ifndef SERVICE_MEASURE_H
#define SERVICE_MEASURE_H

#include <stddef.h>
#include <stdint.h>

#define MEASURE_RIPPLE_SAMPLE_CAPACITY 8U

typedef struct
{
    int32_t voltage_avg_mv;
    int32_t current_avg_ma;
    int32_t current_avg_deci_ma;
    int32_t power_mw;
    int32_t voltage_min_mv;
    int32_t voltage_max_mv;
    int32_t current_min_ma;
    int32_t current_max_ma;
    int32_t current_min_deci_ma;
    int32_t current_max_deci_ma;
    uint32_t ripple_pp_est_mv;
    uint32_t ripple_level;
    int32_t ripple_sample_mv[MEASURE_RIPPLE_SAMPLE_CAPACITY];
    uint8_t ripple_sample_count;
    int32_t stat_voltage_max_mv;
    int32_t stat_current_max_ma;
    int32_t stat_current_max_deci_ma;
    int32_t stat_power_max_mw;
    int32_t stat_voltage_avg_mv;
    int32_t stat_current_avg_ma;
    int32_t stat_current_avg_deci_ma;
    int32_t stat_power_avg_mw;
    uint32_t stat_elapsed_ms;
    uint32_t stat_elapsed_s;
    uint32_t stat_capacity_mah;
    uint32_t stat_energy_mwh;
    uint64_t stat_charge_ma_ms;
    uint64_t stat_energy_mw_ms;
    int32_t mcu_temp_deci_c;
    uint8_t voltage_valid;
    uint8_t current_valid;
    uint8_t power_valid;
    uint8_t mcu_temp_valid;
} measure_snapshot_t;

void measure_service_reset(void);
void measure_service_process_samples(const int32_t *voltage_mv,
                                     const int32_t *current_ma,
                                     size_t sample_count,
                                     measure_snapshot_t *snapshot);
void measure_service_process_samples_precise(const int32_t *voltage_mv,
                                             const int32_t *current_ma,
                                             const int32_t *current_deci_ma,
                                             size_t sample_count,
                                             measure_snapshot_t *snapshot);
void measure_service_process_samples_precise_timed(const int32_t *voltage_mv,
                                                   const int32_t *current_ma,
                                                   const int32_t *current_deci_ma,
                                                   size_t sample_count,
                                                   uint32_t elapsed_ms,
                                                   measure_snapshot_t *snapshot);

#endif /* SERVICE_MEASURE_H */

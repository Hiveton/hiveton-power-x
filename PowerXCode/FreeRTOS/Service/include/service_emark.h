#ifndef SERVICE_EMARK_H
#define SERVICE_EMARK_H

#include <stdint.h>

typedef struct
{
    uint8_t present;
    uint32_t current_capacity_a;
    uint8_t max_voltage_v;
    uint8_t cable_length_m;
    uint8_t epr_capable;
    uint8_t usb_speed_grade;
    uint8_t cable_type;
    uint8_t vdo_version;
    uint8_t firmware_version;
    uint8_t hardware_version;
} emark_summary_t;

void service_emark_reset(emark_summary_t *summary);
uint8_t service_emark_summarize_identity(const uint32_t *identity_vdos,
                                         uint8_t vdo_count,
                                         emark_summary_t *summary);

#endif /* SERVICE_EMARK_H */

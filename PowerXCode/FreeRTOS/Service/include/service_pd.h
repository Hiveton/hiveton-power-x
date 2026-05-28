#ifndef SERVICE_PD_H
#define SERVICE_PD_H

#include <stdint.h>

#include "service_protocol_snapshot.h"

#define SERVICE_PD_SOURCE_PDO_MAX 7U

typedef enum
{
    SERVICE_PD_SOURCE_PDO_NONE = 0,
    SERVICE_PD_SOURCE_PDO_FIXED,
    SERVICE_PD_SOURCE_PDO_BATTERY,
    SERVICE_PD_SOURCE_PDO_VARIABLE,
    SERVICE_PD_SOURCE_PDO_PPS,
    SERVICE_PD_SOURCE_PDO_AVS,
} service_pd_source_pdo_type_t;

typedef struct
{
    uint16_t min_mv;
    uint16_t max_mv;
    uint16_t current_ma;
    uint16_t power_deci_w;
    uint8_t position;
    uint8_t type;
} service_pd_source_pdo_t;

typedef struct
{
    uint8_t count;
    service_pd_source_pdo_t pdos[SERVICE_PD_SOURCE_PDO_MAX];
} service_pd_source_caps_snapshot_t;

void service_pd_init(void);
void service_pd_handle_detach(void);
void service_pd_handle_timeout_ms(uint32_t elapsed_ms);
void service_pd_handle_vbus_measurement(int32_t measured_vbus_mv, uint32_t elapsed_ms);
void service_pd_copy_snapshot(protocol_snapshot_t *snapshot);
void service_pd_copy_source_caps(service_pd_source_caps_snapshot_t *snapshot);
void service_pd_set_preferred_voltage_mv(int32_t target_mv);
void service_pd_set_sink_hold(uint8_t enabled);
uint8_t service_pd_sink_hold_enabled(void);
uint8_t service_pd_request_pdo_position(uint8_t position, int32_t target_mv);
void service_pd_request_source_capabilities(void);
void service_pd_request_emark_identity(void);
uint8_t service_pd_prepare_pending_request(uint8_t *tx_packet, uint8_t *tx_length);
uint8_t service_pd_prepare_source_cap_request(uint8_t *tx_packet, uint8_t *tx_length);
uint8_t service_pd_prepare_emark_identity_request(uint8_t *tx_packet, uint8_t *tx_length);
uint8_t service_pd_handle_rx_packet(const uint8_t *packet,
                                    uint8_t byte_count,
                                    uint8_t *tx_packet,
                                    uint8_t *tx_length);

#endif /* SERVICE_PD_H */

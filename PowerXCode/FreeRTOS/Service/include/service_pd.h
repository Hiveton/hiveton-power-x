#ifndef SERVICE_PD_H
#define SERVICE_PD_H

#include <stdint.h>

#include "service_protocol_snapshot.h"

void service_pd_init(void);
void service_pd_handle_detach(void);
void service_pd_handle_timeout_ms(uint32_t elapsed_ms);
void service_pd_copy_snapshot(protocol_snapshot_t *snapshot);
void service_pd_set_preferred_voltage_mv(int32_t target_mv);
uint8_t service_pd_handle_rx_packet(const uint8_t *packet,
                                    uint8_t byte_count,
                                    uint8_t *tx_packet,
                                    uint8_t *tx_length);

#endif /* SERVICE_PD_H */

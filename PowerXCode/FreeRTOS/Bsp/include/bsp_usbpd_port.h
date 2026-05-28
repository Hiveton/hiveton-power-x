#ifndef BSP_USBPD_PORT_H
#define BSP_USBPD_PORT_H

#include <stdint.h>

#define BSP_USBPD_PORT_MAX_PACKET_SIZE 34U

typedef struct
{
    uint32_t rx_total;
    uint32_t rx_sop0;
    uint32_t rx_sop1;
    uint32_t rx_sop2;
    uint32_t rx_goodcrc;
    uint32_t rx_source_cap;
    uint32_t rx_vdm;
    uint32_t rx_overflow;
    uint32_t rx_buf_err;
    uint32_t rx_reset;
    uint16_t last_header;
    uint16_t last_status;
    uint8_t queue_depth;
    uint8_t last_len;
    uint8_t last_sop;
    uint8_t last_msg_type;
    uint8_t last_ndo;
    uint8_t cc_orientation;
} bsp_usbpd_port_diag_t;

void bsp_usbpd_port_init(void);
uint8_t bsp_usbpd_port_fetch_rx_packet(uint8_t *packet, uint8_t *length);
uint8_t bsp_usbpd_port_fetch_detach(void);
uint8_t bsp_usbpd_port_transmit_sop(const uint8_t *packet, uint8_t length);
uint8_t bsp_usbpd_port_transmit_sop_prime(const uint8_t *packet, uint8_t length);
uint8_t bsp_usbpd_port_current_cc(void);
void bsp_usbpd_port_copy_diag(bsp_usbpd_port_diag_t *diag);
void bsp_usbpd_port_monitor_tick_ms(uint32_t elapsed_ms);
void bsp_usbpd_port_resume_rx(void);
void bsp_usbpd_port_set_sink_hold(uint8_t enabled);
uint8_t bsp_usbpd_port_sink_hold_enabled(void);
void bsp_usbpd_irq_handler(void);

#endif /* BSP_USBPD_PORT_H */

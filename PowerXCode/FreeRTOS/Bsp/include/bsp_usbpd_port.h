#ifndef BSP_USBPD_PORT_H
#define BSP_USBPD_PORT_H

#include <stdint.h>

#define BSP_USBPD_PORT_MAX_PACKET_SIZE 34U

void bsp_usbpd_port_init(void);
uint8_t bsp_usbpd_port_fetch_rx_packet(uint8_t *packet, uint8_t *length);
uint8_t bsp_usbpd_port_fetch_detach(void);
uint8_t bsp_usbpd_port_transmit_sop(const uint8_t *packet, uint8_t length);
uint8_t bsp_usbpd_port_transmit_sop_prime(const uint8_t *packet, uint8_t length);
uint8_t bsp_usbpd_port_current_cc(void);
void bsp_usbpd_port_resume_rx(void);
void bsp_usbpd_irq_handler(void);

#endif /* BSP_USBPD_PORT_H */

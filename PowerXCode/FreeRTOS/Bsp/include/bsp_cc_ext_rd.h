#ifndef BSP_CC_EXT_RD_H
#define BSP_CC_EXT_RD_H

#include <stdint.h>

void bsp_cc_ext_rd_init(void);
void bsp_cc_ext_rd_set(uint8_t enabled);
uint8_t bsp_cc_ext_rd_get(void);

#endif /* BSP_CC_EXT_RD_H */

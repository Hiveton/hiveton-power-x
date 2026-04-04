#ifndef BSP_DPDM_H
#define BSP_DPDM_H

typedef enum
{
    BSP_DPDM_MODE_HIZ = 0,
    BSP_DPDM_MODE_DP,
    BSP_DPDM_MODE_DM,
    BSP_DPDM_MODE_BOTH
} bsp_dpdm_mode_t;

void bsp_dpdm_init(void);
void bsp_dpdm_set_mode(bsp_dpdm_mode_t mode);
bsp_dpdm_mode_t bsp_dpdm_get_mode(void);

#endif /* BSP_DPDM_H */

#ifndef BSP_DPDM_H
#define BSP_DPDM_H

typedef enum
{
    BSP_DPDM_MODE_HIZ = 0,
    BSP_DPDM_MODE_DP,
    BSP_DPDM_MODE_DM,
    BSP_DPDM_MODE_BOTH
} bsp_dpdm_mode_t;

typedef enum
{
    BSP_DPDM_LEVEL_HIZ = 0,
    BSP_DPDM_LEVEL_LOW,
    BSP_DPDM_LEVEL_600MV,
    BSP_DPDM_LEVEL_3300MV
} bsp_dpdm_level_t;

typedef struct
{
    unsigned char dp_high;
    unsigned char dm_high;
    unsigned char voltage_valid;
    int dp_mv;
    int dm_mv;
} bsp_dpdm_sample_t;

void bsp_dpdm_init(void);
void bsp_dpdm_set_mode(bsp_dpdm_mode_t mode);
bsp_dpdm_mode_t bsp_dpdm_get_mode(void);
void bsp_dpdm_set_levels(bsp_dpdm_level_t dp_level, bsp_dpdm_level_t dm_level);
void bsp_dpdm_get_levels(bsp_dpdm_level_t *dp_level, bsp_dpdm_level_t *dm_level);
void bsp_dpdm_apply_qc2_voltage_mv(int target_mv);
void bsp_dpdm_apply_qc3_pulse(int step_delta);
unsigned char bsp_dpdm_sample_lines(bsp_dpdm_sample_t *sample);

#if defined(PX1_HOST_TEST)
void bsp_dpdm_mock_set_sample(unsigned char dp_high, unsigned char dm_high);
void bsp_dpdm_mock_set_voltage_mv(int dp_mv, int dm_mv);
void bsp_dpdm_mock_set_adc_unavailable(void);
int bsp_dpdm_mock_get_qc3_offset(void);
int bsp_dpdm_mock_get_qc3_pulse_count(void);
unsigned char bsp_dpdm_mock_get_bc_source_mask(void);
#endif

#endif /* BSP_DPDM_H */

#include "bsp_dpdm.h"
#include "bsp_board_config.h"

#if defined(__riscv)
#include "ch32l103.h"
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#include "ch32l103_usb.h"
#endif

static bsp_dpdm_mode_t g_dpdm_mode = BSP_DPDM_MODE_HIZ;

#if defined(__riscv)
static void bsp_dpdm_gpio_to_floating(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    gpio_init.GPIO_Pin = PX1_USB_DM_PIN | PX1_USB_DP_PIN;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(PX1_DPDM_GPIO, &gpio_init);
}

static void bsp_dpdm_apply_bc_source(uint8_t dp_source_enable, uint8_t dm_source_enable)
{
    uint32_t afio_cr;

    afio_cr = AFIO->CR;
    afio_cr &= ~(AFIO_CR_UPD_BC_VSRC | AFIO_CR_UDM_BC_VSRC |
                 AFIO_CR_UPD_BC_CMPE | AFIO_CR_UDM_BC_CMPE);

    if (dp_source_enable != 0U)
    {
        afio_cr |= AFIO_CR_UPD_BC_VSRC;
    }
    if (dm_source_enable != 0U)
    {
        afio_cr |= AFIO_CR_UDM_BC_VSRC;
    }

    AFIO->CR = afio_cr;
}
#endif

void bsp_dpdm_init(void)
{
    g_dpdm_mode = BSP_DPDM_MODE_HIZ;

#if defined(__riscv)
    RCC_PB2PeriphClockCmd(PX1_DPDM_GPIO_CLOCK | PX1_USBPD_AFIO_CLOCK, ENABLE);
    RCC_HBPeriphClockCmd(RCC_HBPeriph_USBFS, ENABLE);
    bsp_dpdm_gpio_to_floating();
    USBFSD->UDEV_CTRL = USBFS_UD_PD_DIS | USBFS_UD_PORT_EN;
    bsp_dpdm_apply_bc_source(0U, 0U);
#endif
}

void bsp_dpdm_set_mode(bsp_dpdm_mode_t mode)
{
    g_dpdm_mode = mode;

#if defined(__riscv)
    switch (mode)
    {
        case BSP_DPDM_MODE_DP:
            bsp_dpdm_apply_bc_source(1U, 0U);
            break;
        case BSP_DPDM_MODE_DM:
            bsp_dpdm_apply_bc_source(0U, 1U);
            break;
        case BSP_DPDM_MODE_BOTH:
            bsp_dpdm_apply_bc_source(1U, 1U);
            break;
        case BSP_DPDM_MODE_HIZ:
        default:
            bsp_dpdm_apply_bc_source(0U, 0U);
            break;
    }
#endif
}

bsp_dpdm_mode_t bsp_dpdm_get_mode(void)
{
    return g_dpdm_mode;
}

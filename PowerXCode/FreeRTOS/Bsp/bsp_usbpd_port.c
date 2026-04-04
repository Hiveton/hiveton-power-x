#include "bsp_usbpd_port.h"

#include <string.h>

#include "bsp_board_config.h"

#if defined(__riscv)
#include "debug.h"

#include "ch32l103_gpio.h"
#include "ch32l103_misc.h"
#include "ch32l103_rcc.h"
#include "ch32l103_usbpd.h"
#endif

#if defined(__riscv)
__attribute__((aligned(4))) static uint8_t g_pd_rx_buf[34];
__attribute__((aligned(4))) static uint8_t g_pd_tx_buf[34];
static uint8_t g_pd_ack_buf[2];
static uint8_t g_pd_rx_length;
static uint8_t g_pd_message_pending;
static volatile uint8_t g_pd_detach_pending;

static void bsp_usbpd_port_release_cc_line(void)
{
    USBPD->PORT_CC1 &= ~CC_LVE;
    USBPD->PORT_CC2 &= ~CC_LVE;
}

static void bsp_usbpd_port_enter_rx_mode(void)
{
    USBPD->CONFIG |= PD_ALL_CLR;
    USBPD->CONFIG &= ~PD_ALL_CLR;
    USBPD->CONFIG |= IE_RX_ACT | IE_RX_RESET | PD_DMA_EN;
    USBPD->DMA = (uint32_t)g_pd_rx_buf;
    USBPD->CONTROL &= ~PD_TX_EN;
    USBPD->BMC_CLK_CNT = UPD_TMR_RX_96M;
    USBPD->CONTROL |= BMC_START;
    NVIC_EnableIRQ(USBPD_IRQn);
}

static void bsp_usbpd_port_set_sink_mode(void)
{
    USBPD->PORT_CC1 = CC_CMP_66 | CC_PD;
    USBPD->PORT_CC2 = CC_CMP_66 | CC_PD;
}

static uint8_t bsp_usbpd_port_detect_orientation(void)
{
    uint8_t cc1_present;
    uint8_t cc2_present;

    cc1_present = 0U;
    cc2_present = 0U;

    USBPD->PORT_CC1 &= ~(CC_CE | PA_CC_AI);
    USBPD->PORT_CC1 |= CC_CMP_22 | CC_PD;
    Delay_Us(2U);
    if ((USBPD->PORT_CC1 & PA_CC_AI) != 0U)
    {
        cc1_present = 1U;
    }

    USBPD->PORT_CC2 &= ~(CC_CE | PA_CC_AI);
    USBPD->PORT_CC2 |= CC_CMP_22 | CC_PD;
    Delay_Us(2U);
    if ((USBPD->PORT_CC2 & PA_CC_AI) != 0U)
    {
        cc2_present = 1U;
    }

    bsp_usbpd_port_set_sink_mode();

    if (cc1_present != 0U)
    {
        return 1U;
    }

    if (cc2_present != 0U)
    {
        return 2U;
    }

    return 0U;
}

static void bsp_usbpd_port_refresh_orientation(void)
{
    uint8_t orientation;

    orientation = bsp_usbpd_port_detect_orientation();
    if (orientation == 2U)
    {
        USBPD->CONFIG |= CC_SEL;
    }
    else
    {
        USBPD->CONFIG &= ~CC_SEL;
    }
}

static void bsp_usbpd_port_send_packet(uint8_t *packet, uint8_t length, uint8_t sop)
{
    if ((USBPD->CONFIG & CC_SEL) != 0U)
    {
        USBPD->PORT_CC2 |= CC_LVE;
    }
    else
    {
        USBPD->PORT_CC1 |= CC_LVE;
    }

    USBPD->BMC_CLK_CNT = UPD_TMR_TX_96M;
    USBPD->DMA = (uint32_t)packet;
    USBPD->TX_SEL = sop;
    USBPD->BMC_TX_SZ = length;
    USBPD->CONTROL |= PD_TX_EN;
    USBPD->STATUS &= BMC_AUX_INVALID;
    USBPD->CONTROL |= BMC_START;
}
#endif

void bsp_usbpd_port_init(void)
{
#if defined(__riscv)
    {
        GPIO_InitTypeDef gpio_init = { 0 };
        NVIC_InitTypeDef nvic_init = { 0 };
        memset(g_pd_rx_buf, 0, sizeof(g_pd_rx_buf));
        memset(g_pd_tx_buf, 0, sizeof(g_pd_tx_buf));
        g_pd_rx_length = 0U;
        g_pd_message_pending = 0U;
        g_pd_detach_pending = 0U;
        RCC_PB2PeriphClockCmd(PX1_USBPD_CC_GPIO_CLOCK | PX1_USBPD_AFIO_CLOCK, ENABLE);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBPD, ENABLE);

        gpio_init.GPIO_Pin = PX1_USBPD_CC1_PIN | PX1_USBPD_CC2_PIN;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(PX1_USBPD_CC_GPIO, &gpio_init);

        AFIO->CR |= USBPD_IN_HVT;
        USBPD->CONFIG = PD_DMA_EN | PD_FILT_ED;
        USBPD->STATUS = BUF_ERR | IF_RX_BIT | IF_RX_BYTE | IF_RX_ACT | IF_RX_RESET | IF_TX_END;

        bsp_usbpd_port_set_sink_mode();
        bsp_usbpd_port_refresh_orientation();

        nvic_init.NVIC_IRQChannel = USBPD_IRQn;
        nvic_init.NVIC_IRQChannelPreemptionPriority = 0U;
        nvic_init.NVIC_IRQChannelSubPriority = 1U;
        nvic_init.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_init);

        bsp_usbpd_port_enter_rx_mode();
    }
#endif
}

uint8_t bsp_usbpd_port_fetch_rx_packet(uint8_t *packet, uint8_t *length)
{
    uint8_t fetch_ok;

    if ((packet == NULL) || (length == NULL))
    {
        return 0U;
    }

    fetch_ok = 0U;

#if defined(__riscv)
    __disable_irq();
    if ((g_pd_message_pending != 0U) && (g_pd_rx_length <= BSP_USBPD_PORT_MAX_PACKET_SIZE))
    {
        memcpy(packet, g_pd_rx_buf, g_pd_rx_length);
        *length = g_pd_rx_length;
        g_pd_message_pending = 0U;
        g_pd_rx_length = 0U;
        fetch_ok = 1U;
    }
    __enable_irq();
#endif

    return fetch_ok;
}

uint8_t bsp_usbpd_port_fetch_detach(void)
{
    uint8_t detach_pending;

    detach_pending = 0U;

#if defined(__riscv)
    __disable_irq();
    if (g_pd_detach_pending != 0U)
    {
        g_pd_detach_pending = 0U;
        detach_pending = 1U;
    }
    __enable_irq();
#endif

    return detach_pending;
}

uint8_t bsp_usbpd_port_transmit_sop(const uint8_t *packet, uint8_t length)
{
#if defined(__riscv)
    uint8_t got_goodcrc;

    if ((packet == NULL) || (length == 0U) || (length > sizeof(g_pd_tx_buf)))
    {
        return 0U;
    }

    got_goodcrc = 0U;
    memcpy(g_pd_tx_buf, packet, length);
    NVIC_DisableIRQ(USBPD_IRQn);
    USBPD->CONFIG |= IE_TX_END;
    USBPD->STATUS |= IF_TX_END;
    bsp_usbpd_port_send_packet(g_pd_tx_buf, length, UPD_SOP0);

    {
        uint32_t guard;

        guard = 1000000UL;
        while (((USBPD->STATUS & IF_TX_END) == 0U) && (guard != 0UL))
        {
            --guard;
        }

        if (guard == 0UL)
        {
            bsp_usbpd_port_release_cc_line();
            bsp_usbpd_port_enter_rx_mode();
            NVIC_EnableIRQ(USBPD_IRQn);
            return 0U;
        }
    }

    USBPD->STATUS |= IF_TX_END;
    bsp_usbpd_port_release_cc_line();
    bsp_usbpd_port_enter_rx_mode();

    {
        uint32_t guard;

        guard = 250UL;
        while (guard-- != 0UL)
        {
            if ((USBPD->STATUS & IF_RX_ACT) != 0U)
            {
                USBPD->STATUS |= IF_RX_ACT;
                if ((USBPD->BMC_BYTE_CNT == 6U) && ((g_pd_rx_buf[0] & 0x1FU) == DEF_TYPE_GOODCRC))
                {
                    got_goodcrc = 1U;
                    break;
                }
            }
            Delay_Us(3U);
        }
    }

    NVIC_EnableIRQ(USBPD_IRQn);
    return got_goodcrc;
#else
    (void)packet;
    (void)length;
    return 0U;
#endif
}

void bsp_usbpd_irq_handler(void)
{
#if defined(__riscv)
    if ((USBPD->STATUS & IF_RX_ACT) != 0U)
    {
        USBPD->STATUS |= IF_RX_ACT;
        if (((USBPD->STATUS & MASK_PD_STAT) == PD_RX_SOP0) && (USBPD->BMC_BYTE_CNT >= 6U))
        {
            if ((USBPD->BMC_BYTE_CNT != 6U) || ((g_pd_rx_buf[0] & 0x1FU) != DEF_TYPE_GOODCRC))
            {
                g_pd_rx_length = USBPD->BMC_BYTE_CNT;
                g_pd_message_pending = 1U;

                Delay_Us(30U);
                g_pd_ack_buf[0] = 0x41U;
                g_pd_ack_buf[1] = g_pd_rx_buf[1] & 0x0EU;
                USBPD->CONFIG |= IE_TX_END;
                bsp_usbpd_port_send_packet(g_pd_ack_buf, 2U, UPD_SOP0);
            }
        }
    }

    if ((USBPD->STATUS & IF_TX_END) != 0U)
    {
        bsp_usbpd_port_release_cc_line();
        USBPD->STATUS |= IF_TX_END;

        bsp_usbpd_port_enter_rx_mode();
    }

    if ((USBPD->STATUS & IF_RX_RESET) != 0U)
    {
        USBPD->STATUS |= IF_RX_RESET;
        g_pd_detach_pending = 1U;
        g_pd_message_pending = 0U;
        g_pd_rx_length = 0U;
        bsp_usbpd_port_set_sink_mode();
        bsp_usbpd_port_refresh_orientation();
        bsp_usbpd_port_enter_rx_mode();
    }

    if ((USBPD->STATUS & BUF_ERR) != 0U)
    {
        USBPD->STATUS |= BUF_ERR;
        g_pd_detach_pending = 1U;
        g_pd_message_pending = 0U;
        g_pd_rx_length = 0U;
        bsp_usbpd_port_set_sink_mode();
        bsp_usbpd_port_refresh_orientation();
        bsp_usbpd_port_enter_rx_mode();
    }
#endif
}

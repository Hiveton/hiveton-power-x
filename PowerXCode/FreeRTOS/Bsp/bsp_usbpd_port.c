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
__attribute__((aligned(4))) static uint8_t g_pd_ack_buf[4];
static volatile uint8_t g_pd_ack_pending;
static uint8_t g_pd_ack_sop;
static volatile uint8_t g_pd_cc_orientation;
static volatile uint8_t g_pd_monitor_mode = 1U;
static volatile uint8_t g_pd_detach_pending;
static volatile uint8_t g_pd_hw_initialized;
static volatile uint8_t g_pd_sink_hold_requested;

#define PX1_USBPD_TX_RETRY_COUNT 3U
#define PX1_USBPD_RX_QUEUE_DEPTH 8U
#define PX1_USBPD_MONITOR_CC_SCAN_MS 40U

typedef struct
{
    uint8_t data[BSP_USBPD_PORT_MAX_PACKET_SIZE];
    uint8_t length;
    uint8_t sop;
    uint16_t header;
} bsp_usbpd_rx_queue_entry_t;

static bsp_usbpd_rx_queue_entry_t g_pd_rx_queue[PX1_USBPD_RX_QUEUE_DEPTH];
static volatile uint8_t g_pd_rx_queue_head;
static volatile uint8_t g_pd_rx_queue_tail;
static volatile uint8_t g_pd_rx_queue_count;
static volatile uint16_t g_pd_monitor_scan_elapsed_ms;
static volatile uint32_t g_pd_monitor_scan_rx_total;
static bsp_usbpd_port_diag_t g_pd_diag;

static uint16_t bsp_usbpd_port_get_u16_le(const uint8_t *bytes)
{
    return (uint16_t)bytes[0] | (uint16_t)((uint16_t)bytes[1] << 8);
}

static uint8_t bsp_usbpd_port_header_ndo(uint16_t header)
{
    return (uint8_t)((header >> 12) & 0x07U);
}

static uint8_t bsp_usbpd_port_header_type(uint16_t header)
{
    return (uint8_t)(header & 0x1FU);
}

static uint8_t bsp_usbpd_port_header_is_goodcrc(uint16_t header)
{
    return ((bsp_usbpd_port_header_ndo(header) == 0U) &&
            (bsp_usbpd_port_header_type(header) == DEF_TYPE_GOODCRC)) ? 1U : 0U;
}

static void bsp_usbpd_port_reset_queue(void)
{
    g_pd_rx_queue_head = 0U;
    g_pd_rx_queue_tail = 0U;
    g_pd_rx_queue_count = 0U;
}

static void bsp_usbpd_port_select_cc(uint8_t cc)
{
    if (cc == 2U)
    {
        USBPD->CONFIG |= CC_SEL;
        g_pd_cc_orientation = 2U;
    }
    else
    {
        USBPD->CONFIG &= ~CC_SEL;
        g_pd_cc_orientation = 1U;
    }
}

static void bsp_usbpd_port_record_rx_frame(uint8_t sop,
                                           const uint8_t *packet,
                                           uint8_t length,
                                           uint16_t status)
{
    uint16_t header;
    uint8_t ndo;
    uint8_t message_type;

    header = (length >= 2U) ? bsp_usbpd_port_get_u16_le(packet) : 0U;
    ndo = bsp_usbpd_port_header_ndo(header);
    message_type = bsp_usbpd_port_header_type(header);

    g_pd_diag.rx_total++;
    if (sop == PD_RX_SOP0)
    {
        g_pd_diag.rx_sop0++;
    }
    else if (sop == PD_RX_SOP1_HRST)
    {
        g_pd_diag.rx_sop1++;
    }
    else if (sop == PD_RX_SOP2_CRST)
    {
        g_pd_diag.rx_sop2++;
    }

    if (bsp_usbpd_port_header_is_goodcrc(header) != 0U)
    {
        g_pd_diag.rx_goodcrc++;
    }
    else if ((ndo != 0U) && (message_type == 0x01U))
    {
        g_pd_diag.rx_source_cap++;
    }
    else if ((ndo != 0U) && (message_type == 0x0FU))
    {
        g_pd_diag.rx_vdm++;
    }

    g_pd_diag.last_header = header;
    g_pd_diag.last_status = status;
    g_pd_diag.last_len = length;
    g_pd_diag.last_sop = sop;
    g_pd_diag.last_msg_type = message_type;
    g_pd_diag.last_ndo = ndo;
    g_pd_diag.cc_orientation = g_pd_cc_orientation;
    g_pd_monitor_scan_elapsed_ms = 0U;
    g_pd_monitor_scan_rx_total = g_pd_diag.rx_total;
}

static void bsp_usbpd_port_queue_rx_packet(uint8_t sop,
                                           const uint8_t *packet,
                                           uint8_t length,
                                           uint16_t status)
{
    bsp_usbpd_rx_queue_entry_t *entry;

    if ((packet == NULL) ||
        (length < 2U) ||
        (length > BSP_USBPD_PORT_MAX_PACKET_SIZE))
    {
        return;
    }

    bsp_usbpd_port_record_rx_frame(sop, packet, length, status);
    if (bsp_usbpd_port_header_is_goodcrc(g_pd_diag.last_header) != 0U)
    {
        return;
    }

    if (g_pd_rx_queue_count >= PX1_USBPD_RX_QUEUE_DEPTH)
    {
        g_pd_rx_queue_tail = (uint8_t)((g_pd_rx_queue_tail + 1U) % PX1_USBPD_RX_QUEUE_DEPTH);
        g_pd_rx_queue_count--;
        g_pd_diag.rx_overflow++;
    }

    entry = &g_pd_rx_queue[g_pd_rx_queue_head];
    memcpy(entry->data, packet, length);
    entry->length = length;
    entry->sop = sop;
    entry->header = g_pd_diag.last_header;
    g_pd_rx_queue_head = (uint8_t)((g_pd_rx_queue_head + 1U) % PX1_USBPD_RX_QUEUE_DEPTH);
    g_pd_rx_queue_count++;
    g_pd_diag.queue_depth = g_pd_rx_queue_count;
}

static void bsp_usbpd_port_release_cc_line(void)
{
    USBPD->PORT_CC1 &= ~CC_LVE;
    USBPD->PORT_CC2 &= ~CC_LVE;
}

static void bsp_usbpd_port_prepare_rx_hw(void)
{
    USBPD->CONFIG |= PD_ALL_CLR;
    USBPD->CONFIG &= ~PD_ALL_CLR;
    USBPD->CONFIG |= IE_RX_ACT | IE_RX_RESET | PD_DMA_EN;
    USBPD->DMA = (uint32_t)g_pd_rx_buf;
    USBPD->CONTROL &= ~PD_TX_EN;
    USBPD->BMC_CLK_CNT = UPD_TMR_RX_96M;
    USBPD->CONTROL |= BMC_START;
}

static void bsp_usbpd_port_enter_rx_mode(void)
{
    bsp_usbpd_port_prepare_rx_hw();
    NVIC_EnableIRQ(USBPD_IRQn);
}

static void bsp_usbpd_port_pause_rx_mode(void)
{
    bsp_usbpd_port_release_cc_line();
    USBPD->CONFIG &= ~(IE_RX_ACT | IE_RX_RESET | IE_TX_END);
    NVIC_DisableIRQ(USBPD_IRQn);
}

static void bsp_usbpd_port_set_sink_mode(void)
{
    USBPD->PORT_CC1 = CC_CMP_66 | CC_PD;
    USBPD->PORT_CC2 = CC_CMP_66 | CC_PD;
}

static void bsp_usbpd_port_set_monitor_mode_hw(void)
{
    USBPD->PORT_CC1 = CC_CMP_45;
    USBPD->PORT_CC2 = CC_CMP_45;
    USBPD->PORT_CC1 &= ~(CC_PD | CC_LVE);
    USBPD->PORT_CC2 &= ~(CC_PD | CC_LVE);
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

    if (g_pd_monitor_mode != 0U)
    {
        bsp_usbpd_port_set_monitor_mode_hw();
    }
    else
    {
        bsp_usbpd_port_set_sink_mode();
    }

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
        bsp_usbpd_port_select_cc(2U);
    }
    else if (orientation == 1U)
    {
        bsp_usbpd_port_select_cc(1U);
    }
    else
    {
        g_pd_cc_orientation = 0U;
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

static void bsp_usbpd_port_enable_cc_switch(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };

    RCC_PB2PeriphClockCmd(PX1_USBPD_CC_EN_GPIO_CLOCK | PX1_CC1_EXT_RD_CTL_GPIO_CLOCK, ENABLE);
    gpio_init.GPIO_Pin = PX1_USBPD_CC_EN_PIN | PX1_CC1_EXT_RD_CTL_PIN;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(PX1_USBPD_CC_EN_GPIO, &gpio_init);
    GPIO_SetBits(PX1_USBPD_CC_EN_GPIO, PX1_USBPD_CC_EN_PIN);
    if (g_pd_sink_hold_requested != 0U)
    {
        GPIO_ResetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
    else
    {
        GPIO_SetBits(PX1_CC1_EXT_RD_CTL_GPIO, PX1_CC1_EXT_RD_CTL_PIN);
    }
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
        memset(g_pd_rx_queue, 0, sizeof(g_pd_rx_queue));
        memset(&g_pd_diag, 0, sizeof(g_pd_diag));
        bsp_usbpd_port_reset_queue();
        g_pd_monitor_scan_elapsed_ms = 0U;
        g_pd_monitor_scan_rx_total = 0U;
        g_pd_ack_pending = 0U;
        g_pd_ack_sop = UPD_SOP0;
        g_pd_cc_orientation = 0U;
        g_pd_monitor_mode = (g_pd_sink_hold_requested != 0U) ? 0U : 1U;
        g_pd_detach_pending = 0U;
        g_pd_hw_initialized = 0U;
        RCC_PB2PeriphClockCmd(PX1_USBPD_CC_GPIO_CLOCK | PX1_USBPD_AFIO_CLOCK, ENABLE);
        RCC_HBPeriphClockCmd(RCC_HBPeriph_USBPD, ENABLE);
        bsp_usbpd_port_enable_cc_switch();

        gpio_init.GPIO_Pin = PX1_USBPD_CC1_PIN | PX1_USBPD_CC2_PIN;
        gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
        gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
        GPIO_Init(PX1_USBPD_CC_GPIO, &gpio_init);

        AFIO->CR |= USBPD_IN_HVT;
        USBPD->CONFIG = PD_DMA_EN | PD_FILT_ED;
        USBPD->STATUS = BUF_ERR | IF_RX_BIT | IF_RX_BYTE | IF_RX_ACT | IF_RX_RESET | IF_TX_END;

        if (g_pd_sink_hold_requested != 0U)
        {
            bsp_usbpd_port_set_sink_mode();
        }
        else
        {
            bsp_usbpd_port_set_monitor_mode_hw();
        }
        bsp_usbpd_port_refresh_orientation();

        nvic_init.NVIC_IRQChannel = USBPD_IRQn;
        nvic_init.NVIC_IRQChannelPreemptionPriority = 0U;
        nvic_init.NVIC_IRQChannelSubPriority = 1U;
        nvic_init.NVIC_IRQChannelCmd = ENABLE;
        NVIC_Init(&nvic_init);

        g_pd_hw_initialized = 1U;
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
    if (g_pd_rx_queue_count != 0U)
    {
        bsp_usbpd_rx_queue_entry_t *entry;

        entry = &g_pd_rx_queue[g_pd_rx_queue_tail];
        memcpy(packet, entry->data, entry->length);
        *length = entry->length;
        g_pd_rx_queue_tail = (uint8_t)((g_pd_rx_queue_tail + 1U) % PX1_USBPD_RX_QUEUE_DEPTH);
        g_pd_rx_queue_count--;
        g_pd_diag.queue_depth = g_pd_rx_queue_count;
        fetch_ok = 1U;
    }
    __enable_irq();
#endif

    return fetch_ok;
}

void bsp_usbpd_port_resume_rx(void)
{
#if defined(__riscv)
    NVIC_DisableIRQ(USBPD_IRQn);
    if (g_pd_sink_hold_requested != 0U)
    {
        g_pd_monitor_mode = 0U;
    }
    if (g_pd_monitor_mode != 0U)
    {
        bsp_usbpd_port_set_monitor_mode_hw();
        if (g_pd_cc_orientation == 0U)
        {
            bsp_usbpd_port_select_cc(((USBPD->CONFIG & CC_SEL) != 0U) ? 2U : 1U);
        }
    }
    else
    {
        bsp_usbpd_port_set_sink_mode();
        bsp_usbpd_port_refresh_orientation();
    }
    bsp_usbpd_port_prepare_rx_hw();
    NVIC_EnableIRQ(USBPD_IRQn);
#endif
}

void bsp_usbpd_port_set_sink_hold(uint8_t enabled)
{
#if defined(__riscv)
    g_pd_sink_hold_requested = (enabled != 0U) ? 1U : 0U;
    bsp_usbpd_port_enable_cc_switch();
    if (g_pd_hw_initialized == 0U)
    {
        return;
    }

    NVIC_DisableIRQ(USBPD_IRQn);
    g_pd_monitor_mode = (g_pd_sink_hold_requested != 0U) ? 0U : 1U;
    if (g_pd_monitor_mode == 0U)
    {
        bsp_usbpd_port_set_sink_mode();
        bsp_usbpd_port_refresh_orientation();
    }
    else
    {
        bsp_usbpd_port_set_monitor_mode_hw();
        if (g_pd_cc_orientation == 0U)
        {
            bsp_usbpd_port_select_cc(((USBPD->CONFIG & CC_SEL) != 0U) ? 2U : 1U);
        }
    }
    bsp_usbpd_port_prepare_rx_hw();
    NVIC_EnableIRQ(USBPD_IRQn);
#else
    (void)enabled;
#endif
}

uint8_t bsp_usbpd_port_sink_hold_enabled(void)
{
#if defined(__riscv)
    return (g_pd_sink_hold_requested != 0U) ? 1U : 0U;
#else
    return 0U;
#endif
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

uint8_t bsp_usbpd_port_current_cc(void)
{
#if defined(__riscv)
    return g_pd_cc_orientation;
#else
    return 0U;
#endif
}

void bsp_usbpd_port_copy_diag(bsp_usbpd_port_diag_t *diag)
{
    if (diag == NULL)
    {
        return;
    }

#if defined(__riscv)
    __disable_irq();
    *diag = g_pd_diag;
    diag->queue_depth = g_pd_rx_queue_count;
    diag->cc_orientation = g_pd_cc_orientation;
    __enable_irq();
#else
    memset(diag, 0, sizeof(*diag));
#endif
}

void bsp_usbpd_port_monitor_tick_ms(uint32_t elapsed_ms)
{
#if defined(__riscv)
    if (g_pd_monitor_mode == 0U)
    {
        return;
    }

    if (g_pd_diag.rx_total != g_pd_monitor_scan_rx_total)
    {
        g_pd_monitor_scan_rx_total = g_pd_diag.rx_total;
        g_pd_monitor_scan_elapsed_ms = 0U;
        return;
    }

    if (elapsed_ms > 255U)
    {
        elapsed_ms = 255U;
    }

    if (g_pd_monitor_scan_elapsed_ms <= (uint16_t)(0xFFFFU - (uint16_t)elapsed_ms))
    {
        g_pd_monitor_scan_elapsed_ms = (uint16_t)(g_pd_monitor_scan_elapsed_ms + (uint16_t)elapsed_ms);
    }
    else
    {
        g_pd_monitor_scan_elapsed_ms = 0xFFFFU;
    }

    if (g_pd_monitor_scan_elapsed_ms >= PX1_USBPD_MONITOR_CC_SCAN_MS)
    {
        g_pd_monitor_scan_elapsed_ms = 0U;
        if ((g_pd_rx_queue_count != 0U) || (g_pd_ack_pending != 0U))
        {
            return;
        }

        NVIC_DisableIRQ(USBPD_IRQn);
        if ((g_pd_rx_queue_count != 0U) || (g_pd_ack_pending != 0U))
        {
            NVIC_EnableIRQ(USBPD_IRQn);
            return;
        }
        bsp_usbpd_port_set_monitor_mode_hw();
        bsp_usbpd_port_select_cc(((USBPD->CONFIG & CC_SEL) != 0U) ? 1U : 2U);
        bsp_usbpd_port_prepare_rx_hw();
        NVIC_EnableIRQ(USBPD_IRQn);
    }
#else
    (void)elapsed_ms;
#endif
}

static uint8_t bsp_usbpd_port_transmit_packet(const uint8_t *packet, uint8_t length, uint8_t sop)
{
#if defined(__riscv)
    uint8_t got_goodcrc;
    uint8_t tx_attempt;

    if ((packet == NULL) || (length == 0U) || (length > sizeof(g_pd_tx_buf)))
    {
        return 0U;
    }

    got_goodcrc = 0U;
    tx_attempt = 0U;
    g_pd_monitor_mode = 0U;
    bsp_usbpd_port_set_sink_mode();
    bsp_usbpd_port_refresh_orientation();
    memcpy(g_pd_tx_buf, packet, length);
    while ((tx_attempt < PX1_USBPD_TX_RETRY_COUNT) && (got_goodcrc == 0U))
    {
        tx_attempt++;
        NVIC_DisableIRQ(USBPD_IRQn);
        USBPD->CONFIG |= IE_TX_END;
        USBPD->STATUS |= IF_TX_END;
        bsp_usbpd_port_send_packet(g_pd_tx_buf, length, sop);

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
                return 0U;
            }
        }

        USBPD->STATUS |= IF_TX_END;
        bsp_usbpd_port_release_cc_line();
        bsp_usbpd_port_prepare_rx_hw();

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
    }

    bsp_usbpd_port_enter_rx_mode();
    return got_goodcrc;
#else
    (void)packet;
    (void)length;
    (void)sop;
    return 0U;
#endif
}

uint8_t bsp_usbpd_port_transmit_sop(const uint8_t *packet, uint8_t length)
{
#if defined(__riscv)
    return bsp_usbpd_port_transmit_packet(packet, length, UPD_SOP0);
#else
    (void)packet;
    (void)length;
    return 0U;
#endif
}

uint8_t bsp_usbpd_port_transmit_sop_prime(const uint8_t *packet, uint8_t length)
{
#if defined(__riscv)
    return bsp_usbpd_port_transmit_packet(packet, length, UPD_SOP1);
#else
    (void)packet;
    (void)length;
    return 0U;
#endif
}

void bsp_usbpd_irq_handler(void)
{
#if defined(__riscv)
    uint16_t status;

    status = USBPD->STATUS;

    if ((status & IF_RX_ACT) != 0U)
    {
        uint8_t rx_sop;
        uint8_t rx_length;

        rx_sop = (uint8_t)(status & MASK_PD_STAT);
        rx_length = (uint8_t)USBPD->BMC_BYTE_CNT;
        USBPD->STATUS |= IF_RX_ACT;
        if ((g_pd_monitor_mode != 0U) &&
            ((rx_sop == PD_RX_SOP0) || (rx_sop == PD_RX_SOP1_HRST) || (rx_sop == PD_RX_SOP2_CRST)) &&
            (rx_length >= 2U) &&
            (rx_length <= BSP_USBPD_PORT_MAX_PACKET_SIZE))
        {
            bsp_usbpd_port_queue_rx_packet(rx_sop, g_pd_rx_buf, rx_length, status);
            bsp_usbpd_port_enter_rx_mode();
        }
        else if (((rx_sop == PD_RX_SOP0) || (rx_sop == PD_RX_SOP1_HRST)) &&
                 (rx_length >= 6U) &&
                 (rx_length <= BSP_USBPD_PORT_MAX_PACKET_SIZE))
        {
            uint16_t header;

            header = bsp_usbpd_port_get_u16_le(g_pd_rx_buf);
            if (bsp_usbpd_port_header_is_goodcrc(header) == 0U)
            {
                bsp_usbpd_port_queue_rx_packet(rx_sop, g_pd_rx_buf, rx_length, status);

                Delay_Us(30U);
                g_pd_ack_buf[0] = 0x41U;
                g_pd_ack_buf[1] = g_pd_rx_buf[1] & 0x0EU;
                g_pd_ack_pending = 1U;
                g_pd_ack_sop = (rx_sop == PD_RX_SOP1_HRST) ? UPD_SOP1 : UPD_SOP0;
                USBPD->CONFIG |= IE_TX_END;
                bsp_usbpd_port_send_packet(g_pd_ack_buf, 2U, g_pd_ack_sop);
            }
        }
    }

    status = USBPD->STATUS;
    if ((status & IF_TX_END) != 0U)
    {
        bsp_usbpd_port_release_cc_line();
        USBPD->STATUS |= IF_TX_END;

        if (g_pd_ack_pending != 0U)
        {
            g_pd_ack_pending = 0U;
            bsp_usbpd_port_enter_rx_mode();
        }
        else
        {
            bsp_usbpd_port_enter_rx_mode();
        }
    }

    status = USBPD->STATUS;
    if ((status & IF_RX_RESET) != 0U)
    {
        USBPD->STATUS |= IF_RX_RESET;
        g_pd_diag.rx_reset++;
        if (g_pd_monitor_mode != 0U)
        {
            bsp_usbpd_port_set_monitor_mode_hw();
            bsp_usbpd_port_enter_rx_mode();
        }
        else
        {
            bsp_usbpd_port_pause_rx_mode();
            g_pd_cc_orientation = 0U;
            g_pd_detach_pending = 1U;
            bsp_usbpd_port_reset_queue();
            g_pd_diag.queue_depth = 0U;
            g_pd_ack_pending = 0U;
            g_pd_ack_sop = UPD_SOP0;
            bsp_usbpd_port_set_sink_mode();
            bsp_usbpd_port_refresh_orientation();
        }
    }

    status = USBPD->STATUS;
    if ((status & BUF_ERR) != 0U)
    {
        USBPD->STATUS |= BUF_ERR;
        g_pd_diag.rx_buf_err++;
        if (g_pd_monitor_mode != 0U)
        {
            bsp_usbpd_port_set_monitor_mode_hw();
            bsp_usbpd_port_enter_rx_mode();
        }
        else
        {
            bsp_usbpd_port_pause_rx_mode();
            g_pd_detach_pending = 1U;
            bsp_usbpd_port_reset_queue();
            g_pd_diag.queue_depth = 0U;
            g_pd_ack_pending = 0U;
            bsp_usbpd_port_set_sink_mode();
            bsp_usbpd_port_refresh_orientation();
        }
    }
#endif
}

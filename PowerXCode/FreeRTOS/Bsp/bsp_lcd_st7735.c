#include "bsp_lcd_st7735.h"

#include "bsp_board_config.h"

#if defined(__riscv)
#include "debug.h"

#include "ch32l103_dma.h"
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#include "ch32l103_spi.h"
#endif

typedef struct
{
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} bsp_lcd_window_t;

static bsp_lcd_window_t g_window = { 0U, 0U, LCD_WIDTH, LCD_HEIGHT };
static uint8_t g_lcd_ready;

#if defined(__riscv)
#define PX1_LCD_X_OFFSET 0U
#define PX1_LCD_Y_OFFSET 24U
#define PX1_LCD_DMA_CHANNEL DMA1_Channel3
#define PX1_LCD_DMA_CLOCK RCC_HBPeriph_DMA1
#define PX1_LCD_DMA_TC_FLAG DMA1_FLAG_TC3

typedef struct
{
    uint8_t command;
    const uint8_t *data;
    uint8_t data_length;
    uint16_t delay_ms;
} bsp_lcd_init_step_t;

static const uint8_t g_lcd_init_b1[] = { 0x05U, 0x3CU, 0x3CU };
static const uint8_t g_lcd_init_b2[] = { 0x05U, 0x3CU, 0x3CU };
static const uint8_t g_lcd_init_b3[] = { 0x05U, 0x3CU, 0x3CU, 0x05U, 0x3CU, 0x3CU };
static const uint8_t g_lcd_init_b4[] = { 0x03U };
static const uint8_t g_lcd_init_c0[] = { 0x0EU, 0x0EU, 0x04U };
static const uint8_t g_lcd_init_c1[] = { 0xC5U };
static const uint8_t g_lcd_init_c2[] = { 0x0DU, 0x00U };
static const uint8_t g_lcd_init_c3[] = { 0x8DU, 0x2AU };
static const uint8_t g_lcd_init_c4[] = { 0x8DU, 0xEEU };
static const uint8_t g_lcd_init_c5[] = { 0x06U };
static const uint8_t g_lcd_init_36[] = { 0x78U };
static const uint8_t g_lcd_init_3a[] = { 0x55U };
static const uint8_t g_lcd_init_e0[] = {
    0x0BU, 0x17U, 0x0AU, 0x0DU, 0x1AU, 0x19U, 0x16U, 0x1DU,
    0x21U, 0x26U, 0x37U, 0x3CU, 0x00U, 0x09U, 0x05U, 0x10U
};
static const uint8_t g_lcd_init_e1[] = {
    0x0CU, 0x19U, 0x09U, 0x0DU, 0x1BU, 0x19U, 0x15U, 0x1DU,
    0x21U, 0x26U, 0x39U, 0x3EU, 0x00U, 0x09U, 0x05U, 0x10U
};

static const bsp_lcd_init_step_t g_lcd_init_sequence[] = {
    { 0x11U, 0, 0U, 120U },
    { 0xB1U, g_lcd_init_b1, sizeof(g_lcd_init_b1), 0U },
    { 0xB2U, g_lcd_init_b2, sizeof(g_lcd_init_b2), 0U },
    { 0xB3U, g_lcd_init_b3, sizeof(g_lcd_init_b3), 0U },
    { 0xB4U, g_lcd_init_b4, sizeof(g_lcd_init_b4), 0U },
    { 0xC0U, g_lcd_init_c0, sizeof(g_lcd_init_c0), 0U },
    { 0xC1U, g_lcd_init_c1, sizeof(g_lcd_init_c1), 0U },
    { 0xC2U, g_lcd_init_c2, sizeof(g_lcd_init_c2), 0U },
    { 0xC3U, g_lcd_init_c3, sizeof(g_lcd_init_c3), 0U },
    { 0xC4U, g_lcd_init_c4, sizeof(g_lcd_init_c4), 0U },
    { 0xC5U, g_lcd_init_c5, sizeof(g_lcd_init_c5), 0U },
    { 0x36U, g_lcd_init_36, sizeof(g_lcd_init_36), 0U },
    { 0x3AU, g_lcd_init_3a, sizeof(g_lcd_init_3a), 0U },
    { 0xE0U, g_lcd_init_e0, sizeof(g_lcd_init_e0), 0U },
    { 0xE1U, g_lcd_init_e1, sizeof(g_lcd_init_e1), 0U },
    { 0x29U, 0, 0U, 20U },
};

static void bsp_lcd_spi_init(void)
{
    GPIO_InitTypeDef gpio_init = { 0 };
    SPI_InitTypeDef spi_init = { 0 };

    RCC_PB2PeriphClockCmd(PX1_LCD_SPI_GPIO_CLOCK | PX1_LCD_SPI_CLOCK, ENABLE);

    gpio_init.GPIO_Pin = PX1_LCD_PIN_SCK | PX1_LCD_PIN_MOSI;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PX1_LCD_SPI_GPIO, &gpio_init);

    spi_init.SPI_Direction = SPI_Direction_1Line_Tx;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_8b;
    spi_init.SPI_CPOL = SPI_CPOL_High;
    spi_init.SPI_CPHA = SPI_CPHA_2Edge;
    spi_init.SPI_NSS = SPI_NSS_Soft;
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = 7U;
    SPI_Init(PX1_LCD_SPI, &spi_init);
    SPI_Cmd(PX1_LCD_SPI, ENABLE);
}

static void bsp_lcd_dma_init(void)
{
    DMA_InitTypeDef dma_init = { 0 };

    RCC_HBPeriphClockCmd(PX1_LCD_DMA_CLOCK, ENABLE);
    DMA_DeInit(PX1_LCD_DMA_CHANNEL);

    dma_init.DMA_PeripheralBaseAddr = (uint32_t)&PX1_LCD_SPI->DATAR;
    dma_init.DMA_MemoryBaseAddr = 0U;
    dma_init.DMA_DIR = DMA_DIR_PeripheralDST;
    dma_init.DMA_BufferSize = 0U;
    dma_init.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma_init.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma_init.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma_init.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma_init.DMA_Mode = DMA_Mode_Normal;
    dma_init.DMA_Priority = DMA_Priority_VeryHigh;
    dma_init.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(PX1_LCD_DMA_CHANNEL, &dma_init);
}

static void bsp_lcd_ctrl_init(void)
{
#if PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS
    GPIO_InitTypeDef gpio_init = { 0 };

    RCC_PB2PeriphClockCmd(PX1_LCD_CTRL_GPIO_CLOCK, ENABLE);

    gpio_init.GPIO_Pin = PX1_LCD_PIN_DC | PX1_LCD_PIN_RST | PX1_LCD_PIN_CS;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(PX1_LCD_CTRL_GPIO, &gpio_init);

    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_DC | PX1_LCD_PIN_RST | PX1_LCD_PIN_CS);
#endif
}

static void bsp_lcd_write_u8(uint8_t value)
{
    while (SPI_I2S_GetFlagStatus(PX1_LCD_SPI, SPI_I2S_FLAG_TXE) == RESET)
    {
    }

    SPI_I2S_SendData(PX1_LCD_SPI, value);
    while (SPI_I2S_GetFlagStatus(PX1_LCD_SPI, SPI_I2S_FLAG_BSY) != RESET)
    {
    }
}

static void bsp_lcd_write_command(uint8_t command)
{
#if PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS
    GPIO_ResetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS | PX1_LCD_PIN_DC);
    bsp_lcd_write_u8(command);
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS | PX1_LCD_PIN_DC);
#else
    (void)command;
#endif
}

static void bsp_lcd_write_data_block(const uint8_t *data, uint8_t length)
{
#if PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS
    if ((data == 0) || (length == 0U))
    {
        return;
    }

    GPIO_ResetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS);
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_DC);
    while (length-- != 0U)
    {
        bsp_lcd_write_u8(*data++);
    }
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS);
#else
    (void)data;
    (void)length;
#endif
}

static void bsp_lcd_write_data_u16(uint16_t value)
{
    uint8_t bytes[2];

    bytes[0] = (uint8_t)(value >> 8);
    bytes[1] = (uint8_t)value;
    bsp_lcd_write_data_block(bytes, 2U);
}

static void bsp_lcd_push_pixels_dma(const uint16_t *pixels, uint16_t count)
{
    if ((pixels == 0) || (count == 0U))
    {
        return;
    }

    DMA_Cmd(PX1_LCD_DMA_CHANNEL, DISABLE);
    DMA_ClearFlag(PX1_LCD_DMA_TC_FLAG);
    PX1_LCD_DMA_CHANNEL->MADDR = (uint32_t)pixels;
    DMA_SetCurrDataCounter(PX1_LCD_DMA_CHANNEL, count);

    SPI_DataSizeConfig(PX1_LCD_SPI, SPI_DataSize_16b);
    SPI_I2S_DMACmd(PX1_LCD_SPI, SPI_I2S_DMAReq_Tx, ENABLE);
    DMA_Cmd(PX1_LCD_DMA_CHANNEL, ENABLE);

    while (DMA_GetFlagStatus(PX1_LCD_DMA_TC_FLAG) == RESET)
    {
    }

    while (SPI_I2S_GetFlagStatus(PX1_LCD_SPI, SPI_I2S_FLAG_BSY) != RESET)
    {
    }

    DMA_Cmd(PX1_LCD_DMA_CHANNEL, DISABLE);
    DMA_ClearFlag(PX1_LCD_DMA_TC_FLAG);
    SPI_I2S_DMACmd(PX1_LCD_SPI, SPI_I2S_DMAReq_Tx, DISABLE);
    SPI_DataSizeConfig(PX1_LCD_SPI, SPI_DataSize_8b);
}

static void bsp_lcd_address_set(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    bsp_lcd_write_command(0x2AU);
    bsp_lcd_write_data_u16((uint16_t)(x1 + PX1_LCD_X_OFFSET));
    bsp_lcd_write_data_u16((uint16_t)(x2 + PX1_LCD_X_OFFSET));

    bsp_lcd_write_command(0x2BU);
    bsp_lcd_write_data_u16((uint16_t)(y1 + PX1_LCD_Y_OFFSET));
    bsp_lcd_write_data_u16((uint16_t)(y2 + PX1_LCD_Y_OFFSET));

    bsp_lcd_write_command(0x2CU);
}

static void bsp_lcd_run_init_sequence(void)
{
    uint8_t index;

    for (index = 0U; index < (uint8_t)(sizeof(g_lcd_init_sequence) / sizeof(g_lcd_init_sequence[0])); ++index)
    {
        const bsp_lcd_init_step_t *step;

        step = &g_lcd_init_sequence[index];
        bsp_lcd_write_command(step->command);
        bsp_lcd_write_data_block(step->data, step->data_length);
        if (step->delay_ms != 0U)
        {
            Delay_Ms(step->delay_ms);
        }
    }
}
#endif

void bsp_lcd_init(void)
{
    g_window = (bsp_lcd_window_t){ 0U, 0U, LCD_WIDTH, LCD_HEIGHT };
    g_lcd_ready = 0U;

#if defined(__riscv)
    bsp_lcd_ctrl_init();
    bsp_lcd_spi_init();
    bsp_lcd_dma_init();

#if PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS
    GPIO_ResetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_RST);
    Delay_Ms(20U);
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_RST);
    Delay_Ms(120U);

    bsp_lcd_run_init_sequence();
    bsp_lcd_address_set(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U);
    g_lcd_ready = 1U;
#endif
#endif
}

void bsp_lcd_set_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    if (x >= LCD_WIDTH)
    {
        x = LCD_WIDTH - 1U;
    }

    if (y >= LCD_HEIGHT)
    {
        y = LCD_HEIGHT - 1U;
    }

    if (width == 0U)
    {
        width = 1U;
    }

    if (height == 0U)
    {
        height = 1U;
    }

    if ((uint32_t)x + width > LCD_WIDTH)
    {
        width = LCD_WIDTH - x;
    }

    if ((uint32_t)y + height > LCD_HEIGHT)
    {
        height = LCD_HEIGHT - y;
    }

    g_window.x = x;
    g_window.y = y;
    g_window.width = width;
    g_window.height = height;

#if defined(__riscv)
    if (g_lcd_ready == 0U)
    {
        return;
    }

    bsp_lcd_address_set(x, y, (uint16_t)(x + width - 1U), (uint16_t)(y + height - 1U));
#endif

    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void bsp_lcd_push_pixels(const uint16_t *pixels, uint16_t count)
{
    if ((pixels == 0) || (count == 0U))
    {
        return;
    }

#if defined(__riscv)
    if (g_lcd_ready == 0U)
    {
        return;
    }

#if PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS
    GPIO_ResetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS);
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_DC);
    while (count != 0U)
    {
        uint16_t chunk;

        chunk = count;
        bsp_lcd_push_pixels_dma(pixels, chunk);
        pixels += chunk;
        count = (uint16_t)(count - chunk);
    }
    GPIO_SetBits(PX1_LCD_CTRL_GPIO, PX1_LCD_PIN_CS);
#else
    (void)pixels;
    (void)count;
#endif
#else
    (void)pixels;
    (void)count;
#endif
}

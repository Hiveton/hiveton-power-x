#ifndef BSP_BOARD_CONFIG_H
#define BSP_BOARD_CONFIG_H

#if defined(__riscv)
#include "ch32l103_gpio.h"
#include "ch32l103_rcc.h"
#include "ch32l103_spi.h"
#include "ch32l103_tim.h"
#endif

/*
 * Board-level pin mapping switches for PX1.
 *
 * MCU-side GPIO bindings confirmed from:
 * `docs/SCH_HivetonPX1_2026-04-03.pdf`
 *
 * Confirmed mappings:
 * - LCD_SPI_DC   -> PA2
 * - LCD_SPI_RST  -> PA3
 * - LCD_SPI_CS   -> PA4
 * - LCD_SPI_SCK  -> PA5 / SPI1_SCK
 * - LCD_SPI_MOSI -> PA7 / SPI1_MOSI
 * - LCD_BL_PWM   -> PA8 / TIM1_CH1
 * - BTN_1        -> PB15
 * - BTN_2        -> PB9
 * - BTN_3        -> PA15
 * - CC_EN        -> PB8
 * - CC1_EXT_RD_CTL -> PB5
 */

#define PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS 1
#define PX1_BOARD_HAS_CONFIRMED_BACKLIGHT_PWM 1
#define PX1_BOARD_HAS_CONFIRMED_KEY_PINS 1

/*
 * PA11/PA12 are reserved for USB D+/D- passthrough and legacy QC signaling.
 * Do not add a USBFS device stack on these pins in this firmware.
 */

#if defined(__riscv)
/* LCD */
#define PX1_LCD_SPI SPI1
#define PX1_LCD_SPI_GPIO GPIOA
#define PX1_LCD_SPI_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_LCD_SPI_CLOCK RCC_PB2Periph_SPI1
#define PX1_LCD_PIN_SCK GPIO_Pin_5
#define PX1_LCD_PIN_MOSI GPIO_Pin_7
#define PX1_LCD_CTRL_GPIO GPIOA
#define PX1_LCD_CTRL_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_LCD_PIN_DC GPIO_Pin_2
#define PX1_LCD_PIN_RST GPIO_Pin_3
#define PX1_LCD_PIN_CS GPIO_Pin_4

/* Backlight */
#define PX1_BACKLIGHT_GPIO GPIOA
#define PX1_BACKLIGHT_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_BACKLIGHT_TIM TIM1
#define PX1_BACKLIGHT_TIM_CLOCK RCC_PB2Periph_TIM1
#define PX1_BACKLIGHT_PIN GPIO_Pin_8

/* Keys */
#define PX1_KEY1_GPIO GPIOB
#define PX1_KEY1_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_KEY1_PIN GPIO_Pin_15
#define PX1_KEY2_GPIO GPIOB
#define PX1_KEY2_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_KEY2_PIN GPIO_Pin_9
#define PX1_KEY3_GPIO GPIOA
#define PX1_KEY3_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_KEY3_PIN GPIO_Pin_15

/* DP/DM ADC sense taps */
#define PX1_BOARD_HAS_DPDM_ADC_SENSE 1
#define PX1_ADC_GPIO GPIOB
#define PX1_ADC_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_DP_ADC_PIN GPIO_Pin_0
#define PX1_DM_ADC_PIN GPIO_Pin_1

/* INA226 power monitor */
#define PX1_INA226_I2C_GPIO GPIOA
#define PX1_INA226_I2C_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_INA226_I2C_SCL_PIN GPIO_Pin_10
#define PX1_INA226_I2C_SDA_PIN GPIO_Pin_9

/* USB D+/D- */
#define PX1_DPDM_GPIO GPIOA
#define PX1_DPDM_GPIO_CLOCK RCC_PB2Periph_GPIOA
#define PX1_USB_DM_PIN GPIO_Pin_11
#define PX1_USB_DP_PIN GPIO_Pin_12

/* USBPD / CC */
#define PX1_USBPD_CC_GPIO GPIOB
#define PX1_USBPD_CC_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_USBPD_AFIO_CLOCK RCC_PB2Periph_AFIO
#define PX1_USBPD_CC1_PIN GPIO_Pin_6
#define PX1_USBPD_CC2_PIN GPIO_Pin_7
#define PX1_USBPD_CC_EN_GPIO GPIOB
#define PX1_USBPD_CC_EN_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_USBPD_CC_EN_PIN GPIO_Pin_8

/* Misc board controls */
#define PX1_CC1_EXT_RD_CTL_GPIO GPIOB
#define PX1_CC1_EXT_RD_CTL_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_CC1_EXT_RD_CTL_PIN GPIO_Pin_5
#endif

/*
 * Direct ADC scaling constants are kept for DP/DM line sampling and host-side
 * compatibility. Product VBUS/current readings use the INA226 path below.
 */
#define PX1_BOARD_ADC_VREF_MV 3300
#define PX1_BOARD_ADC_FULL_SCALE_COUNTS 4095

/*
 * Legacy direct-VBUS conversion ratio. The firmware does not use this path for
 * product meter readings on PX1; it is intentionally neutral.
 */
#define PX1_BOARD_ADC_VBUS_NUM 1
#define PX1_BOARD_ADC_VBUS_DEN 1

/*
 * Legacy direct-current conversion constants. Real product current is converted
 * from INA226 shunt voltage with the 5mOhm shunt definition below.
 */
#define PX1_BOARD_ADC_CURRENT_ZERO_RAW 0
#define PX1_BOARD_ADC_CURRENT_NUMERATOR 0
#define PX1_BOARD_ADC_CURRENT_DENOMINATOR 1

/*
 * INA226 real measurement path:
 * - Schematic U3 address is 0x40.
 * - R3 is 5mOhm between VBUS_IN and VBUS_OUT.
 * - INA226 VIN+ is on VBUS_OUT and VIN- is on VBUS_IN, so load current is the
 *   negative of the raw shunt-voltage sign.
 */
#define PX1_BOARD_INA226_ADDRESS_7BIT 0x40U
#define PX1_BOARD_INA226_SHUNT_MILLIOHM 5
#define PX1_BOARD_INA226_CURRENT_SIGN (-1)

#endif /* BSP_BOARD_CONFIG_H */

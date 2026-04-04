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
 * - CC1_EXT_RD_CTL -> PB5
 */

#define PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS 1
#define PX1_BOARD_HAS_CONFIRMED_BACKLIGHT_PWM 1
#define PX1_BOARD_HAS_CONFIRMED_KEY_PINS 1

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

/* ADC front-end */
#define PX1_ADC_GPIO GPIOB
#define PX1_ADC_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_ADC_VBUS_PIN GPIO_Pin_0
#define PX1_ADC_CURRENT_PIN GPIO_Pin_1

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

/* Misc board controls */
#define PX1_CC1_EXT_RD_CTL_GPIO GPIOB
#define PX1_CC1_EXT_RD_CTL_GPIO_CLOCK RCC_PB2Periph_GPIOB
#define PX1_CC1_EXT_RD_CTL_PIN GPIO_Pin_5
#endif

/*
 * ADC scaling defaults.
 *
 * Voltage and current channels can be brought to real engineering units by
 * replacing these placeholders with the confirmed divider / gain values from
 * the PX1 analog front-end.
 *
 * Fill these from the schematic / board measurements:
 * - VBUS divider ratio
 * - current zero offset
 * - current shunt/gain conversion
 */
#define PX1_BOARD_ADC_VREF_MV 3300
#define PX1_BOARD_ADC_FULL_SCALE_COUNTS 4095

/*
 * Voltage channel:
 * `PX1_BOARD_ADC_VBUS_NUM / DEN` expresses the ratio between ADC input voltage
 * and real VBUS voltage. Default 1:1 keeps the current behavior conservative.
 */
#define PX1_BOARD_ADC_VBUS_NUM 1
#define PX1_BOARD_ADC_VBUS_DEN 1

/*
 * Current channel:
 * Use an affine transform around an optional zero offset:
 * current_ma = ((raw - zero_raw) * numerator) / denominator
 *
 * The default numerator 0 keeps current reporting disabled until the shunt /
 * amplifier path is confirmed.
 */
#define PX1_BOARD_ADC_CURRENT_ZERO_RAW 0
#define PX1_BOARD_ADC_CURRENT_NUMERATOR 0
#define PX1_BOARD_ADC_CURRENT_DENOMINATOR 1

#endif /* BSP_BOARD_CONFIG_H */

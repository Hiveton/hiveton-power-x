# PX1 Board Bring-Up Template

## Purpose

This file is the handoff checklist for completing the remaining PX1 board-level
 bindings that are still intentionally conservative in firmware.

Current firmware status:

- `USBPD` path is integrated and reviewed.
- `LCD`, `backlight`, and `keys` are gated by board-config switches.
- `VBUS` and `current` engineering scaling are gated by board-config constants.
- `legacy charge` passive detection remains conservative until the DP/DM analog
  network is fully confirmed.

## File To Update

- `PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h`

## LCD / UI

Confirm these MCU-side bindings from schematic/PCB:

- `LCD_SPI_SCK`
- `LCD_SPI_MOSI`
- `LCD_SPI_CS`
- `LCD_SPI_DC`
- `LCD_SPI_RST`

After confirmation:

- Set `PX1_BOARD_HAS_CONFIRMED_LCD_CTRL_PINS` to `1`
- Replace the placeholder GPIO definitions in:
  - `PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c`

Notes:

- `SPI1` `PA5/PA7` is already used in the current driver for `SCK/MOSI`
- `CS/DC/RST` must be confirmed before enabling the LCD control block

## Backlight

Confirm these items:

- `LCD_BL_PWM` MCU pin
- timer channel used for PWM
- active polarity

After confirmation:

- Set `PX1_BOARD_HAS_CONFIRMED_BACKLIGHT_PWM` to `1`
- Replace the placeholder PWM mapping in:
  - `PowerXCode/FreeRTOS/Bsp/bsp_backlight.c`

## Keys

Confirm the MCU GPIO pin for each key:

- `BTN_1`
- `BTN_2`
- `BTN_3`

After confirmation:

- Set `PX1_BOARD_HAS_CONFIRMED_KEY_PINS` to `1`
- Replace the placeholder GPIO reads in:
  - `PowerXCode/FreeRTOS/Bsp/bsp_keys.c`

## Voltage Scaling

Fill these constants in:

- `PX1_BOARD_ADC_VREF_MV`
- `PX1_BOARD_ADC_FULL_SCALE_COUNTS`
- `PX1_BOARD_ADC_VBUS_NUM`
- `PX1_BOARD_ADC_VBUS_DEN`

Recommended source of truth:

- VBUS divider resistor values from schematic
- ADC reference assumptions validated on hardware

Formula used by firmware:

```text
adc_mv = raw * VREF / FULL_SCALE
vbus_mv = adc_mv * VBUS_NUM / VBUS_DEN
```

Example:

- If the divider is `100k : 10k`, then real voltage is about `11x` ADC input
- Use:
  - `PX1_BOARD_ADC_VBUS_NUM 11`
  - `PX1_BOARD_ADC_VBUS_DEN 1`

## Current Scaling

Fill these constants in:

- `PX1_BOARD_ADC_CURRENT_ZERO_RAW`
- `PX1_BOARD_ADC_CURRENT_NUMERATOR`
- `PX1_BOARD_ADC_CURRENT_DENOMINATOR`

Formula used by firmware:

```text
current_ma = (raw - zero_raw) * numerator / denominator
```

Recommended source of truth:

- shunt resistor value
- OPA gain
- ADC input range
- zero-current offset measured on the assembled board

Current is intentionally disabled while:

- `PX1_BOARD_ADC_CURRENT_NUMERATOR == 0`

## Legacy Charge DP/DM

Still required before trustworthy passive detection:

- confirm whether `PA11/UDM` and `PA12/UDP` are driven directly or through a
  resistor / analog switch / bias network
- confirm safe high/low/HIZ combinations for:
  - `QC2`
  - `QC3`
  - `AFC`
  - `FCP`
- confirm whether passive detection requires ADC/comparator sensing rather than
  plain GPIO level reads

Until then:

- active requests may still drive mode changes
- passive detect should remain conservative and return `NONE`

## Suggested Evidence To Collect

- one annotated screenshot for the LCD/key/backlight GPIO page
- one annotated screenshot for the measurement analog front-end
- one short table:
  - net name
  - MCU pin
  - electrical role
  - any inversion / pull-up / analog conditioning


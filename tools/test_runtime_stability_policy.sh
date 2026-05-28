#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LCD_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c"
ADC_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_adc_dma.c"
DPDM_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c"
USBPD_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c"
IRQ_FILE="$ROOT/PowerXCode/FreeRTOS/User/ch32l103_it.c"
APP_TASKS="$ROOT/PowerXCode/FreeRTOS/App/app_tasks.c"

fail() {
  printf 'FAIL runtime-stability-policy: %s\n' "$1" >&2
  exit 1
}

require_match() {
  local desc="$1"
  shift
  "$@" >/dev/null || fail "$desc"
}

require_match \
  "LCD SPI busy wait must have a timeout guard" \
  rg -n "PX1_LCD_SPI_WAIT_GUARD" "$LCD_DRIVER"

require_match \
  "LCD DMA transfer wait must have a timeout guard" \
  rg -n "PX1_LCD_DMA_WAIT_GUARD" "$LCD_DRIVER"

require_match \
  "MCU temperature ADC EOC wait must have a timeout guard" \
  rg -n "PX1_ADC_EOC_WAIT_GUARD" "$ADC_DRIVER"

require_match \
  "DP/DM ADC EOC wait must have a timeout guard" \
  rg -n "PX1_DPDM_ADC_EOC_WAIT_GUARD" "$DPDM_DRIVER"

require_match \
  "USBPD monitor CC scan must disable the USBPD IRQ while restarting RX hardware" \
  bash -c "sed -n '/void bsp_usbpd_port_monitor_tick_ms/,/^}/p' '$USBPD_DRIVER' | rg 'NVIC_DisableIRQ\\(USBPD_IRQn\\)'"

require_match \
  "USBPD monitor CC scan must restart RX through the low-level prepare path, not by re-enabling IRQ mid-critical-section" \
  bash -c "sed -n '/void bsp_usbpd_port_monitor_tick_ms/,/^}/p' '$USBPD_DRIVER' | rg 'bsp_usbpd_port_prepare_rx_hw\\('"

require_match \
  "HardFault handler must not hide crashes by immediately resetting the MCU" \
  bash -c "! sed -n '/void HardFault_Handler/,/^}/p' '$IRQ_FILE' | rg 'NVIC_SystemReset'"

require_match \
  "UI task stack must leave headroom for full-page render locals" \
  rg -n "#define APP_TASK_STACK_UI 1024U|#define APP_TASK_STACK_UI 1280U" "$APP_TASKS"

require_match \
  "protocol task stack must leave headroom for PD packet buffers and snapshots" \
  rg -n "#define APP_TASK_STACK_PROTOCOL 640U|#define APP_TASK_STACK_PROTOCOL 768U|#define APP_TASK_STACK_PROTOCOL 1024U" "$APP_TASKS"

printf '%s\n' "PASS runtime-stability-policy"

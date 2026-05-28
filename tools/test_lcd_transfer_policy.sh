#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LCD_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c"
UI_RENDERER="$ROOT/PowerXCode/FreeRTOS/UI/ui_renderer.c"
UI_PAGES="$ROOT/PowerXCode/FreeRTOS/UI/ui_pages.c"

fail() {
  printf 'FAIL lcd-transfer-policy: %s\n' "$1" >&2
  exit 1
}

require_match() {
  local desc="$1"
  shift
  "$@" >/dev/null || fail "$desc"
}

require_match \
  "pixel streaming must queue SPI bytes and wait for bus idle only at the end" \
  rg -n "bsp_lcd_write_u8_stream|bsp_lcd_wait_idle" "$LCD_DRIVER"

require_match \
  "bsp_lcd_push_pixels must use DMA pixel streaming instead of byte polling" \
  bash -c "sed -n '/void bsp_lcd_push_pixels/,/^}/p' '$LCD_DRIVER' | rg 'bsp_lcd_dma_transfer_pixels'"

require_match \
  "LCD SPI DMA must be enabled for Tx transfers" \
  rg -n "SPI_I2S_DMACmd\\([^,]+, SPI_I2S_DMAReq_Tx, ENABLE\\)" "$LCD_DRIVER"

require_match \
  "full UI page renders must stream one continuous LCD window instead of resetting the address for every row" \
  rg -n "ui_renderer_begin_frame_stream|g_frame_streaming|bsp_lcd_set_window\\(0U, 0U, LCD_WIDTH, LCD_HEIGHT\\)" "$UI_RENDERER"

require_match \
  "ui_pages_draw must wrap page rendering in a frame stream" \
  rg -n "ui_renderer_begin_frame_stream\\(|ui_renderer_end_frame_stream\\(" "$UI_PAGES"

printf '%s\n' "PASS lcd-transfer-policy"

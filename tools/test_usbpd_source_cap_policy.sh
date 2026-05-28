#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
USBPD_DRIVER="$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_usbpd_port.c"

fail() {
  printf 'FAIL usbpd-source-cap-policy: %s\n' "$1" >&2
  exit 1
}

require_match() {
  local desc="$1"
  shift
  "$@" >/dev/null || fail "$desc"
}

require_match \
  "USB-PD transmit must retry the same packet when GoodCRC is missed" \
  rg -n "PX1_USBPD_TX_RETRY_COUNT|tx_attempt" "$USBPD_DRIVER"

require_match \
  "USB-PD transmit retry must resend the same prepared packet instead of asking service code for a new MsgID" \
  bash -c "sed -n '/static uint8_t bsp_usbpd_port_transmit_packet/,/^}/p' '$USBPD_DRIVER' | rg 'while .*tx_attempt.*PX1_USBPD_TX_RETRY_COUNT'"

require_match \
  "USB-PD must boot in passive monitor mode so inline PDO/E-Marker sniffing does not disturb the charger" \
  rg -n "g_pd_monitor_mode = 1U|bsp_usbpd_port_set_monitor_mode_hw" "$USBPD_DRIVER"

require_match \
  "USB-PD monitor mode must not emit GoodCRC for pass-through traffic" \
  bash -c "sed -n '/void bsp_usbpd_irq_handler/,/^}/p' '$USBPD_DRIVER' | rg 'g_pd_monitor_mode.*0U|monitor mode'"

require_match \
  "USB-PD monitor receive path must queue multiple frames because Source_Cap can arrive behind GoodCRC/Request traffic" \
  rg -n "PX1_USBPD_RX_QUEUE_DEPTH|bsp_usbpd_port_queue_rx_packet" "$USBPD_DRIVER"

require_match \
  "USB-PD IRQ must snapshot STATUS before clearing IF_RX_ACT so SOP type is not lost" \
  bash -c "sed -n '/void bsp_usbpd_irq_handler/,/^}/p' '$USBPD_DRIVER' | rg 'status = USBPD->STATUS'"

require_match \
  "USB-PD monitor mode must ignore GoodCRC frames instead of occupying the RX queue with them" \
  rg -n "bsp_usbpd_port_header_is_goodcrc" "$USBPD_DRIVER"

require_match \
  "USB-PD monitor mode must scan CC1/CC2 when no frames are seen so a wrong initial orientation does not leave PDO empty" \
  rg -n "bsp_usbpd_port_monitor_tick_ms|g_pd_monitor_scan_elapsed_ms" "$USBPD_DRIVER"

require_match \
  "USB-PD monitor resume must preserve the selected CC line instead of probing with internal Rd after every packet" \
  bash -c "if awk '/if \\(g_pd_monitor_mode != 0U\\)/,/else/' '$USBPD_DRIVER' | rg 'bsp_usbpd_port_refresh_orientation'; then exit 1; fi"

printf '%s\n' "PASS usbpd-source-cap-policy"

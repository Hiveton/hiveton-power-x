#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ELF="${1:-$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.elf}"
DEFAULT_TOOLCHAIN="/Applications/MounRiver Studio 2.app/Contents/Resources/app/resources/darwin/components/WCH/Toolchain/RISC-V Embedded GCC/bin"

fail() {
  printf 'FAIL product-binary-contract: %s\n' "$1" >&2
  exit 1
}

[[ -s "$ELF" ]] || fail "missing or empty ELF: $ELF"

if command -v riscv-none-embed-nm >/dev/null 2>&1; then
  NM_BIN="$(command -v riscv-none-embed-nm)"
elif [[ -x "$DEFAULT_TOOLCHAIN/riscv-none-embed-nm" ]]; then
  NM_BIN="$DEFAULT_TOOLCHAIN/riscv-none-embed-nm"
elif command -v nm >/dev/null 2>&1; then
  NM_BIN="$(command -v nm)"
else
  fail "no nm tool found"
fi

SYMBOLS="$("$NM_BIN" "$ELF" 2>/dev/null || true)"
[[ -n "$SYMBOLS" ]] || fail "no symbols read from $ELF"

require_symbol() {
  local symbol="$1"

  grep -Eq "[[:space:]]${symbol}$" <<<"$SYMBOLS" \
    || fail "missing required product symbol: $symbol"
}

forbid_symbol_name_regex() {
  local regex="$1"
  local match

  match="$(awk -v re="$regex" 'NF >= 2 { name=$NF; if (name ~ re) print $0 }' <<<"$SYMBOLS" | head -n 1)"
  [[ -z "$match" ]] || fail "forbidden symbol present: $match"
}

forbid_strong_symbol_name_regex() {
  local regex="$1"
  local match

  match="$(awk -v re="$regex" '
    NF >= 3 {
      type=$(NF - 1)
      name=$NF
      if (type ~ /^[TtDdBbRrSsGgCc]$/ && name ~ re) {
        print $0
        exit
      }
    }
  ' <<<"$SYMBOLS")"
  [[ -z "$match" ]] || fail "forbidden strong symbol present: $match"
}

require_symbol "app_ui_navigation_apply"
require_symbol "app_protocol_arbiter_publish"
require_symbol "app_protocol_arbiter_copy"
require_symbol "bsp_adc_dma_init"
require_symbol "bsp_adc_dma_fetch_window"
require_symbol "bsp_keys_poll"
require_symbol "bsp_keys_exti9_5_irq_handler"
require_symbol "bsp_keys_exti15_10_irq_handler"
require_symbol "bsp_lcd_set_rotation"
require_symbol "bsp_backlight_set"
require_symbol "bsp_usbpd_port_init"
require_symbol "bsp_usbpd_irq_handler"
require_symbol "bsp_usbpd_port_fetch_rx_packet"
require_symbol "bsp_usbpd_port_fetch_detach"
require_symbol "bsp_usbpd_port_current_cc"
require_symbol "bsp_usbpd_port_transmit_sop"
require_symbol "bsp_usbpd_port_transmit_sop_prime"
require_symbol "bsp_usbpd_port_set_sink_hold"
require_symbol "bsp_dpdm_sample_lines"
require_symbol "service_pd_handle_rx_packet"
require_symbol "service_pd_handle_vbus_measurement"
require_symbol "service_pd_handle_timeout_ms"
require_symbol "service_pd_copy_source_caps"
require_symbol "service_pd_set_sink_hold"
require_symbol "service_pd_request_pdo_position"
require_symbol "service_pd_request_source_capabilities"
require_symbol "service_pd_prepare_pending_request"
require_symbol "service_pd_prepare_source_cap_request"
require_symbol "service_pd_prepare_emark_identity_request"
require_symbol "service_emark_summarize_identity"
require_symbol "service_legacy_charge_poll"
require_symbol "ui_pages_draw"
require_symbol "ui_renderer_draw_main_page"
require_symbol "ui_renderer_draw_scope_page"
require_symbol "ui_renderer_draw_ripple_page"
require_symbol "ui_renderer_draw_trigger_select_page"
require_symbol "ui_renderer_draw_trigger_adjust_page"
require_symbol "ui_renderer_draw_protocol_page"
require_symbol "ui_renderer_draw_pdo_page"
require_symbol "ui_renderer_draw_emark_page"
require_symbol "ui_renderer_draw_settings_page"

forbid_symbol_name_regex "mock"
forbid_symbol_name_regex "ui_renderer_draw_key_debug_page|bsp_keys_get_debug_state|app_draw_minimal_key_diag|app_draw_business_heartbeat|app_draw_stage_marker"
forbid_strong_symbol_name_regex "CDC|USBD|USBFS|usbfs|cdc|usbd"

printf '%s\n' "PASS product-binary-contract"

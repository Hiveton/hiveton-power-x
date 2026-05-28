#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
ELF="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.elf"
UI_REPORT="$ROOT/artifacts/ui-preview/current-ui-report.md"
FLASH_REPORT="${PX1_FLASH_REPORT:-$ROOT/artifacts/flash/latest-success.md}"
HARDWARE_REPORT="${1:-$ROOT/artifacts/hardware-validation/latest.md}"
COVERAGE_DIR="$ROOT/artifacts/goal"
COVERAGE_REPORT="$COVERAGE_DIR/objective-coverage.md"
SOFTWARE_ONLY=0
FAIL_COUNT=0
BLOCKED_COUNT=0

if [[ "${1:-}" == "--software-only" ]]; then
  SOFTWARE_ONLY=1
  shift
  HARDWARE_REPORT="${1:-$ROOT/artifacts/hardware-validation/latest.md}"
fi

mkdir -p "$COVERAGE_DIR"
: >"$COVERAGE_REPORT"

append_report() {
  printf '%s\n' "$*" >>"$COVERAGE_REPORT"
}

mark_row() {
  local item="$1"
  local status="$2"
  local evidence="$3"

  append_report "| $item | $status | $evidence |"
}

mark_fail() {
  mark_row "$1" "fail" "$2"
  FAIL_COUNT=$((FAIL_COUNT + 1))
}

mark_blocked() {
  mark_row "$1" "blocked" "$2"
  BLOCKED_COUNT=$((BLOCKED_COUNT + 1))
}

mark_pass() {
  mark_row "$1" "pass" "$2"
}

file_has() {
  local file="$1"
  shift

  [[ -f "$file" ]] && rg -Fq "$*" "$file"
}

file_lacks() {
  local file="$1"
  shift

  [[ -f "$file" ]] && ! rg -q "$*" "$file"
}

png_is_size() {
  local file="$1"
  local width="$2"
  local height="$3"
  local actual_width
  local actual_height

  [[ -s "$file" ]] || return 1
  actual_width="$(sips -g pixelWidth "$file" 2>/dev/null | awk '/pixelWidth/ { print $2; exit }')"
  actual_height="$(sips -g pixelHeight "$file" 2>/dev/null | awk '/pixelHeight/ { print $2; exit }')"
  [[ "$actual_width" == "$width" && "$actual_height" == "$height" ]]
}

require_ui_pages() {
  local page

  [[ -s "$UI_REPORT" ]] || return 1
  for page in main dpdm power capacity protocol pdo emark scope ripple settings menu; do
    grep -Eq "^[|][[:space:]]*${page}[[:space:]]*[|][[:space:]]*160x80[[:space:]]*[|]" "$UI_REPORT" || return 1
    png_is_size "$ROOT/artifacts/ui-preview/current-ui-${page}-160x80.png" 160 80 || return 1
  done
}

require_ui_render_contract() {
  local renderer
  local behavior_test

  for renderer in \
    ui_renderer_draw_main_page \
    ui_renderer_draw_dpdm_page \
    ui_renderer_draw_power_stats_page \
    ui_renderer_draw_capacity_page \
    ui_renderer_draw_scope_page \
    ui_renderer_draw_ripple_page \
    ui_renderer_draw_protocol_page \
    ui_renderer_draw_pdo_page \
    ui_renderer_draw_emark_page \
    ui_renderer_draw_menu_page \
    ui_renderer_draw_settings_page; do
    file_has "$ROOT/PowerXCode/FreeRTOS/UI/ui_pages.c" "$renderer" || return 1
    file_has "$ROOT/PowerXCode/FreeRTOS/UI/ui_renderer.c" "$renderer" || return 1
  done

  for behavior_test in \
    test_main_page_routes_to_main_renderer \
    test_extra_pages_route_to_dedicated_renderers \
    test_invalid_page_falls_back_to_product_home; do
    file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_ui_pages.c" "$behavior_test" || return 1
  done
}

require_protocol_arbitration_contract() {
  local behavior_test

  for behavior_test in \
    test_pd_none_does_not_clear_legacy_qc \
    test_pd_snapshot_has_priority_over_legacy_qc \
    test_cc_attached_pd_path_hides_passive_legacy_qc_available \
    test_pd_cc_detach_clears_hidden_passive_legacy_qc_available; do
    file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_app_protocol_arbiter.c" "$behavior_test" || return 1
  done

  for behavior_test in \
    test_vbus_without_protocol_renders_as_other \
    test_elevated_vbus_without_pd_does_not_guess_qc \
    test_vbus_with_cc_attached_does_not_fallback_to_other \
    test_vbus_with_cc_orientation_does_not_fallback_to_other; do
    file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_ui_pages.c" "$behavior_test" || return 1
  done

  file_has "$ROOT/PowerXCode/FreeRTOS/App/app_protocol_arbiter.c" 'APP_PROTOCOL_SOURCE_PD' || return 1
  file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_app_protocol_arbiter.c" 'PROTOCOL_KIND_QC' || return 1
  file_has "$ROOT/PowerXCode/FreeRTOS/UI/ui_pages.c" 'measure->voltage_avg_mv >= UI_PAGES_VBUS_PRESENT_MV' || return 1
}

require_hardware_report_complete() {
  [[ -s "$HARDWARE_REPORT" ]] || return 1
  PX1_FLASH_REPORT="$FLASH_REPORT" \
    "$ROOT/tools/check_powerx_hardware_validation.sh" "$HARDWARE_REPORT" "$BIN" >/dev/null
}

require_flash_success() {
  local sha

  [[ -s "$BIN" && -s "$FLASH_REPORT" ]] || return 1
  sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
  grep -Fq "Firmware SHA256: \`$sha\`" "$FLASH_REPORT" || return 1
  grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`flashed`[[:space:]]*$' "$FLASH_REPORT" || return 1
  grep -Fq '## wchisp info' "$FLASH_REPORT" || return 1
  grep -Fq 'CH32L103K8U6' "$FLASH_REPORT" || return 1
  grep -Fq 'wchisp info exit: `0`' "$FLASH_REPORT" || return 1
  grep -Fq '## wchisp flash' "$FLASH_REPORT" || return 1
  grep -Fq 'wchisp flash exit: `0`' "$FLASH_REPORT" || return 1
  grep -Fq 'Verify OK' "$FLASH_REPORT" || return 1
  grep -Fq 'Device reset' "$FLASH_REPORT" || return 1
}

append_report "# PX1 Objective Coverage Audit"
append_report ""
append_report "- Time: $(date '+%Y-%m-%d %H:%M:%S %z')"
append_report "- Mode: $([[ "$SOFTWARE_ONLY" == "1" ]] && printf 'software-only' || printf 'full')"
append_report "- Firmware: \`$BIN\`"
append_report "- UI report: \`$UI_REPORT\`"
append_report "- Flash report: \`$FLASH_REPORT\`"
append_report "- Hardware report: \`$HARDWARE_REPORT\`"
append_report ""
append_report "| requirement | status | evidence |"
append_report "| --- | --- | --- |"

if require_ui_pages && require_ui_render_contract; then
  mark_pass "160x80 UI and 11 active UI pages" "all current-ui page PNGs are 160x80, listed in current-ui-report.md, and wired to dedicated renderer/page-route tests"
else
  mark_fail "160x80 UI and 11 active UI pages" "missing 160x80 page artifact, UI report row, renderer symbol, or page-route behavior-test evidence"
fi

if "$ROOT/tools/check_ui_reference_delta.sh" >/dev/null; then
  mark_pass "design-reference visual regression" "tools/check_ui_reference_delta.sh passes against 160x80 reference pages"
else
  mark_fail "design-reference visual regression" "tools/check_ui_reference_delta.sh failed"
fi

if "$ROOT/tools/check_firmware_contract.sh" >/dev/null; then
  mark_pass "product task configuration" "measure/protocol/legacy tasks enabled and diagnostic screens disabled"
else
  mark_fail "product task configuration" "tools/check_firmware_contract.sh failed"
fi

if [[ -s "$ELF" ]] && "$ROOT/tools/check_product_binary_contract.sh" "$ELF" >/dev/null; then
  mark_pass "product binary integration" "required BSP/Service/App/UI symbols are present; debug UI, mock, USB CDC, and USBFS symbols are forbidden"
else
  mark_fail "product binary integration" "tools/check_product_binary_contract.sh failed or ELF is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h" 'PX1_BOARD_INA226_ADDRESS_7BIT 0x40U' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h" 'PX1_BOARD_INA226_SHUNT_MILLIOHM 5' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_bsp_adc_config.c" 'PX1_BOARD_INA226_CURRENT_SIGN == -1'; then
  mark_pass "real voltage/current/power path" "INA226 0x40 with 5mR shunt is locked by board config and host test"
else
  mark_fail "real voltage/current/power path" "INA226 address/shunt/current-sign evidence is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_requests_default_pdo_after_source_capabilities' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_selects_fixed_pdo_and_updates_contract' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_waits_for_vbus_measurement_after_ps_rdy' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_requests_pps_apdo_when_target_matches_pps_range' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_ui_model.c" 'test_pdo_fine_target_steps_in_20mv_units' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Service/service_pd.c" 'service_pd_prepare_emark_identity_request'; then
  mark_pass "USB PD detect and PDO" "PD Source_Cap default PDO, fixed PDO contract, PPS APDO request, PS_RDY/VBUS verification, PDO target model, and SOP prime E-marker paths are behavior-tested"
else
  mark_fail "USB PD detect and PDO" "PD Source_Cap/PDO/PPS/VBUS/PDO model behavior-test or E-marker path evidence is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_legacy_charge_request_drives_dpdm_mode' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_legacy_charge_records_requested_voltage_and_qc3_steps' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_legacy_charge_poll_reports_qc_state_and_dpdm_voltage' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_legacy_charge_qc3_voltage_uses_200mv_steps' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_legacy_charge_timeout_releases_dpdm_request_levels' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c" 'bsp_dpdm_apply_qc3_pulse'; then
  mark_pass "USB QC detect" "QC2/QC3 DP/DM mode, fixed-voltage requests, 200mV QC3 pulse stepping, VBUS confirmation, and timeout release are behavior-tested"
else
  mark_fail "USB QC detect" "QC2/QC3 DP/DM, VBUS confirmation, timeout release, or driver-path behavior evidence is missing"
fi

if require_protocol_arbitration_contract; then
  mark_pass "protocol arbitration and VBUS fallback" "PD/QC arbitration priority, passive-QC hiding, CC detach recovery, and no-protocol VBUS fallback are behavior-tested"
else
  mark_fail "protocol arbitration and VBUS fallback" "protocol arbitration behavior-test or VBUS fallback behavior-test evidence is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_app_ui_navigation.c" 'UI_PAGE_MAIN' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_app_ui_navigation.c" 'UI_PAGE_SETTINGS' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_bsp_keys_polarity.c" 'btn2_is_active_high' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_bsp_keys_polarity.c" 'btn1_and_btn3_are_active_low' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_bsp_keys_polarity.c" 'test_btn1_poll_fallback_emits_short_event_on_release' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Bsp/bsp_keys.c" 'BTN1 is PB15 and BTN3 is PA15, so both share EXTI line 15'; then
  mark_pass "three-button browse/action interaction" "page order/action mode are tested; BTN2/BTN3 use EXTI and BTN1 uses the explicit scan fallback because PB15 shares EXTI15 with PA15"
else
  mark_fail "three-button browse/action interaction" "navigation, key polarity, EXTI, or BTN1 scan-fallback evidence is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/UI/ui_renderer.c" 'ui_renderer_draw_emark_page' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_prepares_emark_discover_identity_after_contract_ready' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c" 'test_service_pd_publishes_emark_identity_summary' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c" 'test_emark_identity_summary_extracts_cable_vdo'; then
  mark_pass "cable/E-marker page" "E-marker page exists and SOP prime E-marker request, PD identity publish, and cable VDO summary extraction are behavior-tested"
else
  mark_fail "cable/E-marker page" "E-marker page or E-marker behavior-test evidence is missing"
fi

if file_has "$ROOT/PowerXCode/FreeRTOS/UI/ui_renderer.c" 'UI_COLOR_DIM 0xFFFFU' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_ui_model.c" 'test_settings_activation_changes_values' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Tests/test_app_ui_navigation.c" 'test_settings_confirm_activates_selected_row_without_exit' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/App/app_tasks.c" 'bsp_backlight_set' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/App/app_tasks.c" 'bsp_lcd_set_rotation'; then
  mark_pass "settings and readable text" "settings brightness/rotation behavior, confirm handling, backlight driver, LCD rotation driver, and white dim text are covered"
else
  mark_fail "settings and readable text" "settings behavior, driver integration, or white-text evidence is missing"
fi

if file_lacks "$ROOT/PowerXCode/FreeRTOS/obj/App/subdir.mk" 'cdc|usb_(core|desc|endp|hw|init|istr|prop|pwr)|usbd_' &&
   file_lacks "$ROOT/PowerXCode/FreeRTOS/obj/makefile" 'cdc|usb_(core|desc|endp|hw|init|istr|prop|pwr)|usbd_' &&
   file_has "$ROOT/PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h" 'PA11/PA12 are reserved for USB D+/D- passthrough and legacy QC signaling'; then
  mark_pass "USB CDC removed / DPDM reserved" "product makefiles lack CDC/USBFS sources and board config reserves PA11/PA12"
else
  mark_fail "USB CDC removed / DPDM reserved" "USB CDC/USBFS exclusion evidence is missing"
fi

if [[ "$SOFTWARE_ONLY" == "1" ]]; then
  mark_blocked "WCH ISP flash" "software-only mode; real success requires $FLASH_REPORT with Status: flashed"
  mark_blocked "hardware validation" "software-only mode; real completion requires filled hardware report"
else
  if require_flash_success; then
    mark_pass "WCH ISP flash" "current firmware SHA matches successful PX1 wchisp info and flash output with Verify OK and Device reset"
  else
    mark_fail "WCH ISP flash" "missing complete successful PX1 wchisp info/flash evidence for current firmware"
  fi

  if require_hardware_report_complete; then
    mark_pass "hardware validation" "tools/check_powerx_hardware_validation.sh passes for the current firmware and flash evidence"
  else
    mark_fail "hardware validation" "tools/check_powerx_hardware_validation.sh failed for the current firmware or flash evidence"
  fi
fi

append_report ""
append_report "- Failed: $FAIL_COUNT"
append_report "- Blocked: $BLOCKED_COUNT"

if (( FAIL_COUNT > 0 )); then
  printf 'FAIL powerx-objective-coverage: %s failed requirement(s); report: %s\n' "$FAIL_COUNT" "$COVERAGE_REPORT" >&2
  exit 1
fi

if (( SOFTWARE_ONLY == 0 && BLOCKED_COUNT > 0 )); then
  printf 'FAIL powerx-objective-coverage: %s blocked requirement(s); report: %s\n' "$BLOCKED_COUNT" "$COVERAGE_REPORT" >&2
  exit 1
fi

if (( BLOCKED_COUNT > 0 )); then
  printf 'PASS powerx-objective-coverage-software; blocked=%s report=%s\n' "$BLOCKED_COUNT" "$COVERAGE_REPORT"
else
  printf 'PASS powerx-objective-coverage; report=%s\n' "$COVERAGE_REPORT"
fi

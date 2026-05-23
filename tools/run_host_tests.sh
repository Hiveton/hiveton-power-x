#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

COMMON=(
  -std=c99
  -Wall
  -Wextra
  -DPX1_HOST_TEST=1
  -IPowerXCode/FreeRTOS/Bsp/include
  -IPowerXCode/FreeRTOS/Service/include
  -IPowerXCode/FreeRTOS/UI/include
  -IPowerXCode/FreeRTOS/App/include
  -IPowerXCode/FreeRTOS/Assets
)

run_test() {
  local name="$1"
  shift
  cc "${COMMON[@]}" "$@" -o "/tmp/${name}"
  "/tmp/${name}"
  printf '%s\n' "PASS ${name}"
}

"$ROOT/tools/check_firmware_contract.sh"
"$ROOT/tools/test_ui_reference_delta_coverage.sh"
"$ROOT/tools/check_ui_preview_artifacts.sh"
"$ROOT/tools/check_ui_reference_delta.sh"

run_test test_measure_service \
  PowerXCode/FreeRTOS/Tests/test_measure_service.c \
  PowerXCode/FreeRTOS/Service/service_measure.c

run_test test_ui_model \
  PowerXCode/FreeRTOS/Tests/test_ui_model.c \
  PowerXCode/FreeRTOS/UI/ui_model.c

run_test test_ui_scope_model \
  PowerXCode/FreeRTOS/Tests/test_ui_scope_model.c

run_test test_protocol_snapshot \
  PowerXCode/FreeRTOS/Tests/test_protocol_snapshot.c \
  PowerXCode/FreeRTOS/Service/service_pd.c \
  PowerXCode/FreeRTOS/Service/service_emark.c

run_test test_ui_pages \
  PowerXCode/FreeRTOS/Tests/test_ui_pages.c \
  PowerXCode/FreeRTOS/UI/ui_pages.c \
  PowerXCode/FreeRTOS/UI/ui_model.c \
  PowerXCode/FreeRTOS/UI/ui_widgets.c \
  PowerXCode/FreeRTOS/Service/service_pd.c \
  PowerXCode/FreeRTOS/Service/service_emark.c

run_test test_app_protocol_arbiter \
  PowerXCode/FreeRTOS/Tests/test_app_protocol_arbiter.c \
  PowerXCode/FreeRTOS/App/app_protocol_arbiter.c \
  PowerXCode/FreeRTOS/Service/service_pd.c \
  PowerXCode/FreeRTOS/Service/service_emark.c

run_test test_app_trigger_control \
  PowerXCode/FreeRTOS/Tests/test_app_trigger_control.c \
  PowerXCode/FreeRTOS/App/app_trigger_control.c \
  PowerXCode/FreeRTOS/UI/ui_model.c

run_test test_app_ui_navigation \
  PowerXCode/FreeRTOS/Tests/test_app_ui_navigation.c \
  PowerXCode/FreeRTOS/App/app_ui_navigation.c \
  PowerXCode/FreeRTOS/UI/ui_model.c

run_test test_bsp_adc_config \
  PowerXCode/FreeRTOS/Tests/test_bsp_adc_config.c

run_test test_bsp_backlight \
  PowerXCode/FreeRTOS/Tests/test_bsp_backlight.c \
  PowerXCode/FreeRTOS/Bsp/bsp_backlight.c

run_test test_bsp_board_init \
  PowerXCode/FreeRTOS/Tests/test_bsp_board_init.c \
  PowerXCode/FreeRTOS/Bsp/bsp_board.c \
  PowerXCode/FreeRTOS/Bsp/bsp_backlight.c \
  PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c

run_test test_bsp_keys_polarity \
  PowerXCode/FreeRTOS/Tests/test_bsp_keys_polarity.c \
  PowerXCode/FreeRTOS/Bsp/bsp_keys.c

run_test test_bsp_lcd_rotation \
  PowerXCode/FreeRTOS/Tests/test_bsp_lcd_rotation.c \
  PowerXCode/FreeRTOS/Bsp/bsp_lcd_st7735.c

run_test test_legacy_charge_and_emark \
  PowerXCode/FreeRTOS/Tests/test_legacy_charge_and_emark.c \
  PowerXCode/FreeRTOS/Service/service_legacy_charge.c \
  PowerXCode/FreeRTOS/Service/service_emark.c \
  PowerXCode/FreeRTOS/Bsp/bsp_dpdm.c \
  PowerXCode/FreeRTOS/Bsp/bsp_cc_ext_rd.c \
  PowerXCode/FreeRTOS/Service/service_pd.c

run_test test_ui_value_format \
  PowerXCode/FreeRTOS/Tests/test_ui_value_format.c

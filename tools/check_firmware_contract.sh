#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"

fail() {
  printf 'FAIL firmware-contract: %s\n' "$1" >&2
  exit 1
}

require_define() {
  local file="$1"
  local name="$2"
  local value="$3"

  if ! grep -Eq "^#define[[:space:]]+${name}[[:space:]]+${value}([[:space:]]|$)" "$file"; then
    fail "${name} must stay ${value} in ${file}"
  fi
}

ensure_no_match() {
  local description="$1"
  shift

  local matches
  if matches="$("$@" 2>/dev/null)" && [[ -n "$matches" ]]; then
    printf '%s\n' "$matches" >&2
    fail "$description"
  fi
}

require_match() {
  local description="$1"
  shift

  local matches
  if ! matches="$("$@" 2>/dev/null)" || [[ -z "$matches" ]]; then
    fail "$description"
  fi
}

APP_TASKS="PowerXCode/FreeRTOS/App/app_tasks.c"
BOARD_CONFIG="PowerXCode/FreeRTOS/Bsp/include/bsp_board_config.h"

require_define "$APP_TASKS" "PX1_ENABLE_MEASURE_TASK" "1"
require_define "$APP_TASKS" "PX1_ENABLE_PROTOCOL_TASK" "1"
require_define "$APP_TASKS" "PX1_ENABLE_LEGACY_TASK" "1"

require_define "$APP_TASKS" "PX1_KEY_DEBUG_SCREEN" "0"
require_define "$APP_TASKS" "PX1_MINIMAL_HEARTBEAT_DIAG" "0"
require_define "$APP_TASKS" "PX1_BUSINESS_HEARTBEAT_DIAG" "0"
require_define "$APP_TASKS" "PX1_BUSINESS_STAGE_DIAG" "0"

require_match \
  "Product build must include protocol arbiter and UI navigation app modules" \
  rg -n \
  "app_protocol_arbiter\.c|app_ui_navigation\.c|app_trigger_control\.c" \
  PowerXCode/FreeRTOS/obj/App/subdir.mk

require_match \
  "Product build must include USBPD, DP/DM, keys, and LCD BSP modules" \
  rg -n \
  "bsp_usbpd_port\.c|bsp_dpdm\.c|bsp_keys\.c|bsp_lcd_st7735\.c" \
  PowerXCode/FreeRTOS/obj/Bsp/subdir.mk

require_match \
  "PD task must handle pending PD requests and SOP prime E-marker discovery" \
  rg -n \
  "service_pd_prepare_pending_request|bsp_usbpd_port_transmit_sop_prime|service_pd_prepare_emark_identity_request" \
  "$APP_TASKS"

require_match \
  "Legacy/QC task must poll DP/DM protocol service and publish snapshots" \
  rg -n \
  "service_legacy_charge_poll|APP_PROTOCOL_SOURCE_LEGACY|app_publish_protocol_snapshot" \
  "$APP_TASKS"

require_match \
  "UI task must poll keys, route navigation, and apply brightness/rotation settings" \
  rg -n \
  "bsp_keys_poll|app_ui_navigation_apply|bsp_backlight_set|bsp_lcd_set_rotation" \
  "$APP_TASKS"

require_match \
  "main must initialize board hardware before creating application tasks" \
  perl -0ne \
  'print if /bsp_board_init\(\);\s*app_controller_init\(\);\s*app_tasks_create\(\);/s' \
  PowerXCode/FreeRTOS/User/main.c

require_match \
  "Navigation must route trigger, PDO, and QC confirmations to dedicated control paths" \
  rg -n \
  "app_trigger_control_apply_auto|app_trigger_control_apply_pd|app_trigger_control_apply_qc" \
  PowerXCode/FreeRTOS/App/app_ui_navigation.c

require_match \
  "Keys must keep interrupt setup for BTN2 high-active and BTN1/BTN3 low-active paths" \
  rg -n \
  "EXTI_Mode_Interrupt|EXTI_Trigger_Rising|EXTI_Trigger_Falling|bsp_keys_record_irq_mask" \
  PowerXCode/FreeRTOS/Bsp/bsp_keys.c

ensure_no_match \
  "Board ADC/INA226 config must not carry placeholder calibration wording" \
  rg -n \
  "(placeholder|Fill these|Default 1:1|current reporting disabled|confirmed divider / gain)" \
  "$BOARD_CONFIG"

ensure_no_match \
  "Product UI renderer must not expose placeholder page rendering after all 9 pages are implemented" \
  rg -n \
  "ui_renderer_draw_placeholder_page|placeholder_page" \
  PowerXCode/FreeRTOS/UI

require_match \
  "Key debug renderer declaration must be hidden behind PX1_ENABLE_KEY_DEBUG_RENDERER or PX1_HOST_TEST" \
  perl -0ne \
  'print if /#if[^\n]*(PX1_ENABLE_KEY_DEBUG_RENDERER|PX1_HOST_TEST)[\s\S]{0,500}ui_renderer_draw_key_debug_page[\s\S]{0,500}#endif/s' \
  PowerXCode/FreeRTOS/UI/include/ui_renderer.h

require_match \
  "Key debug renderer implementation must be hidden behind PX1_ENABLE_KEY_DEBUG_RENDERER or PX1_HOST_TEST" \
  perl -0ne \
  'print if /#if[^\n]*(PX1_ENABLE_KEY_DEBUG_RENDERER|PX1_HOST_TEST)[\s\S]{0,600}ui_renderer_draw_key_debug_page[\s\S]{0,8000}#endif/s' \
  PowerXCode/FreeRTOS/UI/ui_renderer.c

require_match \
  "Key debug state declaration must be hidden behind PX1_ENABLE_KEY_DEBUG_STATE or PX1_HOST_TEST" \
  perl -0ne \
  'print if /#if[^\n]*(PX1_ENABLE_KEY_DEBUG_STATE|PX1_HOST_TEST)[\s\S]{0,500}bsp_keys_get_debug_state[\s\S]{0,500}#endif/s' \
  PowerXCode/FreeRTOS/Bsp/include/bsp_keys.h

require_match \
  "Key debug state implementation must be hidden behind PX1_ENABLE_KEY_DEBUG_STATE or PX1_HOST_TEST" \
  perl -0ne \
  'print if /#if[^\n]*(PX1_ENABLE_KEY_DEBUG_STATE|PX1_HOST_TEST)[\s\S]{0,700}bsp_keys_get_debug_state[\s\S]{0,2500}#endif/s' \
  PowerXCode/FreeRTOS/Bsp/bsp_keys.c

ensure_no_match \
  "Public BSP headers must expose host-test mock APIs only through PX1_HOST_TEST, not broad !__riscv guards" \
  rg -n \
  "#if[[:space:]]+!defined\\(__riscv\\)" \
  PowerXCode/FreeRTOS/Bsp/include/bsp_dpdm.h \
  PowerXCode/FreeRTOS/Bsp/include/bsp_keys.h

ensure_no_match \
  "USB CDC/USBFS source must not be part of the product make build" \
  grep -E \
  "(ch32l103_usb|usb_(core|desc|endp|hw|init|istr|prop|pwr)|usbd_|cdc)" \
  PowerXCode/FreeRTOS/obj/*.mk \
  PowerXCode/FreeRTOS/obj/*/subdir.mk \
  PowerXCode/FreeRTOS/obj/*/*/subdir.mk

ensure_no_match \
  "Product code must not enable or handle USBFS/CDC; PA11/PA12 stay for QC and USB pass-through" \
  rg -n \
  "(RCC_HBPeriph_USBFS|RCC_USBFS|USBFSD|USBFSH|USBFS_IRQn|USBFSWakeUp_IRQn|USBFS_U[A-Z0-9_]*|CDC_[A-Z0-9_]+|USBD_[A-Z0-9_]+|USBFS_IRQHandler)" \
  PowerXCode/FreeRTOS/App \
  PowerXCode/FreeRTOS/Bsp \
  PowerXCode/FreeRTOS/Service \
  PowerXCode/FreeRTOS/UI \
  PowerXCode/FreeRTOS/User

printf '%s\n' "PASS firmware-contract"

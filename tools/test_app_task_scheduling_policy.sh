#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_TASKS="$ROOT/PowerXCode/FreeRTOS/App/app_tasks.c"

fail() {
  printf 'FAIL app-task-scheduling-policy: %s\n' "$1" >&2
  exit 1
}

require_match() {
  local desc="$1"
  shift
  "$@" >/dev/null || fail "$desc"
}

require_match \
  "UI refresh must be page-aware instead of always forcing redraw every loop" \
  rg -n "app_ui_refresh_period_ms\\(|last_redraw_tick" "$APP_TASKS"

require_match \
  "UI must not run above measurement/protocol tasks because full-screen LCD draws can starve live sampling" \
  rg -n "APP_TASK_PRIORITY_PROTOCOL[[:space:]]+\\(tskIDLE_PRIORITY \\+ 3U\\)|APP_TASK_PRIORITY_MEASURE[[:space:]]+\\(tskIDLE_PRIORITY \\+ 3U\\)|APP_TASK_PRIORITY_UI[[:space:]]+\\(tskIDLE_PRIORITY \\+ 2U\\)" "$APP_TASKS"

require_match \
  "live pages should refresh at the same 20ms cadence as the UI poll loop after LCD streaming is optimized" \
  rg -n "#define UI_TASK_LIVE_REFRESH_MS 20U" "$APP_TASKS"

require_match \
  "The old zero-tick refresh loop must not remain" \
  bash -c "! rg -n '#define UI_TASK_REFRESH_TICKS 0U|#define UI_TASK_DPDM_REFRESH_TICKS 0U' '$APP_TASKS'"

printf '%s\n' "PASS app-task-scheduling-policy"

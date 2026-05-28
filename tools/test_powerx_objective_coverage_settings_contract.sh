#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

fail() {
  printf 'FAIL objective-coverage-settings-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing objective coverage checker: $CHECKER"

rg -q 'test_settings_activation_changes_values' "$CHECKER" \
  || fail "objective coverage does not require brightness/rotation setting behavior test"

rg -q 'test_settings_confirm_activates_selected_row_without_exit' "$CHECKER" \
  || fail "objective coverage does not require navigation-level settings confirm behavior test"

rg -q 'bsp_backlight_set' "$CHECKER" \
  || fail "objective coverage does not require settings to reach the backlight driver"

rg -q 'bsp_lcd_set_rotation' "$CHECKER" \
  || fail "objective coverage does not require settings to reach the LCD rotation driver"

printf '%s\n' "PASS objective-coverage-settings-contract-test"

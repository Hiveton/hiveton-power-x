#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

fail() {
  printf 'FAIL objective-coverage-key-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing objective coverage checker: $CHECKER"

rg -q 'test_btn1_poll_fallback_emits_short_event_on_release' "$CHECKER" \
  || fail "objective coverage does not require BTN1 scan fallback behavior test"

rg -q 'BTN2/BTN3.*EXTI.*BTN1.*fallback|BTN1.*fallback.*BTN2/BTN3.*EXTI' "$CHECKER" \
  || fail "objective coverage evidence does not name the real EXTI/fallback key contract"

printf '%s\n' "PASS objective-coverage-key-contract-test"

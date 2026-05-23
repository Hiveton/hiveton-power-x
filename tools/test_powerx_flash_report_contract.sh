#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
FLASH_SCRIPT="$ROOT/tools/flash_powerx.sh"
GATE="$ROOT/tools/run_powerx_goal_gate.sh"
HARDWARE_CHECKER="$ROOT/tools/check_powerx_hardware_validation.sh"

fail() {
  printf 'FAIL flash-report-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$FLASH_SCRIPT" ]] || fail "missing flash script: $FLASH_SCRIPT"
[[ -s "$GATE" ]] || fail "missing goal gate: $GATE"
[[ -s "$HARDWARE_CHECKER" ]] || fail "missing hardware checker: $HARDWARE_CHECKER"

required_flash_patterns=(
  'WCHISP_INFO_STATUS=""'
  'WCHISP_FLASH_OUTPUT=""'
  'WCHISP_FLASH_STATUS=""'
  'wchisp info exit:'
  '## wchisp flash'
  'wchisp flash exit:'
  'WCHISP_FLASH_OUTPUT="$("$WCHISP_BIN" flash "$BIN" 2>&1)"'
  'printf '\''%s\n'\'' "$WCHISP_FLASH_OUTPUT"'
  'cp "$FLASH_REPORT" "$FLASH_SUCCESS_REPORT"'
  '[[ "$status" == "flashed" ]]'
)

for pattern in "${required_flash_patterns[@]}"; do
  rg -Fq -- "$pattern" "$FLASH_SCRIPT" \
    || fail "flash script is missing required report behavior: $pattern"
done

rg -Fq '## wchisp flash' "$HARDWARE_CHECKER" \
  || fail "hardware checker does not require raw wchisp flash output section"

rg -Fq 'Verify OK' "$HARDWARE_CHECKER" \
  || fail "hardware checker does not require verify evidence"

rg -Fq 'Device reset' "$HARDWARE_CHECKER" \
  || fail "hardware checker does not require reset evidence"

rg -Fq 'tools/test_powerx_flash_report_contract.sh' "$GATE" \
  || fail "run_powerx_goal_gate.sh does not execute this flash report contract self-test"

rg -Fq 'tools/test_powerx_flash_script_no_isp_fake_wchisp.sh' "$GATE" \
  || fail "run_powerx_goal_gate.sh does not execute no-ISP fake wchisp self-test"

printf '%s\n' "PASS flash-report-contract-test"

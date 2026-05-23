#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_goal_completion.sh"
GATE="$ROOT/tools/run_powerx_goal_gate.sh"

fail() {
  printf 'FAIL goal-completion-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing completion checker: $CHECKER"
[[ -s "$GATE" ]] || fail "missing goal gate: $GATE"

required_completion_fixed_patterns=(
  'run_powerx_goal_gate.sh" --no-flash'
  'check_powerx_objective_coverage.sh" --software-only'
  'check_powerx_objective_coverage.sh" "$HARDWARE_REPORT"'
  'check_powerx_hardware_validation.sh" "$HARDWARE_REPORT" "$BIN"'
  'artifacts/flash/latest-success.md'
  'Firmware SHA256: \`$CURRENT_SHA\`'
  'Status:[[:space:]]*`flashed`'
  'wchisp info exit: `0`'
  'wchisp flash exit: `0`'
  'missing or empty file'
)

for pattern in "${required_completion_fixed_patterns[@]}"; do
  rg -Fq "$pattern" "$CHECKER" \
    || fail "completion checker is missing required evidence gate: $pattern"
done

rg -Fq 'tools/test_powerx_goal_completion_contract.sh' "$GATE" \
  || fail "run_powerx_goal_gate.sh does not execute this completion contract self-test"

rg -Fq 'tools/test_powerx_goal_completion_flash_evidence_contract.sh' "$GATE" \
  || fail "run_powerx_goal_gate.sh does not execute flash evidence completion contract self-test"

printf '%s\n' "PASS goal-completion-contract-test"

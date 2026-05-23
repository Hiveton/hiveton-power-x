#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_goal_completion.sh"
GATE="$ROOT/tools/run_powerx_goal_gate.sh"

fail() {
  printf 'FAIL goal-completion-flash-evidence-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing completion checker: $CHECKER"
[[ -s "$GATE" ]] || fail "missing goal gate: $GATE"

required_completion_flash_patterns=(
  '## wchisp info'
  'CH32L103K8U6'
  'wchisp info exit: `0`'
  '## wchisp flash'
  'wchisp flash exit: `0`'
  'Verify OK'
  'Device reset'
)

for pattern in "${required_completion_flash_patterns[@]}"; do
  rg -Fq -- "$pattern" "$CHECKER" \
    || fail "completion checker does not require flash evidence: $pattern"
done

rg -Fq 'tools/test_powerx_goal_completion_flash_evidence_contract.sh' "$GATE" \
  || fail "run_powerx_goal_gate.sh does not execute this flash evidence contract self-test"

printf '%s\n' "PASS goal-completion-flash-evidence-contract-test"

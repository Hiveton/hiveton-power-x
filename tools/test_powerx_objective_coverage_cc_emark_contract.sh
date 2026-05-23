#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

fail() {
  printf 'FAIL objective-coverage-cc-emark-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing objective coverage checker: $CHECKER"

rg -q 'test_service_pd_prepares_emark_discover_identity_after_contract_ready' "$CHECKER" \
  || fail "objective coverage does not require SOP prime E-marker discover request behavior test"

rg -q 'test_service_pd_publishes_emark_identity_summary' "$CHECKER" \
  || fail "objective coverage does not require PD E-marker identity publish behavior test"

rg -q 'test_emark_identity_summary_extracts_cable_vdo' "$CHECKER" \
  || fail "objective coverage does not require cable VDO summary extraction behavior test"

printf '%s\n' "PASS objective-coverage-cc-emark-contract-test"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

fail() {
  printf 'FAIL objective-coverage-protocol-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing objective coverage checker: $CHECKER"

required_arbiter_tests=(
  "test_pd_none_does_not_clear_legacy_qc"
  "test_pd_snapshot_has_priority_over_legacy_qc"
  "test_cc_attached_pd_path_hides_passive_legacy_qc_available"
  "test_pd_cc_detach_clears_hidden_passive_legacy_qc_available"
)

for test_name in "${required_arbiter_tests[@]}"; do
  rg -Fq "$test_name" "$CHECKER" \
    || fail "objective coverage does not require protocol arbitration behavior test: $test_name"
done

required_vbus_tests=(
  "test_vbus_without_protocol_renders_as_other"
  "test_elevated_vbus_without_pd_does_not_guess_qc"
  "test_vbus_with_cc_attached_does_not_fallback_to_other"
  "test_vbus_with_cc_orientation_does_not_fallback_to_other"
)

for test_name in "${required_vbus_tests[@]}"; do
  rg -Fq "$test_name" "$CHECKER" \
    || fail "objective coverage does not require VBUS fallback behavior test: $test_name"
done

printf '%s\n' "PASS objective-coverage-protocol-contract-test"

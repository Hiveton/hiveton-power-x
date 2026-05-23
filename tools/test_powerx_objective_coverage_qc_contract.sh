#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

required_patterns=(
  "test_legacy_charge_request_drives_dpdm_mode"
  "test_legacy_charge_records_requested_voltage_and_qc3_steps"
  "test_legacy_charge_poll_reports_qc_state_and_dpdm_voltage"
  "test_legacy_charge_qc3_voltage_uses_200mv_steps"
  "test_legacy_charge_timeout_releases_dpdm_request_levels"
  "test_qc_trigger_uses_legacy_service"
  "test_qc_non_fixed_target_uses_qc3"
  "test_qc_page_forces_legacy_qc_request"
  "test_qc_page_uses_qc3_for_15v_target"
)

for pattern in "${required_patterns[@]}"; do
  if ! rg -Fq "$pattern" "$CHECKER"; then
    printf 'FAIL: objective coverage checker is missing QC contract evidence: %s\n' "$pattern" >&2
    exit 1
  fi
done

printf 'PASS: objective coverage checker requires QC behavior-test evidence\n'

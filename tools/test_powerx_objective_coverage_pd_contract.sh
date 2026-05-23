#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

required_patterns=(
  "test_service_pd_requests_default_pdo_after_source_capabilities"
  "test_service_pd_selects_fixed_pdo_and_updates_contract"
  "test_service_pd_waits_for_vbus_measurement_after_ps_rdy"
  "test_service_pd_requests_pps_apdo_when_target_matches_pps_range"
  "test_pdo_page_pd_request_uses_fine_target"
)

for pattern in "${required_patterns[@]}"; do
  if ! rg -Fq "$pattern" "$CHECKER"; then
    printf 'FAIL: objective coverage checker is missing PD contract evidence: %s\n' "$pattern" >&2
    exit 1
  fi
done

printf 'PASS: objective coverage checker requires PD behavior-test evidence\n'

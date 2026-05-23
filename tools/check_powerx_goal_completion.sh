#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
UI_REPORT="$ROOT/artifacts/ui-preview/current-ui-report.md"
FLASH_ATTEMPT_REPORT="${PX1_FLASH_ATTEMPT_REPORT:-$ROOT/artifacts/flash/latest.md}"
FLASH_REPORT="${PX1_FLASH_REPORT:-$ROOT/artifacts/flash/latest-success.md}"
ISP_PREFLIGHT_REPORT="${PX1_ISP_PREFLIGHT_REPORT:-$ROOT/artifacts/flash/isp-preflight.md}"
HARDWARE_REPORT="${1:-$ROOT/artifacts/hardware-validation/latest.md}"
GOAL_REPORT_DIR="$ROOT/artifacts/goal"
GOAL_REPORT="$GOAL_REPORT_DIR/latest.md"
OBJECTIVE_COVERAGE_REPORT="$GOAL_REPORT_DIR/objective-coverage.md"
STATUS_SOFTWARE="pending"
STATUS_OBJECTIVE="pending"
STATUS_UI="pending"
STATUS_FLASH="pending"
STATUS_HARDWARE="pending"

fail() {
  write_goal_report "failed" "$1"
  printf 'FAIL powerx-goal-completion: %s\n' "$1" >&2
  exit 1
}

step() {
  printf '\n== %s ==\n' "$1"
}

require_file() {
  local path="$1"

  [[ -s "$path" ]] || fail "missing or empty file: $path"
}

require_report_page() {
  local page="$1"

  grep -Eq "^[|][[:space:]]*${page}[[:space:]]*[|][[:space:]]*160x80[[:space:]]*[|]" "$UI_REPORT" \
    || fail "UI report does not contain a 160x80 row for page: $page"
}

read_flash_status() {
  awk -F'`' '/Status:/ { print $2; exit }' "$FLASH_REPORT" 2>/dev/null || true
}

read_flash_attempt_status() {
  awk -F'`' '/Status:/ { print $2; exit }' "$FLASH_ATTEMPT_REPORT" 2>/dev/null || true
}

read_isp_preflight_status() {
  awk -F'`' '/Status:/ { print $2; exit }' "$ISP_PREFLIGHT_REPORT" 2>/dev/null || true
}

read_report_section_first_line() {
  local report="$1"
  local section="$2"

  awk -v section="## $section" '
    $0 == section {
      in_section=1
      next
    }
    in_section && /^## / {
      exit
    }
    in_section && NF {
      print
      exit
    }
  ' "$report" 2>/dev/null || true
}

write_goal_report() {
  local status="$1"
  local message="$2"
  local sha
  local flash_status
  local flash_attempt_status
  local isp_preflight_status
  local isp_preflight_diagnosis

  mkdir -p "$GOAL_REPORT_DIR"
  sha="unknown"
  if [[ -s "$BIN" ]]; then
    sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
  fi
  flash_status="$(read_flash_status)"
  [[ -n "$flash_status" ]] || flash_status="missing"
  flash_attempt_status="$(read_flash_attempt_status)"
  [[ -n "$flash_attempt_status" ]] || flash_attempt_status="missing"
  isp_preflight_status="$(read_isp_preflight_status)"
  [[ -n "$isp_preflight_status" ]] || isp_preflight_status="missing"
  isp_preflight_diagnosis="$(read_report_section_first_line "$ISP_PREFLIGHT_REPORT" "USB Host Diagnosis")"
  [[ -n "$isp_preflight_diagnosis" ]] || isp_preflight_diagnosis="missing"

  {
    printf '# PX1 Goal Completion Audit\n\n'
    printf '%s\n' "- Time: $(date '+%Y-%m-%d %H:%M:%S %z')"
    printf '%s\n' "- Overall status: \`$status\`"
    printf '%s\n' "- Message: $message"
    printf '%s\n' "- Firmware SHA256: \`$sha\`"
    printf '%s\n' "- ISP preflight status: \`$isp_preflight_status\`"
    printf '%s\n' "- ISP preflight diagnosis: $isp_preflight_diagnosis"
    printf '%s\n' "- Flash success status: \`$flash_status\`"
    printf '%s\n' "- Latest flash attempt status: \`$flash_attempt_status\`"
    printf '\n## Evidence\n\n'
    printf '%s\n' "- Firmware: \`$BIN\`"
    printf '%s\n' "- UI report: \`$UI_REPORT\`"
    printf '%s\n' "- Objective coverage report: \`$OBJECTIVE_COVERAGE_REPORT\`"
    printf '%s\n' "- ISP preflight report: \`$ISP_PREFLIGHT_REPORT\`"
    printf '%s\n' "- Flash success report: \`$FLASH_REPORT\`"
    printf '%s\n' "- Latest flash attempt report: \`$FLASH_ATTEMPT_REPORT\`"
    printf '%s\n' "- Hardware report: \`$HARDWARE_REPORT\`"
    printf '\n## Checklist\n\n'
    printf '| item | status |\n'
    printf '| --- | --- |\n'
    printf '| no-flash software gate | %s |\n' "$STATUS_SOFTWARE"
    printf '| objective coverage audit | %s |\n' "$STATUS_OBJECTIVE"
    printf '| 9-page UI preview evidence | %s |\n' "$STATUS_UI"
    printf '| WCH ISP flash evidence | %s |\n' "$STATUS_FLASH"
    printf '| hardware validation report | %s |\n' "$STATUS_HARDWARE"
  } >"$GOAL_REPORT"
}

step "PX1 no-flash software gate"
if "$ROOT/tools/run_powerx_goal_gate.sh" --no-flash; then
  STATUS_SOFTWARE="pass"
else
  STATUS_SOFTWARE="fail"
  fail "no-flash software gate failed"
fi

step "PX1 objective coverage audit (software)"
STATUS_OBJECTIVE="fail"
if "$ROOT/tools/check_powerx_objective_coverage.sh" --software-only "$HARDWARE_REPORT"; then
  STATUS_OBJECTIVE="blocked"
else
  STATUS_OBJECTIVE="fail"
  fail "objective coverage audit failed"
fi

step "PX1 UI preview evidence"
STATUS_UI="fail"
require_file "$UI_REPORT"
for page in main protocol trigger cc cable settings scope pdo qc; do
  require_report_page "$page"
done
STATUS_UI="pass"
printf '%s\n' "PASS ui-preview-evidence"

step "PX1 flash evidence"
STATUS_FLASH="fail"
require_file "$BIN"
require_file "$FLASH_REPORT"
CURRENT_SHA="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
grep -Fq "Firmware SHA256: \`$CURRENT_SHA\`" "$FLASH_REPORT" \
  || fail "flash evidence SHA does not match current firmware: $CURRENT_SHA"
grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`flashed`[[:space:]]*$' "$FLASH_REPORT" \
  || fail "flash evidence is not successful: $FLASH_REPORT"
grep -Fq '## wchisp info' "$FLASH_REPORT" \
  || fail "flash evidence is missing wchisp info output: $FLASH_REPORT"
grep -Fq 'CH32L103K8U6' "$FLASH_REPORT" \
  || fail "flash evidence is missing PX1 CH32L103K8U6 target chip output: $FLASH_REPORT"
grep -Fq 'wchisp info exit: `0`' "$FLASH_REPORT" \
  || fail "flash evidence is missing successful wchisp info exit code: $FLASH_REPORT"
grep -Fq '## wchisp flash' "$FLASH_REPORT" \
  || fail "flash evidence is missing raw wchisp flash output: $FLASH_REPORT"
grep -Fq 'wchisp flash exit: `0`' "$FLASH_REPORT" \
  || fail "flash evidence is missing successful wchisp flash exit code: $FLASH_REPORT"
grep -Fq 'Verify OK' "$FLASH_REPORT" \
  || fail "flash evidence is missing Verify OK output: $FLASH_REPORT"
grep -Fq 'Device reset' "$FLASH_REPORT" \
  || fail "flash evidence is missing Device reset output: $FLASH_REPORT"
STATUS_FLASH="pass"
printf '%s\n' "PASS flash-evidence"

step "PX1 hardware validation"
STATUS_HARDWARE="fail"
if "$ROOT/tools/check_powerx_hardware_validation.sh" "$HARDWARE_REPORT" "$BIN"; then
  STATUS_HARDWARE="pass"
else
  STATUS_HARDWARE="fail"
  fail "hardware validation report failed"
fi

step "PX1 objective coverage audit (full)"
STATUS_OBJECTIVE="fail"
if "$ROOT/tools/check_powerx_objective_coverage.sh" "$HARDWARE_REPORT"; then
  STATUS_OBJECTIVE="pass"
else
  STATUS_OBJECTIVE="fail"
  fail "full objective coverage audit failed"
fi

write_goal_report "passed" "All PX1 goal completion checks passed."
printf '\n%s\n' "PASS powerx-goal-completion"

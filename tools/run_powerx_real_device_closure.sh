#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
HARDWARE_REPORT="$ROOT/artifacts/hardware-validation/latest.md"
CLOSURE_REPORT_DIR="$ROOT/artifacts/goal"
CLOSURE_REPORT="$CLOSURE_REPORT_DIR/real-device-closure.md"
WAIT_SECONDS="${PX1_FLASH_WAIT_SECONDS:-30}"
WAIT_FOREVER=0
SKIP_FLASH=0
FORCE_REPORT=0
CLOSURE_SOFTWARE="pending"
CLOSURE_OBJECTIVE="pending"
CLOSURE_FLASH="pending"
CLOSURE_HARDWARE_REPORT="pending"
CLOSURE_FINAL_AUDIT="pending"

usage() {
  cat <<'MSG'
Usage: tools/run_powerx_real_device_closure.sh [--wait seconds] [--wait-forever] [--hardware-report report.md] [--force-report] [--skip-flash]

Runs the PX1 real-device closure flow:
  1. no-flash software gate and cross build
  2. objective coverage audit for all software-verifiable requirements
  3. WCH ISP flash of the current FreeRTOS.bin, unless --skip-flash is used
  4. hardware validation report preparation, if missing or --force-report is used
  5. final goal completion audit

The final audit still requires a filled real hardware report. This script does
not fake hardware evidence; it only prepares the report template after a
successful flash.

If an existing report does not match the current firmware SHA, the script stops
unless --force-report is provided. This protects filled real evidence from
accidental overwrite.

--skip-flash is a dry run. It writes a blocked closure report and exits 2 so
automation cannot confuse software-only readiness with real-device completion.
MSG
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --wait)
      [[ "${2:-}" =~ ^[0-9]+$ ]] || {
        printf 'Invalid --wait value: %s\n' "${2:-}" >&2
        exit 64
      }
      WAIT_SECONDS="$2"
      shift 2
      ;;
    --hardware-report)
      [[ -n "${2:-}" ]] || {
        printf 'Missing --hardware-report path\n' >&2
        exit 64
      }
      HARDWARE_REPORT="$2"
      shift 2
      ;;
    --wait-forever)
      WAIT_FOREVER=1
      WAIT_SECONDS="forever"
      shift
      ;;
    --force-report)
      FORCE_REPORT=1
      shift
      ;;
    --skip-flash)
      SKIP_FLASH=1
      shift
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 64
      ;;
  esac
done

step() {
  printf '\n== %s ==\n' "$1"
}

relative_or_original() {
  local path="$1"

  if [[ "$path" == "$ROOT/"* ]]; then
    printf '%s\n' "${path#"$ROOT/"}"
  else
    printf '%s\n' "$path"
  fi
}

read_report_status() {
  local report="$1"

  awk -F'`' '/Status:/ { print $2; exit }' "$report" 2>/dev/null || true
}

read_report_sha() {
  local report="$1"

  awk '
    /Firmware SHA256:/ {
      line=$0
      gsub("`", "", line)
      sub(/^.*Firmware SHA256:[[:space:]]*/, "", line)
      sub(/[[:space:]].*$/, "", line)
      print line
      exit
    }
  ' "$report" 2>/dev/null || true
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

detect_hardware_report_status() {
  local sha="unknown"

  if [[ -s "$BIN" ]]; then
    sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
  fi

  if [[ ! -e "$HARDWARE_REPORT" ]]; then
    printf 'missing\n'
  elif [[ ! -s "$HARDWARE_REPORT" ]]; then
    printf 'empty\n'
  elif [[ "$sha" != "unknown" ]] &&
       grep -Fq "Firmware SHA256: $sha" "$HARDWARE_REPORT"; then
    printf 'current-template\n'
  elif [[ "$sha" != "unknown" ]]; then
    printf 'stale\n'
  else
    printf 'unknown\n'
  fi
}

write_closure_report() {
  local status="$1"
  local message="$2"
  local sha="unknown"
  local latest_flash="$ROOT/artifacts/flash/latest.md"
  local latest_success="$ROOT/artifacts/flash/latest-success.md"
  local isp_preflight="$ROOT/artifacts/flash/isp-preflight.md"
  local latest_flash_status
  local latest_flash_sha
  local latest_success_status
  local latest_success_sha
  local isp_preflight_status
  local isp_preflight_diagnosis
  local hardware_report_status

  mkdir -p "$CLOSURE_REPORT_DIR"
  if [[ -s "$BIN" ]]; then
    sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
  fi
  latest_flash_status="$(read_report_status "$latest_flash")"
  [[ -n "$latest_flash_status" ]] || latest_flash_status="missing"
  latest_flash_sha="$(read_report_sha "$latest_flash")"
  [[ -n "$latest_flash_sha" ]] || latest_flash_sha="missing"
  if [[ "$latest_flash_status" != "missing" &&
        "$latest_flash_sha" != "missing" &&
        "$sha" != "unknown" &&
        "$latest_flash_sha" != "$sha" ]]; then
    latest_flash_status="stale"
  fi
  latest_success_status="$(read_report_status "$latest_success")"
  [[ -n "$latest_success_status" ]] || latest_success_status="missing"
  latest_success_sha="$(read_report_sha "$latest_success")"
  [[ -n "$latest_success_sha" ]] || latest_success_sha="missing"
  if [[ "$latest_success_status" == "flashed" &&
        "$sha" != "unknown" &&
        "$latest_success_sha" != "$sha" ]]; then
    latest_success_status="stale"
  fi
  isp_preflight_status="$(read_report_status "$isp_preflight")"
  [[ -n "$isp_preflight_status" ]] || isp_preflight_status="missing"
  isp_preflight_diagnosis="$(read_report_section_first_line "$isp_preflight" "USB Host Diagnosis")"
  [[ -n "$isp_preflight_diagnosis" ]] || isp_preflight_diagnosis="missing"
  hardware_report_status="$CLOSURE_HARDWARE_REPORT"
  if [[ "$hardware_report_status" == "pending" ]]; then
    hardware_report_status="$(detect_hardware_report_status)"
  fi

  {
    printf '# PX1 Real Device Closure\n\n'
    printf '%s\n' "- Time: $(date '+%Y-%m-%d %H:%M:%S %z')"
    printf '%s\n' "- Overall status: \`$status\`"
    printf '%s\n' "- Message: $message"
    printf '%s\n' "- Firmware: \`$(relative_or_original "$BIN")\`"
    printf '%s\n' "- Firmware SHA256: \`$sha\`"
    printf '%s\n' "- Hardware report: \`$(relative_or_original "$HARDWARE_REPORT")\`"
    printf '%s\n' "- Latest flash attempt: \`$(relative_or_original "$latest_flash")\`"
    printf '%s\n' "- Latest flash attempt SHA256: \`$latest_flash_sha\`"
    printf '%s\n' "- Latest flash attempt status: \`$latest_flash_status\`"
    printf '%s\n' "- ISP preflight report: \`$(relative_or_original "$isp_preflight")\`"
    printf '%s\n' "- ISP preflight status: \`$isp_preflight_status\`"
    printf '%s\n' "- ISP preflight diagnosis: $isp_preflight_diagnosis"
    printf '%s\n' "- Flash success report: \`$(relative_or_original "$latest_success")\`"
    printf '%s\n' "- Flash success SHA256: \`$latest_success_sha\`"
    printf '%s\n' "- Flash success status: \`$latest_success_status\`"
    printf '\n## Checklist\n\n'
    printf '| item | status |\n'
    printf '| --- | --- |\n'
    printf '| no-flash software gate | %s |\n' "$CLOSURE_SOFTWARE"
    printf '| objective coverage audit | %s |\n' "$CLOSURE_OBJECTIVE"
    printf '| WCH ISP flash | %s |\n' "$CLOSURE_FLASH"
    printf '| hardware report preparation | %s |\n' "$hardware_report_status"
    printf '| final goal audit | %s |\n' "$CLOSURE_FINAL_AUDIT"
  } >"$CLOSURE_REPORT"
}

refresh_isp_preflight() {
  if [[ -x "$ROOT/tools/check_wch_isp_device.sh" ]]; then
    "$ROOT/tools/check_wch_isp_device.sh" --wait 0 >/dev/null 2>&1 || true
  fi
}

step "PX1 software gate"
if "$ROOT/tools/run_powerx_goal_gate.sh" --no-flash; then
  CLOSURE_SOFTWARE="pass"
else
  status=$?
  CLOSURE_SOFTWARE="fail"
  write_closure_report "failed" "PX1 no-flash software gate failed."
  exit "$status"
fi

step "PX1 objective coverage"
if "$ROOT/tools/check_powerx_objective_coverage.sh" --software-only "$HARDWARE_REPORT"; then
  CLOSURE_OBJECTIVE="software-pass-blocked-hardware"
else
  status=$?
  CLOSURE_OBJECTIVE="fail"
  write_closure_report "failed" "PX1 objective coverage audit failed before hardware flashing."
  exit "$status"
fi

if [[ "$SKIP_FLASH" == "1" ]]; then
  step "PX1 flash skipped"
  CLOSURE_FLASH="skipped"
  write_closure_report "blocked" "Flash was explicitly skipped; real completion still requires WCH ISP flash and hardware validation."
  printf 'Firmware ready for flashing: %s\n' "$BIN"
  shasum -a 256 "$BIN"
  exit 2
fi

step "PX1 WCH ISP flash"
if [[ "$WAIT_FOREVER" == "1" ]]; then
  if "$ROOT/tools/flash_powerx.sh" --wait-forever "$BIN"; then
    CLOSURE_FLASH="pass"
  else
    status=$?
    CLOSURE_FLASH="blocked"
    refresh_isp_preflight
    write_closure_report "blocked" "WCH ISP flash failed or no PX1 ISP USB device was visible."
    exit "$status"
  fi
else
  if "$ROOT/tools/flash_powerx.sh" --wait "$WAIT_SECONDS" "$BIN"; then
    CLOSURE_FLASH="pass"
  else
    status=$?
    CLOSURE_FLASH="blocked"
    refresh_isp_preflight
    write_closure_report "blocked" "WCH ISP flash failed or no PX1 ISP USB device was visible."
    exit "$status"
  fi
fi

step "PX1 hardware report"
current_sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
if [[ ! -e "$HARDWARE_REPORT" ]]; then
  "$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
    --output "$HARDWARE_REPORT" \
    "$BIN"
  CLOSURE_HARDWARE_REPORT="prepared"
elif [[ "$FORCE_REPORT" == "1" ]]; then
  "$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
    --force \
    --output "$HARDWARE_REPORT" \
    "$BIN"
  CLOSURE_HARDWARE_REPORT="refreshed"
elif grep -Fq "Firmware SHA256: $current_sha" "$HARDWARE_REPORT"; then
  printf 'Hardware report already exists for current firmware: %s\n' "$HARDWARE_REPORT"
  printf 'Keeping it unchanged. Fill it with real hardware evidence before final audit can pass.\n'
  CLOSURE_HARDWARE_REPORT="preserved"
else
  printf 'Hardware report already exists: %s\n' "$HARDWARE_REPORT"
  printf 'It does not match the current firmware SHA256: %s\n' "$current_sha"
  printf 'Use --force-report to refresh the blank template, or update the report manually if it already contains real evidence.\n'
  CLOSURE_HARDWARE_REPORT="stale-blocked"
  write_closure_report "blocked" "Existing hardware report does not match the current firmware SHA."
  exit 1
fi

step "PX1 final goal audit"
if "$ROOT/tools/check_powerx_goal_completion.sh" "$HARDWARE_REPORT"; then
  CLOSURE_FINAL_AUDIT="pass"
  write_closure_report "passed" "All PX1 real-device closure checks passed."
else
  status=$?
  CLOSURE_FINAL_AUDIT="fail"
  write_closure_report "failed" "PX1 final goal audit failed; inspect the hardware validation report and flash evidence."
  exit "$status"
fi

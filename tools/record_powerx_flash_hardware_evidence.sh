#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
REPORT="$ROOT/artifacts/hardware-validation/latest.md"
FLASH_REPORT="${PX1_FLASH_REPORT:-$ROOT/artifacts/flash/latest-success.md}"

usage() {
  cat <<'MSG'
Usage: tools/record_powerx_flash_hardware_evidence.sh [--report report.md] [--flash-report latest-success.md] [firmware.bin]

Records only the hardware-validation evidence proven by a successful WCH ISP
flash report:
  - [x] flash-wchisp
  - Flash evidence: <flash report>
  - 烧录时间: <flash report time>

It deliberately does not check boot, LCD, UI, PD/QC, cable, settings, button, or
stability items because those require separate real hardware observations.
MSG
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --report)
      [[ -n "${2:-}" ]] || {
        printf 'missing value for --report\n' >&2
        exit 2
      }
      REPORT="$2"
      shift 2
      ;;
    --flash-report)
      [[ -n "${2:-}" ]] || {
        printf 'missing value for --flash-report\n' >&2
        exit 2
      }
      FLASH_REPORT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      BIN="$1"
      shift
      ;;
  esac
done

fail() {
  printf 'FAIL record-flash-hardware-evidence: %s\n' "$1" >&2
  exit 1
}

repo_relative_or_abs() {
  local path="$1"

  if [[ "$path" == "$ROOT/"* ]]; then
    printf '%s\n' "${path#"$ROOT/"}"
  else
    printf '%s\n' "$path"
  fi
}

read_flash_time() {
  awk '
    /^[[:space:]]*-[[:space:]]*Time:/ {
      sub(/^[[:space:]]*-[[:space:]]*Time:[[:space:]]*/, "", $0)
      print
      exit
    }
  ' "$FLASH_REPORT"
}

[[ -s "$BIN" ]] || fail "missing or empty firmware: $BIN"
[[ -s "$REPORT" ]] || fail "missing or empty hardware report: $REPORT"
[[ -s "$FLASH_REPORT" ]] || fail "missing or empty flash success report: $FLASH_REPORT"

CURRENT_SHA="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"

grep -Fq "Firmware SHA256: $CURRENT_SHA" "$REPORT" \
  || fail "hardware report does not match current firmware SHA256: $CURRENT_SHA"
grep -Fq "Firmware SHA256: \`$CURRENT_SHA\`" "$FLASH_REPORT" \
  || fail "flash evidence does not match current firmware SHA256: $CURRENT_SHA"
grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`flashed`[[:space:]]*$' "$FLASH_REPORT" \
  || fail "flash evidence is not successful: $FLASH_REPORT"
grep -Fq 'CH32L103K8U6' "$FLASH_REPORT" \
  || fail "flash evidence is missing PX1 CH32L103K8U6 target chip output: $FLASH_REPORT"
grep -Fq 'wchisp info exit: `0`' "$FLASH_REPORT" \
  || fail "flash evidence is missing successful wchisp info exit code: $FLASH_REPORT"
grep -Fq 'wchisp flash exit: `0`' "$FLASH_REPORT" \
  || fail "flash evidence is missing successful wchisp flash exit code: $FLASH_REPORT"
grep -Fq 'Verify OK' "$FLASH_REPORT" \
  || fail "flash evidence is missing Verify OK output: $FLASH_REPORT"
grep -Fq 'Device reset' "$FLASH_REPORT" \
  || fail "flash evidence is missing Device reset output: $FLASH_REPORT"

FLASH_TIME="$(read_flash_time)"
[[ "$FLASH_TIME" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2}:[0-9]{2}[[:space:]]+[+-][0-9]{4}$ ]] \
  || fail "flash evidence is missing a concrete timestamp: $FLASH_REPORT"

grep -Eq '^[[:space:]]*-[[:space:]]*\[[ xX]\][[:space:]]+flash-wchisp([[:space:]]|$)' "$REPORT" \
  || fail "hardware report is missing flash-wchisp checklist item"
grep -Eq '^[[:space:]]*-[[:space:]]*Flash evidence:' "$REPORT" \
  || fail "hardware report is missing Flash evidence field"
grep -Eq '^[[:space:]]*-[[:space:]]*烧录时间:' "$REPORT" \
  || fail "hardware report is missing 烧录时间 field"

FLASH_REPORT_FIELD="$(repo_relative_or_abs "$FLASH_REPORT")"
tmp_report="$(mktemp)"
awk \
  -v flash_report="$FLASH_REPORT_FIELD" \
  -v flash_time="$FLASH_TIME" '
    /^[[:space:]]*-[[:space:]]*\[[ xX]\][[:space:]]+flash-wchisp([[:space:]]|$)/ {
      sub(/\[[ xX]\]/, "[x]")
      print
      next
    }
    /^[[:space:]]*-[[:space:]]*Flash evidence:/ {
      print "- Flash evidence: " flash_report
      next
    }
    /^[[:space:]]*-[[:space:]]*烧录时间:/ {
      print "- 烧录时间: " flash_time
      next
    }
    { print }
  ' "$REPORT" >"$tmp_report"
mv "$tmp_report" "$REPORT"

printf 'Recorded flash hardware evidence: %s\n' "$REPORT"
printf 'Firmware SHA256: %s\n' "$CURRENT_SHA"
printf 'Flash evidence: %s\n' "$FLASH_REPORT_FIELD"
printf '烧录时间: %s\n' "$FLASH_TIME"

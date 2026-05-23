#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
TEMPLATE="$ROOT/docs/px1_hardware_validation_report_template.md"
REPORT="$ROOT/artifacts/hardware-validation/latest.md"
FLASH_REPORT="${PX1_FLASH_REPORT:-$ROOT/artifacts/flash/latest-success.md}"
FORCE=0

usage() {
  cat <<'MSG'
Usage: tools/prepare_powerx_hardware_validation_report.sh [--force] [--output report.md] [firmware.bin]

Creates a PX1 hardware validation report from the template, filling:
  Firmware SHA256
  Flash evidence path
  Evidence file paths and TODO evidence placeholders

The command will not overwrite an existing report unless --force is provided.
MSG
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --force)
      FORCE=1
      shift
      ;;
    --output)
      [[ -n "${2:-}" ]] || {
        printf 'missing value for --output\n' >&2
        exit 2
      }
      REPORT="$2"
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

[[ -s "$BIN" ]] || {
  printf 'missing or empty firmware: %s\n' "$BIN" >&2
  exit 1
}
[[ -s "$TEMPLATE" ]] || {
  printf 'missing or empty template: %s\n' "$TEMPLATE" >&2
  exit 1
}

if [[ -e "$REPORT" && "$FORCE" -eq 0 ]]; then
  printf 'report already exists: %s\n' "$REPORT" >&2
  printf 'use --force to overwrite it\n' >&2
  exit 1
fi

flash_evidence="$FLASH_REPORT"
if [[ "$flash_evidence" == "$ROOT/"* ]]; then
  flash_evidence="${flash_evidence#"$ROOT/"}"
fi

sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
report_dir="$(dirname "$REPORT")"
mkdir -p "$report_dir"
report_dir_abs="$(cd "$report_dir" && pwd)"
evidence_dir_abs="$report_dir_abs/evidence"
mkdir -p "$evidence_dir_abs"

repo_relative_or_abs() {
  local path="$1"

  if [[ "$path" == "$ROOT/"* ]]; then
    printf '%s\n' "${path#"$ROOT/"}"
  else
    printf '%s\n' "$path"
  fi
}

write_placeholder_if_missing() {
  local path="$1"
  local title="$2"
  local body="$3"
  local firmware_sha="$4"

  if [[ -e "$path" ]] && ! grep -Eiq '(TODO|TBD|待填|示例|example|REPLACE|__|YYYY-MM-DD|placeholder)' "$path"; then
    return 0
  fi
  {
    printf '# %s\n\n' "$title"
    printf 'Firmware SHA256: %s\n\n' "$firmware_sha"
    printf 'TODO: replace this placeholder with real PX1 hardware evidence before final validation.\n'
    printf '%s\n' "$body"
  } >"$path"
}

ui_evidence="$evidence_dir_abs/ui-pages.txt"
protocol_evidence="$evidence_dir_abs/pd-qc-trigger.txt"
buttons_evidence="$evidence_dir_abs/buttons-settings.txt"
cable_evidence="$evidence_dir_abs/cc-cable-emarker.txt"

write_placeholder_if_missing \
  "$ui_evidence" \
  "PX1 UI pages evidence" \
  "Required: main scope protocol trigger pdo qc cc cable settings, all 160x80, 9 pages observed." \
  "$sha"
write_placeholder_if_missing \
  "$protocol_evidence" \
  "PX1 PD/QC trigger evidence" \
  "Required: PD Source_Cap/PDO voltage list, requested target voltage, VBUS/current, QC2/QC3 mode, DP/DM voltage, QC VBUS/current." \
  "$sha"
write_placeholder_if_missing \
  "$buttons_evidence" \
  "PX1 buttons and settings evidence" \
  "Required: BTN1/BTN2/BTN3 behavior, brightness, rotation, TRIG AUTO/MAN setting changes." \
  "$sha"
write_placeholder_if_missing \
  "$cable_evidence" \
  "PX1 CC cable E-marker evidence" \
  "Required: CC1/CC2 orientation, E-marker detection result, cable current/type." \
  "$sha"

evidence_field="$(repo_relative_or_abs "$ui_evidence"); $(repo_relative_or_abs "$protocol_evidence"); $(repo_relative_or_abs "$buttons_evidence"); $(repo_relative_or_abs "$cable_evidence")"

tmp_report="$(mktemp)"
sed \
  -e "s|Firmware SHA256: REPLACE_WITH_CURRENT_FIRMWARE_SHA256|Firmware SHA256: $sha|" \
  -e "s|- Flash evidence: .*|- Flash evidence: $flash_evidence|" \
  -e "s|- 证据文件: .*|- 证据文件: $evidence_field|" \
  "$TEMPLATE" >"$tmp_report"
mv "$tmp_report" "$REPORT"

printf 'Prepared PX1 hardware validation report: %s\n' "$REPORT"
printf 'Firmware SHA256: %s\n' "$sha"
printf 'Flash evidence: %s\n' "$flash_evidence"
printf 'Evidence files: %s\n' "$evidence_field"

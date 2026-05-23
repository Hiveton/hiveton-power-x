#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL prepare-hardware-report-test: %s\n' "$1" >&2
  exit 1
}

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

firmware="$tmp_dir/FreeRTOS.bin"
report="$tmp_dir/hardware-report.md"
printf 'powerx-hardware-report-test-firmware' >"$firmware"

sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"

"$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
  --output "$report" \
  "$firmware" \
  >/tmp/powerx_prepare_hw_report.out 2>/tmp/powerx_prepare_hw_report.err \
  || {
    cat /tmp/powerx_prepare_hw_report.err >&2
    fail "failed to prepare report"
  }

[[ -s "$report" ]] || fail "generated report is missing"

grep -Fq "Firmware SHA256: $sha" "$report" \
  || fail "firmware SHA was not replaced"
! grep -Fq "REPLACE_WITH_CURRENT_FIRMWARE_SHA256" "$report" \
  || fail "firmware SHA placeholder was left in report"
grep -Fq -- "- Flash evidence: artifacts/flash/latest-success.md" "$report" \
  || fail "default flash evidence path was not preserved as repo-relative"

evidence_dir="$tmp_dir/evidence"
evidence_files=(
  "$evidence_dir/ui-pages.txt"
  "$evidence_dir/pd-qc-trigger.txt"
  "$evidence_dir/buttons-settings.txt"
  "$evidence_dir/cc-cable-emarker.txt"
)

for evidence_file in "${evidence_files[@]}"; do
  [[ -s "$evidence_file" ]] \
    || fail "evidence placeholder was not generated: $evidence_file"
  grep -Fq "TODO" "$evidence_file" \
    || fail "evidence placeholder must remain clearly unverified: $evidence_file"
  grep -Fq "$sha" "$evidence_file" \
    || fail "evidence placeholder must bind to current firmware SHA: $evidence_file"
done

grep -Fq -- "- 证据文件: ${evidence_files[0]}; ${evidence_files[1]}; ${evidence_files[2]}; ${evidence_files[3]}" "$report" \
  || fail "generated report does not point to the evidence placeholder files"

required_items=(
  "flash-wchisp"
  "boot-backlight"
  "lcd-main-no-garble"
  "ui-all-9-pages"
  "buttons-browse-and-action"
  "pd-detect-source-cap"
  "pd-trigger-request-vbus"
  "qc-detect-dpdm"
  "qc-trigger-request-vbus"
  "cc-orientation"
  "cable-emarker"
  "settings-brightness-rotation-mode"
  "stability-no-freeze-5min"
)

for item in "${required_items[@]}"; do
  grep -Fq -- "- [ ] $item" "$report" \
    || fail "missing checklist item: $item"
done

required_fields=(
  "板子编号:"
  "烧录时间:"
  "电源/PD 充电器型号:"
  "QC 充电器型号:"
  "PD 请求目标与 INA226 实测:"
  "QC 请求目标与 INA226 实测:"
  "线缆/E-marker 结果:"
  "UI 页面确认:"
  "按键确认:"
  "设置确认:"
  "5分钟稳定性:"
  "证据文件:"
  "异常现象:"
)

for field in "${required_fields[@]}"; do
  grep -Fq -- "- $field" "$report" \
    || fail "missing evidence field: $field"
done

if "$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
  --output "$report" \
  "$firmware" \
  >/tmp/powerx_prepare_hw_overwrite.out 2>/tmp/powerx_prepare_hw_overwrite.err; then
  fail "existing report was overwritten without --force"
fi

custom_report="$tmp_dir/custom-flash-success.md"
forced_report="$tmp_dir/forced-hardware-report.md"
PX1_FLASH_REPORT="$custom_report" \
  "$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
    --force \
    --output "$forced_report" \
    "$firmware" \
    >/tmp/powerx_prepare_hw_force.out 2>/tmp/powerx_prepare_hw_force.err \
  || {
    cat /tmp/powerx_prepare_hw_force.err >&2
    fail "failed to prepare forced report with custom flash evidence"
  }

grep -Fq -- "- Flash evidence: $custom_report" "$forced_report" \
  || fail "custom flash evidence path was not written"

forced_evidence_dir="$tmp_dir/evidence"
[[ -s "$forced_evidence_dir/cc-cable-emarker.txt" ]] \
  || fail "forced report did not prepare cable evidence placeholder"

stale_dir="$tmp_dir/stale"
stale_report="$stale_dir/hardware-report.md"
mkdir -p "$stale_dir/evidence"
printf '%s\n' "TODO: stale evidence without current firmware sha" >"$stale_dir/evidence/ui-pages.txt"

"$ROOT/tools/prepare_powerx_hardware_validation_report.sh" \
  --force \
  --output "$stale_report" \
  "$firmware" \
  >/tmp/powerx_prepare_hw_stale.out 2>/tmp/powerx_prepare_hw_stale.err \
  || {
    cat /tmp/powerx_prepare_hw_stale.err >&2
    fail "failed to refresh stale evidence placeholders"
  }

grep -Fq "$sha" "$stale_dir/evidence/ui-pages.txt" \
  || fail "stale TODO evidence placeholder was not refreshed with current firmware SHA"

printf '%s\n' "PASS prepare-hardware-report-test"

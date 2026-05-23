#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
COVERAGE_REPORT="$ROOT/artifacts/goal/objective-coverage.md"

fail() {
  printf 'FAIL objective-coverage-full-success-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$BIN" ]] || fail "missing firmware: $BIN"

tmp_dir="$(mktemp -d)"
saved_coverage_report="$tmp_dir/objective-coverage.saved"
had_coverage_report=0

if [[ -f "$COVERAGE_REPORT" ]]; then
  cp "$COVERAGE_REPORT" "$saved_coverage_report"
  had_coverage_report=1
fi

cleanup() {
  if [[ "$had_coverage_report" == "1" ]]; then
    mkdir -p "$(dirname "$COVERAGE_REPORT")"
    cp "$saved_coverage_report" "$COVERAGE_REPORT"
  else
    rm -f "$COVERAGE_REPORT"
  fi
  rm -rf "$tmp_dir"
}

trap cleanup EXIT

sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
flash_report="$tmp_dir/latest-success.md"
hardware_report="$tmp_dir/hardware.md"
evidence_ui="$tmp_dir/evidence-ui-pages.txt"
evidence_protocol="$tmp_dir/evidence-pd-qc.txt"
evidence_buttons="$tmp_dir/evidence-buttons-settings.txt"
evidence_cable="$tmp_dir/evidence-cc-cable-emarker.txt"

printf '%s\n' "Firmware SHA256: $sha. UI sweep: main scope protocol trigger pdo qc cc cable settings all 160x80, 9 pages observed" >"$evidence_ui"
printf '%s\n' "Firmware SHA256: $sha. PD Source_Cap PDO=5V/9V/15V/20V; request target 20V; VBUS=19.92V I=0.31A. QC3 mode; target 12V; DP=0.60V DM=0.60V VBUS=12.08V I=0.22A." >"$evidence_protocol"
printf '%s\n' "Firmware SHA256: $sha. BTN1 page down, BTN2 confirm action, BTN3 page up; brightness, rotation, TRIG AUTO/MAN settings switched." >"$evidence_buttons"
printf '%s\n' "Firmware SHA256: $sha. CC orientation: CC1 active; E-marker detected, cable current 5A, USB-C 5A cable." >"$evidence_cable"

cat >"$flash_report" <<EOF
# PX1 Flash Attempt

- Time: 2026-05-19 07:00:00 +0800
- Firmware SHA256: \`$sha\`
- wchisp: \`/Users/hiveton/.cargo/bin/wchisp\`
- Wait seconds: \`30\`
- Status: \`flashed\`
- Message: wchisp flash completed successfully.

## wchisp info

- wchisp info exit: \`0\`

\`\`\`text
CH32L103K8U6[0x3225]
BTVER 02.60
\`\`\`

## wchisp flash

- wchisp flash exit: \`0\`

\`\`\`text
Code flash 32560 bytes written
Verify OK
Device reset
\`\`\`
EOF

cat >"$hardware_report" <<EOF
# PX1 硬件验证报告模板

Firmware SHA256: $sha

## 必须逐项真机确认

- [x] flash-wchisp
- [x] boot-backlight
- [x] lcd-main-no-garble
- [x] ui-all-9-pages
- [x] buttons-browse-and-action
- [x] pd-detect-source-cap
- [x] pd-trigger-request-vbus
- [x] qc-detect-dpdm
- [x] qc-trigger-request-vbus
- [x] cc-orientation
- [x] cable-emarker
- [x] settings-brightness-rotation-mode
- [x] stability-no-freeze-5min

## 记录

- Flash evidence: $flash_report
- 板子编号: PX1-A01
- 烧录时间: 2026-05-19 07:01
- 电源/PD 充电器型号: Apple 67W PD Source Cap
- QC 充电器型号: Hiveton QC3.0 adapter
- PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；目标 20V；实测 VBUS=19.92V，I=0.31A，P=6.1W
- QC 请求目标与 INA226 实测: QC3 模式；目标 12V；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A
- 线缆/E-marker 结果: CC1 active，E-marker 电流 5A，线缆 USB-C 5A
- UI 页面确认: main/scope/protocol/trigger/pdo/qc/cc/cable/settings 9页逐页无花屏
- 按键确认: BTN1 翻页，BTN2 确认/action，BTN3 翻页
- 设置确认: brightness 亮度、rotation 旋转、TRIG AUTO/MAN 都已切换
- 5分钟稳定性: 连续 5 分钟无卡死、无花屏、按键正常
- 证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable
- 异常现象: none
EOF

PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_objective_coverage.sh" "$hardware_report" \
  >/tmp/powerx_objective_full_success.out 2>/tmp/powerx_objective_full_success.err \
  || {
    cat /tmp/powerx_objective_full_success.out >&2
    cat /tmp/powerx_objective_full_success.err >&2
    fail "full objective coverage rejected complete PX1 flash and hardware evidence"
  }

grep -Eq '^[|][[:space:]]*WCH ISP flash[[:space:]]*[|][[:space:]]*pass[[:space:]]*[|]' "$COVERAGE_REPORT" \
  || fail "complete flash evidence did not pass the WCH ISP flash row"
grep -Eq '^[|][[:space:]]*hardware validation[[:space:]]*[|][[:space:]]*pass[[:space:]]*[|]' "$COVERAGE_REPORT" \
  || fail "complete hardware evidence did not pass the hardware validation row"
grep -Fq -- '- Failed: 0' "$COVERAGE_REPORT" \
  || fail "complete objective coverage report contains failed requirements"
grep -Fq -- '- Blocked: 0' "$COVERAGE_REPORT" \
  || fail "complete objective coverage report contains blocked requirements"

printf '%s\n' "PASS objective-coverage-full-success-contract-test"

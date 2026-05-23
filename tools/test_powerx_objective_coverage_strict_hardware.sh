#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"

fail() {
  printf 'FAIL objective-coverage-strict-hardware-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$BIN" ]] || fail "missing firmware: $BIN"

tmp_dir="$(mktemp -d)"
coverage_report="$ROOT/artifacts/goal/objective-coverage.md"
saved_coverage_report="$tmp_dir/objective-coverage.saved"
had_coverage_report=0

if [[ -f "$coverage_report" ]]; then
  cp "$coverage_report" "$saved_coverage_report"
  had_coverage_report=1
fi

cleanup() {
  if [[ "$had_coverage_report" == "1" ]]; then
    mkdir -p "$(dirname "$coverage_report")"
    cp "$saved_coverage_report" "$coverage_report"
  else
    rm -f "$coverage_report"
  fi
  rm -rf "$tmp_dir"
}

trap cleanup EXIT

sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
flash_report="$tmp_dir/latest-success.md"
weak_report="$tmp_dir/weak-hardware.md"

cat >"$flash_report" <<EOF
# PX1 Flash Attempt

- Time: 2026-05-19 07:00:00 +0800
- Firmware SHA256: \`$sha\`
- wchisp: \`/Users/hiveton/.cargo/bin/wchisp\`
- Status: \`flashed\`
- Message: wchisp flash completed successfully.

## wchisp info

\`\`\`text
CH32L103K8U6[0x3225]
BTVER 02.60
\`\`\`
EOF

cat >"$weak_report" <<EOF
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
- 板子编号: ok
- 烧录时间: ok
- 电源/PD 充电器型号: ok
- QC 充电器型号: ok
- PD 请求目标与 INA226 实测: ok
- QC 请求目标与 INA226 实测: ok
- 线缆/E-marker 结果: ok
- UI 页面确认: ok
- 按键确认: ok
- 设置确认: ok
- 5分钟稳定性: ok
- 异常现象: none
EOF

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_objective_coverage.sh" "$weak_report" \
  >/tmp/powerx_objective_weak.out 2>/tmp/powerx_objective_weak.err; then
  fail "full objective coverage accepted a weak hardware report"
fi

if ! rg -q 'hardware[- ]validation|burn time|硬件验证' /tmp/powerx_objective_weak.out /tmp/powerx_objective_weak.err; then
  cat /tmp/powerx_objective_weak.out >&2
  cat /tmp/powerx_objective_weak.err >&2
  fail "weak report was rejected for the wrong reason"
fi

printf '%s\n' "PASS objective-coverage-strict-hardware-test"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BIN="$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"

fail() {
  printf 'FAIL hardware-validation-checker-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$BIN" ]] || fail "missing firmware: $BIN"

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

sha="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"
flash_report="$tmp_dir/latest-success.md"
thin_flash_report="$tmp_dir/thin-success.md"
no_exit_flash_report="$tmp_dir/no-exit-success.md"
wrong_chip_flash_report="$tmp_dir/wrong-chip-success.md"
weak_report="$tmp_dir/weak-hardware.md"
concrete_report="$tmp_dir/concrete-hardware.md"
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

cat >"$thin_flash_report" <<EOF
# PX1 Flash Attempt

- Firmware SHA256: \`$sha\`
- Status: \`flashed\`
EOF

cat >"$no_exit_flash_report" <<EOF
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

## wchisp flash

\`\`\`text
Code flash 32560 bytes written
Verify OK
Device reset
\`\`\`
EOF

cat >"$wrong_chip_flash_report" <<EOF
# PX1 Flash Attempt

- Time: 2026-05-19 07:00:00 +0800
- Firmware SHA256: \`$sha\`
- wchisp: \`/Users/hiveton/.cargo/bin/wchisp\`
- Status: \`flashed\`
- Message: wchisp flash completed successfully.

## wchisp info

\`\`\`text
CH32V203C8T6[0x2038]
BTVER 02.60
\`\`\`

## wchisp flash

\`\`\`text
Code flash 32560 bytes written
Verify OK
Device reset
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
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$weak_report" "$BIN" >/tmp/powerx_hw_weak.out 2>/tmp/powerx_hw_weak.err; then
  fail "weak generic report was accepted"
fi

cat >"$concrete_report" <<EOF
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

- Flash evidence: $thin_flash_report
- 板子编号: PX1-A01
- 烧录时间: 2026-05-19 07:00
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

if PX1_FLASH_REPORT="$thin_flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$concrete_report" "$BIN" >/tmp/powerx_hw_thin.out 2>/tmp/powerx_hw_thin.err; then
  fail "thin flash success report was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $no_exit_flash_report|" \
  "$concrete_report" >"$tmp_dir/concrete-no-exit.md"

if PX1_FLASH_REPORT="$no_exit_flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/concrete-no-exit.md" "$BIN" >/tmp/powerx_hw_no_exit.out 2>/tmp/powerx_hw_no_exit.err; then
  fail "flash report missing wchisp exit codes was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $wrong_chip_flash_report|" \
  "$concrete_report" >"$tmp_dir/concrete-wrong-chip.md"

if PX1_FLASH_REPORT="$wrong_chip_flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/concrete-wrong-chip.md" "$BIN" >/tmp/powerx_hw_wrong_chip.out 2>/tmp/powerx_hw_wrong_chip.err; then
  fail "wrong WCH target chip flash report was accepted"
fi

cat >"$tmp_dir/template-boilerplate-hardware.md" <<EOF
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
- 板子编号: PX1-
- 烧录时间: 2026-05-19 07:01
- 电源/PD 充电器型号: 品牌/型号，需支持 PD Source Cap
- QC 充电器型号: 品牌/型号，需支持 QC2/QC3
- PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；目标 20V；实测 VBUS=19.92V，I=0.31A，P=6.1W
- QC 请求目标与 INA226 实测: QC3 模式；目标 12V；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A
- 线缆/E-marker 结果: CC1/CC2 方向；E-marker 电流 5A，线缆类型/速度
- UI 页面确认: main/scope/protocol/trigger/pdo/qc/cc/cable/settings 9页逐页无花屏
- 按键确认: BTN1 上/下切页，BTN2 确认/action，BTN3 上/下切页
- 设置确认: brightness 亮度、rotation 旋转、TRIG AUTO/MAN 都已切换
- 5分钟稳定性: 连续运行 5 分钟，无卡死/无花屏/按键仍响应
- 异常现象: none
EOF

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/template-boilerplate-hardware.md" "$BIN" >/tmp/powerx_hw_template_boilerplate.out 2>/tmp/powerx_hw_template_boilerplate.err; then
  fail "template boilerplate hardware report was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|板子编号: PX1-A01|板子编号: ok|" \
  -e "s|电源/PD 充电器型号: Apple 67W PD Source Cap|电源/PD 充电器型号: ok|" \
  -e "s|QC 充电器型号: Hiveton QC3.0 adapter|QC 充电器型号: ok|" \
  "$concrete_report" >"$tmp_dir/generic-field-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/generic-field-hardware.md" "$BIN" >/tmp/powerx_hw_generic_fields.out 2>/tmp/powerx_hw_generic_fields.err; then
  fail "generic ok board/charger fields were accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|UI 页面确认: main/scope/protocol/trigger/pdo/qc/cc/cable/settings 9页逐页无花屏|UI 页面确认: main 9页逐页无花屏|" \
  "$concrete_report" >"$tmp_dir/partial-ui-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/partial-ui-hardware.md" "$BIN" >/tmp/powerx_hw_partial_ui.out 2>/tmp/powerx_hw_partial_ui.err; then
  fail "partial UI page sweep evidence was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；目标 20V；实测 VBUS=19.92V，I=0.31A，P=6.1W|PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；实测 VBUS=19.92V，I=0.31A，P=6.1W|" \
  -e "s|QC 请求目标与 INA226 实测: QC3 模式；目标 12V；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A|QC 请求目标与 INA226 实测: QC3 模式；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A|" \
  "$concrete_report" >"$tmp_dir/no-trigger-target-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/no-trigger-target-hardware.md" "$BIN" >/tmp/powerx_hw_no_target.out 2>/tmp/powerx_hw_no_target.err; then
  fail "PD/QC measurement report without requested trigger targets was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|PD 请求目标与 INA226 实测: Source_Cap PDO=5V/9V/15V/20V；目标 20V；实测 VBUS=19.92V，I=0.31A，P=6.1W|PD 请求目标与 INA226 实测: 目标 20V；实测 VBUS=19.92V，I=0.31A，P=6.1W|" \
  "$concrete_report" >"$tmp_dir/no-pd-source-cap-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/no-pd-source-cap-hardware.md" "$BIN" >/tmp/powerx_hw_no_pd_source_cap.out 2>/tmp/powerx_hw_no_pd_source_cap.err; then
  fail "PD hardware report without Source_Cap/PDO evidence was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|QC 请求目标与 INA226 实测: QC3 模式；目标 12V；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A|QC 请求目标与 INA226 实测: 目标 12V；DP=0.60V，DM=0.60V，VBUS=12.08V，I=0.22A|" \
  "$concrete_report" >"$tmp_dir/no-qc-mode-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/no-qc-mode-hardware.md" "$BIN" >/tmp/powerx_hw_no_qc_mode.out 2>/tmp/powerx_hw_no_qc_mode.err; then
  fail "QC hardware report without QC mode evidence was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e '/证据文件:/d' \
  "$concrete_report" >"$tmp_dir/no-evidence-files-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/no-evidence-files-hardware.md" "$BIN" >/tmp/powerx_hw_no_evidence_files.out 2>/tmp/powerx_hw_no_evidence_files.err; then
  fail "hardware report without evidence files was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $evidence_ui; $tmp_dir/missing-evidence.txt; $evidence_buttons; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/missing-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/missing-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_missing_evidence_file.out 2>/tmp/powerx_hw_missing_evidence_file.err; then
  fail "hardware report with missing evidence file was accepted"
fi

bad_ui_evidence="$tmp_dir/bad-ui-evidence.txt"
printf '%s\n' "main page observed" >"$bad_ui_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $bad_ui_evidence; $evidence_protocol; $evidence_buttons; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/weak-ui-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/weak-ui-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_weak_ui_evidence.out 2>/tmp/powerx_hw_weak_ui_evidence.err; then
  fail "hardware report with weak UI evidence file was accepted"
fi

bad_protocol_evidence="$tmp_dir/bad-protocol-evidence.txt"
printf '%s\n' "PD and QC observed" >"$bad_protocol_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $evidence_ui; $bad_protocol_evidence; $evidence_buttons; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/weak-protocol-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/weak-protocol-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_weak_protocol_evidence.out 2>/tmp/powerx_hw_weak_protocol_evidence.err; then
  fail "hardware report with weak PD/QC evidence file was accepted"
fi

bad_buttons_evidence="$tmp_dir/bad-buttons-evidence.txt"
printf '%s\n' "buttons and settings observed" >"$bad_buttons_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $evidence_ui; $evidence_protocol; $bad_buttons_evidence; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/weak-buttons-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/weak-buttons-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_weak_buttons_evidence.out 2>/tmp/powerx_hw_weak_buttons_evidence.err; then
  fail "hardware report with weak button/settings evidence file was accepted"
fi

bad_cable_evidence="$tmp_dir/bad-cable-evidence.txt"
printf '%s\n' "cable observed" >"$bad_cable_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $bad_cable_evidence|" \
  "$concrete_report" >"$tmp_dir/weak-cable-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/weak-cable-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_weak_cable_evidence.out 2>/tmp/powerx_hw_weak_cable_evidence.err; then
  fail "hardware report with weak cable/CC evidence file was accepted"
fi

todo_evidence="$tmp_dir/todo-evidence.txt"
printf '%s\n' "TODO: UI sweep main scope protocol trigger pdo qc cc cable settings all 160x80, 9 pages observed" >"$todo_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $todo_evidence; $evidence_protocol; $evidence_buttons; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/todo-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/todo-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_todo_evidence.out 2>/tmp/powerx_hw_todo_evidence.err; then
  fail "hardware report with TODO evidence file was accepted"
fi

stale_sha_evidence="$tmp_dir/stale-sha-evidence.txt"
printf '%s\n' "Firmware SHA256: 0000000000000000000000000000000000000000000000000000000000000000. UI sweep: main scope protocol trigger pdo qc cc cable settings all 160x80, 9 pages observed" >"$stale_sha_evidence"
sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|证据文件: $evidence_ui; $evidence_protocol; $evidence_buttons; $evidence_cable|证据文件: $stale_sha_evidence; $evidence_protocol; $evidence_buttons; $evidence_cable|" \
  "$concrete_report" >"$tmp_dir/stale-sha-evidence-file-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/stale-sha-evidence-file-hardware.md" "$BIN" >/tmp/powerx_hw_stale_sha_evidence.out 2>/tmp/powerx_hw_stale_sha_evidence.err; then
  fail "hardware report with stale firmware SHA evidence file was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  -e "s|烧录时间: 2026-05-19 07:00|烧录时间: 2026-05-19 06:00|" \
  "$concrete_report" >"$tmp_dir/concrete-stale-hardware.md"

if PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/concrete-stale-hardware.md" "$BIN" >/tmp/powerx_hw_stale.out 2>/tmp/powerx_hw_stale.err; then
  fail "hardware report older than flash evidence was accepted"
fi

sed -e "s|Flash evidence: $thin_flash_report|Flash evidence: $flash_report|" \
  "$concrete_report" >"$tmp_dir/concrete-valid-hardware.md"

PX1_FLASH_REPORT="$flash_report" \
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$tmp_dir/concrete-valid-hardware.md" "$BIN" \
  >/tmp/powerx_hw_valid.out 2>/tmp/powerx_hw_valid.err \
  || {
    cat /tmp/powerx_hw_valid.err >&2
    fail "concrete validation report was rejected"
  }

printf '%s\n' "PASS hardware-validation-checker-test"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL goal-completion-full-success-contract-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p \
  "$tmp_root/tools" \
  "$tmp_root/PowerXCode/FreeRTOS/obj" \
  "$tmp_root/artifacts/flash" \
  "$tmp_root/artifacts/goal" \
  "$tmp_root/artifacts/hardware-validation/evidence" \
  "$tmp_root/artifacts/hardware-validation" \
  "$tmp_root/artifacts/ui-preview"

cp "$ROOT/tools/check_powerx_goal_completion.sh" "$tmp_root/tools/check_powerx_goal_completion.sh"
chmod +x "$tmp_root/tools/check_powerx_goal_completion.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'PX1 complete goal completion firmware\n' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
printf '%s\n' "Firmware SHA256: $sha. UI sweep: main scope protocol trigger pdo qc cc cable settings all 160x80, 9 pages observed" \
  >"$tmp_root/artifacts/hardware-validation/evidence/ui-pages.txt"
printf '%s\n' "Firmware SHA256: $sha. PD Source_Cap PDO=5V/9V/15V/20V; request target 20V; VBUS=19.92V I=0.31A. QC3 mode; target 12V; DP=0.60V DM=0.60V VBUS=12.08V I=0.22A." \
  >"$tmp_root/artifacts/hardware-validation/evidence/pd-qc-trigger.txt"
printf '%s\n' "Firmware SHA256: $sha. BTN1 page down, BTN2 confirm action, BTN3 page up; brightness, rotation, TRIG AUTO/MAN settings switched." \
  >"$tmp_root/artifacts/hardware-validation/evidence/buttons-settings.txt"
printf '%s\n' "Firmware SHA256: $sha. CC orientation: CC1 active; E-marker detected, cable current 5A, USB-C 5A cable." \
  >"$tmp_root/artifacts/hardware-validation/evidence/cc-cable-emarker.txt"

cat >"$tmp_root/tools/run_powerx_goal_gate.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake no-flash gate pass\n'
EOF

cat >"$tmp_root/tools/check_powerx_objective_coverage.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake objective coverage pass\n'
EOF

cat >"$tmp_root/tools/check_powerx_hardware_validation.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake hardware validation pass\n'
EOF

chmod +x "$tmp_root/tools/run_powerx_goal_gate.sh" \
  "$tmp_root/tools/check_powerx_objective_coverage.sh" \
  "$tmp_root/tools/check_powerx_hardware_validation.sh"

cat >"$tmp_root/artifacts/ui-preview/current-ui-report.md" <<'EOF'
# PX1 UI Preview Report

| page | size | nonblack | frame | cyan | green | amber | white |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| main | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| protocol | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| trigger | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| cc | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| cable | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| settings | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| scope | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| pdo | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
| qc | 160x80 | 1 | 1 | 1 | 1 | 1 | 1 |
EOF

cat >"$tmp_root/artifacts/flash/isp-preflight.md" <<'EOF'
# PX1 WCH ISP Preflight

- Status: `visible`
EOF

cat >"$tmp_root/artifacts/flash/latest.md" <<EOF
# PX1 Flash Attempt

- Status: \`flashed\`
- Firmware SHA256: \`$sha\`
EOF

cat >"$tmp_root/artifacts/flash/latest-success.md" <<EOF
# PX1 Flash Attempt

- Time: 2026-05-19 07:00:00 +0800
- Firmware: \`$firmware\`
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

cat >"$tmp_root/artifacts/hardware-validation/latest.md" <<EOF
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

- Flash evidence: artifacts/flash/latest-success.md
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
- 证据文件: artifacts/hardware-validation/evidence/ui-pages.txt; artifacts/hardware-validation/evidence/pd-qc-trigger.txt; artifacts/hardware-validation/evidence/buttons-settings.txt; artifacts/hardware-validation/evidence/cc-cable-emarker.txt
- 异常现象: none
EOF

"$tmp_root/tools/check_powerx_goal_completion.sh" "$tmp_root/artifacts/hardware-validation/latest.md" \
  >"$tmp_root/goal.out" 2>"$tmp_root/goal.err" \
  || {
    cat "$tmp_root/goal.out" >&2
    cat "$tmp_root/goal.err" >&2
    fail "goal completion checker rejected complete evidence"
  }

grep -Fq 'PASS powerx-goal-completion' "$tmp_root/goal.out" \
  || fail "goal completion command did not print PASS"
grep -Fq -- '- Overall status: `passed`' "$tmp_root/artifacts/goal/latest.md" \
  || fail "goal report does not record passed status"
grep -Fq '| WCH ISP flash evidence | pass |' "$tmp_root/artifacts/goal/latest.md" \
  || fail "goal report does not mark flash evidence pass"
grep -Fq '| hardware validation report | pass |' "$tmp_root/artifacts/goal/latest.md" \
  || fail "goal report does not mark hardware validation pass"
grep -Fq "Firmware SHA256: \`$sha\`" "$tmp_root/artifacts/goal/latest.md" \
  || fail "goal report does not record current firmware SHA"

printf '%s\n' "PASS goal-completion-full-success-contract-test"

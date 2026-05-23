#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
RECORDER="$ROOT/tools/record_powerx_flash_hardware_evidence.sh"

fail() {
  printf 'FAIL record-flash-hardware-evidence-test: %s\n' "$1" >&2
  exit 1
}

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

firmware="$tmp_dir/FreeRTOS.bin"
report="$tmp_dir/hardware-report.md"
flash_report="$tmp_dir/latest-success.md"
printf 'powerx-record-flash-hardware-evidence-firmware' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"

cat >"$report" <<EOF
# PX1 硬件验证报告模板

Firmware SHA256: $sha

## 必须逐项真机确认

- [ ] flash-wchisp
- [ ] boot-backlight
- [ ] lcd-main-no-garble
- [ ] pd-detect-source-cap

## 记录

- Flash evidence: artifacts/flash/latest-success.md
- 板子编号: PX1-
- 烧录时间: YYYY-MM-DD HH:MM
- 电源/PD 充电器型号: 品牌/型号，需支持 PD Source Cap
EOF

cat >"$flash_report" <<EOF
# PX1 Flash Attempt

- Time: 2026-05-19 12:03:04 +0800
- Firmware SHA256: \`$sha\`
- Status: \`flashed\`

## wchisp info

- wchisp info exit: \`0\`

\`\`\`text
CH32L103K8U6[0x3225]
\`\`\`

## wchisp flash

- wchisp flash exit: \`0\`

\`\`\`text
Verify OK
Device reset
\`\`\`
EOF

"$RECORDER" \
  --report "$report" \
  --flash-report "$flash_report" \
  "$firmware" \
  >"$tmp_dir/record.out" 2>"$tmp_dir/record.err" \
  || {
    cat "$tmp_dir/record.err" >&2
    fail "recorder rejected valid flash evidence"
  }

grep -Fq -- '- [x] flash-wchisp' "$report" \
  || fail "flash-wchisp was not checked"
grep -Fq -- '- [ ] boot-backlight' "$report" \
  || fail "recorder must not check boot-backlight"
grep -Fq -- '- [ ] lcd-main-no-garble' "$report" \
  || fail "recorder must not check lcd-main-no-garble"
grep -Fq -- '- [ ] pd-detect-source-cap' "$report" \
  || fail "recorder must not check PD evidence"
grep -Fq -- "- 烧录时间: 2026-05-19 12:03:04 +0800" "$report" \
  || fail "burn time was not copied from flash evidence"
grep -Fq '品牌/型号，需支持 PD Source Cap' "$report" \
  || fail "unproven hardware fields should remain unchanged"
grep -Fq 'Recorded flash hardware evidence' "$tmp_dir/record.out" \
  || fail "recorder did not print success summary"

before_stale="$(shasum -a 256 "$report" | awk '{ print $1 }')"
stale_flash="$tmp_dir/stale-success.md"
cat >"$stale_flash" <<'EOF'
# PX1 Flash Attempt

- Time: 2026-05-19 12:04:05 +0800
- Firmware SHA256: `stale`
- Status: `flashed`

## wchisp info

- wchisp info exit: `0`

```text
CH32L103K8U6[0x3225]
```

## wchisp flash

- wchisp flash exit: `0`

```text
Verify OK
Device reset
```
EOF

if "$RECORDER" \
  --report "$report" \
  --flash-report "$stale_flash" \
  "$firmware" \
  >"$tmp_dir/stale.out" 2>"$tmp_dir/stale.err"; then
  fail "recorder accepted stale flash evidence"
fi
after_stale="$(shasum -a 256 "$report" | awk '{ print $1 }')"
[[ "$before_stale" == "$after_stale" ]] \
  || fail "report changed after rejected stale flash evidence"

printf '%s\n' "PASS record-flash-hardware-evidence-test"

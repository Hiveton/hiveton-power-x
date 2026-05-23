#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL real-device-closure-success-contract-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/PowerXCode/FreeRTOS/obj" "$tmp_root/artifacts/hardware-validation" "$tmp_root/artifacts/flash"
cp "$ROOT/tools/run_powerx_real_device_closure.sh" "$tmp_root/tools/run_powerx_real_device_closure.sh"
chmod +x "$tmp_root/tools/run_powerx_real_device_closure.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'real-device-closure-success-firmware' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
report="$tmp_root/artifacts/hardware-validation/latest.md"
final_audit_marker="$tmp_root/artifacts/goal/final-audit-called"

cat >"$tmp_root/tools/run_powerx_goal_gate.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == "--no-flash" ]] || exit 64
printf 'fake software gate\n'
EOF

cat >"$tmp_root/tools/check_powerx_objective_coverage.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
if [[ "${1:-}" == "--software-only" ]]; then
  printf 'fake software objective coverage\n'
else
  printf 'fake full objective coverage\n'
fi
EOF

cat >"$tmp_root/tools/flash_powerx.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
firmware="${@: -1}"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
mkdir -p "$root/artifacts/flash"
cat >"$root/artifacts/flash/latest-success.md" <<REPORT
# PX1 Flash Attempt

- Time: 2026-05-19 07:00:00 +0800
- Firmware SHA256: \`$sha\`
- wchisp: \`/Users/hiveton/.cargo/bin/wchisp\`
- Status: \`flashed\`

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
REPORT
printf 'fake flash success\n'
EOF

cat >"$tmp_root/tools/prepare_powerx_hardware_validation_report.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'prepare report should not be called when report already matches current firmware\n' >&2
exit 88
EOF

cat >"$tmp_root/tools/check_powerx_goal_completion.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail
mkdir -p "$tmp_root/artifacts/goal"
printf 'final audit called with %s\n' "\${1:-}" >"$final_audit_marker"
printf 'fake final goal audit\n'
EOF

chmod +x "$tmp_root"/tools/*.sh

cat >"$report" <<EOF
Firmware SHA256: $sha
existing matching real evidence should be kept
EOF

"$tmp_root/tools/run_powerx_real_device_closure.sh" \
  --wait 7 \
  --hardware-report "$report" \
  >"$tmp_root/closure.out" 2>"$tmp_root/closure.err" \
  || {
    cat "$tmp_root/closure.out" >&2
    cat "$tmp_root/closure.err" >&2
    fail "real-device closure rejected a complete success path"
  }

grep -Fq 'fake flash success' "$tmp_root/closure.out" \
  || fail "flash step did not run"
grep -Fq 'Hardware report already exists for current firmware' "$tmp_root/closure.out" \
  || fail "matching hardware report was not preserved"
grep -Fq 'fake final goal audit' "$tmp_root/closure.out" \
  || fail "final goal audit did not run"
grep -Fq "$report" "$final_audit_marker" \
  || fail "final audit was not called with the requested hardware report"
grep -Fq 'existing matching real evidence should be kept' "$report" \
  || fail "matching hardware report was overwritten"
[[ -s "$tmp_root/artifacts/flash/latest-success.md" ]] \
  || fail "fake flash did not create latest-success.md"

printf '%s\n' "PASS real-device-closure-success-contract-test"

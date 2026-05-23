#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL real-device-closure-report-contract-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/PowerXCode/FreeRTOS/obj" "$tmp_root/artifacts/hardware-validation"
cp "$ROOT/tools/run_powerx_real_device_closure.sh" "$tmp_root/tools/run_powerx_real_device_closure.sh"
chmod +x "$tmp_root/tools/run_powerx_real_device_closure.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'real-device-closure-contract-firmware' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
report="$tmp_root/artifacts/hardware-validation/latest.md"

cat >"$tmp_root/tools/run_powerx_goal_gate.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake software gate\n'
EOF

cat >"$tmp_root/tools/check_powerx_objective_coverage.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake objective coverage\n'
EOF

cat >"$tmp_root/tools/flash_powerx.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake flash\n'
EOF

cat >"$tmp_root/tools/prepare_powerx_hardware_validation_report.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
output=""
force=0
while [[ $# -gt 0 ]]; do
  case "$1" in
    --force)
      force=1
      shift
      ;;
    --output)
      output="$2"
      shift 2
      ;;
    *)
      firmware="$1"
      shift
      ;;
  esac
done
[[ -n "$output" ]] || exit 2
[[ "$force" == "1" || ! -e "$output" ]] || exit 1
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
{
  printf 'Firmware SHA256: %s\n' "$sha"
  printf 'prepared with force=%s\n' "$force"
} >"$output"
EOF

cat >"$tmp_root/tools/check_powerx_goal_completion.sh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'fake final goal audit\n'
EOF

chmod +x "$tmp_root"/tools/*.sh

cat >"$report" <<'EOF'
Firmware SHA256: stale-sha
existing report should not be overwritten without force
EOF

if "$tmp_root/tools/run_powerx_real_device_closure.sh" \
  --hardware-report "$report" \
  >/tmp/powerx_real_closure_stale.out 2>/tmp/powerx_real_closure_stale.err; then
  fail "stale existing hardware report was accepted without --force-report"
fi

grep -Fq 'stale-sha' "$report" \
  || fail "stale report was overwritten without --force-report"
grep -Eq 'force-report|report.*current firmware|current firmware.*report' /tmp/powerx_real_closure_stale.out /tmp/powerx_real_closure_stale.err \
  || fail "stale report failure did not explain --force-report/current firmware"

"$tmp_root/tools/run_powerx_real_device_closure.sh" \
  --force-report \
  --hardware-report "$report" \
  >/tmp/powerx_real_closure_force.out 2>/tmp/powerx_real_closure_force.err \
  || {
    cat /tmp/powerx_real_closure_force.out >&2
    cat /tmp/powerx_real_closure_force.err >&2
    fail "--force-report did not refresh stale hardware report"
  }

grep -Fq "Firmware SHA256: $sha" "$report" \
  || fail "--force-report did not refresh report SHA"
grep -Fq 'prepared with force=1' "$report" \
  || fail "--force-report did not call prepare script with --force"

printf '%s\n' "PASS real-device-closure-report-contract-test"

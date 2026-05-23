#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL real-device-closure-stale-latest-attempt-report-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/PowerXCode/FreeRTOS/obj" "$tmp_root/artifacts/flash" "$tmp_root/artifacts/hardware-validation"
cp "$ROOT/tools/run_powerx_real_device_closure.sh" "$tmp_root/tools/run_powerx_real_device_closure.sh"
chmod +x "$tmp_root/tools/run_powerx_real_device_closure.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'real-device-closure-current-firmware-with-stale-attempt' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
old_firmware="$tmp_root/old.bin"
printf 'old latest-attempt firmware' >"$old_firmware"
old_sha="$(shasum -a 256 "$old_firmware" | awk '{ print $1 }')"
report="$tmp_root/artifacts/hardware-validation/latest.md"

cat >"$tmp_root/artifacts/flash/latest.md" <<REPORT
# PX1 Flash Attempt

- Firmware SHA256: \`$old_sha\`
- Status: \`no-isp-device\`
- Message: Old blocked flash attempt.
REPORT

cat >"$tmp_root/tools/run_powerx_goal_gate.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == "--no-flash" ]] || exit 64
printf 'fake software gate\n'
SCRIPT

cat >"$tmp_root/tools/check_powerx_objective_coverage.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == "--software-only" ]] || exit 64
printf 'fake software objective coverage; blocked=2\n'
SCRIPT

cat >"$tmp_root/tools/flash_powerx.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'flash should not run when --skip-flash is set\n' >&2
exit 88
SCRIPT

cat >"$tmp_root/tools/check_wch_isp_device.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'preflight should not run when --skip-flash is set\n' >&2
exit 89
SCRIPT

cat >"$tmp_root/tools/prepare_powerx_hardware_validation_report.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'prepare should not run when --skip-flash is set\n' >&2
exit 90
SCRIPT

cat >"$tmp_root/tools/check_powerx_goal_completion.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'final audit should not run when --skip-flash is set\n' >&2
exit 91
SCRIPT

chmod +x "$tmp_root"/tools/*.sh

set +e
"$tmp_root/tools/run_powerx_real_device_closure.sh" \
  --skip-flash \
  --hardware-report "$report" \
  >"$tmp_root/closure.out" 2>"$tmp_root/closure.err"
status=$?
set -e

[[ "$status" == "2" ]] \
  || fail "--skip-flash should return blocked exit code 2, got $status"

closure_report="$tmp_root/artifacts/goal/real-device-closure.md"
[[ -s "$closure_report" ]] \
  || fail "closure did not write blocked status report"
grep -Fq "Firmware SHA256: \`$sha\`" "$closure_report" \
  || fail "closure report does not include current firmware SHA"
grep -Fq "Latest flash attempt SHA256: \`$old_sha\`" "$closure_report" \
  || fail "closure report does not preserve the stale latest attempt SHA"
grep -Fq -- '- Latest flash attempt status: `stale`' "$closure_report" \
  || fail "closure report did not mark mismatched latest flash attempt as stale"

printf '%s\n' "PASS real-device-closure-stale-latest-attempt-report-test"

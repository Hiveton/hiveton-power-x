#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL real-device-closure-skip-flash-current-report-status-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/PowerXCode/FreeRTOS/obj" "$tmp_root/artifacts/hardware-validation"
cp "$ROOT/tools/run_powerx_real_device_closure.sh" "$tmp_root/tools/run_powerx_real_device_closure.sh"
chmod +x "$tmp_root/tools/run_powerx_real_device_closure.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'real-device-closure-skip-flash-current-report-firmware' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
report="$tmp_root/artifacts/hardware-validation/latest.md"

cat >"$report" <<REPORT
# PX1 Hardware Validation

Firmware SHA256: $sha

- [ ] flash-wchisp
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

cat >"$tmp_root/tools/prepare_powerx_hardware_validation_report.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'prepare should not run when --skip-flash is set\n' >&2
exit 89
SCRIPT

cat >"$tmp_root/tools/check_powerx_goal_completion.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'final audit should not run when --skip-flash is set\n' >&2
exit 90
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
  || fail "--skip-flash did not write closure report"
grep -Fq "Firmware SHA256: \`$sha\`" "$closure_report" \
  || fail "closure report does not include current firmware SHA"
grep -Fq '| WCH ISP flash | skipped |' "$closure_report" \
  || fail "closure report does not mark flash as skipped"
grep -Fq '| hardware report preparation | current-template |' "$closure_report" \
  || fail "closure report should mark the matching report template as current-template"

printf '%s\n' "PASS real-device-closure-skip-flash-current-report-status-test"

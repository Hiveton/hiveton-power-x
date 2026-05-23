#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL real-device-closure-isp-preflight-status-report-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/PowerXCode/FreeRTOS/obj" "$tmp_root/artifacts/flash" "$tmp_root/artifacts/hardware-validation"
cp "$ROOT/tools/run_powerx_real_device_closure.sh" "$tmp_root/tools/run_powerx_real_device_closure.sh"
chmod +x "$tmp_root/tools/run_powerx_real_device_closure.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'real-device-closure-isp-preflight-firmware' >"$firmware"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
report="$tmp_root/artifacts/hardware-validation/latest.md"

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
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
firmware="${@: -1}"
sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
cat >"$root/artifacts/flash/latest.md" <<REPORT
# PX1 Flash Attempt

- Firmware SHA256: \`$sha\`
- Status: \`no-isp-device\`
- Message: No WCH ISP USB device found.
REPORT
cat >"$root/artifacts/flash/isp-preflight.md" <<'REPORT'
# PX1 WCH ISP Preflight

- Status: `no-isp-device`
- Message: No WCH ISP USB device found.
REPORT
printf 'fake no ISP\n' >&2
exit 2
SCRIPT

cat >"$tmp_root/tools/prepare_powerx_hardware_validation_report.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'prepare should not run after flash failure\n' >&2
exit 88
SCRIPT

cat >"$tmp_root/tools/check_powerx_goal_completion.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
printf 'final audit should not run after flash failure\n' >&2
exit 89
SCRIPT

chmod +x "$tmp_root"/tools/*.sh

set +e
"$tmp_root/tools/run_powerx_real_device_closure.sh" \
  --wait 1 \
  --hardware-report "$report" \
  >"$tmp_root/closure.out" 2>"$tmp_root/closure.err"
status=$?
set -e

[[ "$status" == "2" ]] \
  || fail "closure should preserve flash failure exit code 2, got $status"

closure_report="$tmp_root/artifacts/goal/real-device-closure.md"
[[ -s "$closure_report" ]] \
  || fail "closure did not write blocked status report"
grep -Fq "Firmware SHA256: \`$sha\`" "$closure_report" \
  || fail "closure report does not include current firmware SHA"
grep -Fq -- '- Latest flash attempt status: `no-isp-device`' "$closure_report" \
  || fail "closure report does not include latest flash attempt status"
grep -Fq -- '- ISP preflight report: `artifacts/flash/isp-preflight.md`' "$closure_report" \
  || fail "closure report does not point to the ISP preflight report"
grep -Fq -- '- ISP preflight status: `no-isp-device`' "$closure_report" \
  || fail "closure report does not include ISP preflight status"

printf '%s\n' "PASS real-device-closure-isp-preflight-status-report-test"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL goal-completion-isp-diagnosis-report-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p \
  "$tmp_root/tools" \
  "$tmp_root/PowerXCode/FreeRTOS/obj" \
  "$tmp_root/artifacts/flash" \
  "$tmp_root/artifacts/hardware-validation" \
  "$tmp_root/artifacts/ui-preview"

cp "$ROOT/tools/check_powerx_goal_completion.sh" "$tmp_root/tools/check_powerx_goal_completion.sh"
chmod +x "$tmp_root/tools/check_powerx_goal_completion.sh"

firmware="$tmp_root/PowerXCode/FreeRTOS/obj/FreeRTOS.bin"
printf 'PX1 goal completion ISP diagnosis firmware\n' >"$firmware"
diagnosis='No external USB device with VID/PID is visible.'

cat >"$tmp_root/tools/run_powerx_goal_gate.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == "--no-flash" ]] || exit 64
printf 'fake no-flash gate pass\n'
SCRIPT

cat >"$tmp_root/tools/check_powerx_objective_coverage.sh" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail
[[ "${1:-}" == "--software-only" ]] || exit 64
printf 'fake software objective coverage; blocked=2\n'
SCRIPT

chmod +x "$tmp_root/tools/run_powerx_goal_gate.sh" \
  "$tmp_root/tools/check_powerx_objective_coverage.sh"

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

cat >"$tmp_root/artifacts/flash/isp-preflight.md" <<EOF
# PX1 WCH ISP Preflight

- Status: \`no-isp-device\`

## USB Host Diagnosis

$diagnosis
EOF

set +e
"$tmp_root/tools/check_powerx_goal_completion.sh" "$tmp_root/artifacts/hardware-validation/latest.md" \
  >"$tmp_root/goal.out" 2>"$tmp_root/goal.err"
status=$?
set -e

[[ "$status" == "1" ]] \
  || fail "goal completion should fail on missing flash success evidence, got $status"

goal_report="$tmp_root/artifacts/goal/latest.md"
[[ -s "$goal_report" ]] \
  || fail "goal completion did not write audit report"

grep -Fq -- "- ISP preflight status: \`no-isp-device\`" "$goal_report" \
  || fail "goal report does not include ISP preflight status"
grep -Fq -- "- ISP preflight diagnosis: $diagnosis" "$goal_report" \
  || fail "goal report does not include ISP preflight diagnosis"
grep -Fq -- "missing or empty file:" "$goal_report" \
  || fail "goal report does not preserve missing flash evidence failure"

printf '%s\n' "PASS goal-completion-isp-diagnosis-report-test"

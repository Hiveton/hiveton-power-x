#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL flash-script-no-isp-fake-wchisp-test: %s\n' "$1" >&2
  exit 1
}

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

fake_wchisp="$tmp_dir/wchisp"
firmware="$tmp_dir/FreeRTOS.bin"
artifact_dir="$tmp_dir/flash-artifacts"
real_success_report="$ROOT/artifacts/flash/latest-success.md"
real_success_before=""

printf 'PX1 fake firmware\n' >"$firmware"

if [[ -e "$real_success_report" ]]; then
  real_success_before="$(shasum -a 256 "$real_success_report" | awk '{ print $1 }')"
fi

cat >"$fake_wchisp" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

case "${1:-}" in
  info)
    printf '%s\n' 'Error: Found 0 USB devices'
    exit 5
    ;;
  flash)
    printf '%s\n' 'flash should not be called when info fails' >&2
    exit 66
    ;;
  *)
    printf 'unexpected wchisp command: %s\n' "${1:-}" >&2
    exit 64
    ;;
esac
SCRIPT
chmod +x "$fake_wchisp"

set +e
PX1_FLASH_ARTIFACT_DIR="$artifact_dir" \
WCHISP="$fake_wchisp" \
  "$ROOT/tools/flash_powerx.sh" "$firmware" >"$tmp_dir/flash.out" 2>"$tmp_dir/flash.err"
status=$?
set -e

[[ "$status" == "2" ]] || fail "flash_powerx.sh should return 2 when no ISP is visible, got $status"
[[ -s "$artifact_dir/latest.md" ]] \
  || fail "flash script did not write no-ISP latest.md"
[[ ! -e "$artifact_dir/latest-success.md" ]] \
  || fail "flash script created latest-success.md after no-ISP probe"

if [[ -n "$real_success_before" ]]; then
  real_success_after="$(shasum -a 256 "$real_success_report" | awk '{ print $1 }')"
  [[ "$real_success_after" == "$real_success_before" ]] \
    || fail "fake no-ISP test modified real latest-success.md"
elif [[ -e "$real_success_report" ]]; then
  fail "fake no-ISP test created real latest-success.md"
fi

grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`no-isp-device`[[:space:]]*$' \
  "$artifact_dir/latest.md" \
  || fail "no-ISP report status is not no-isp-device"
grep -Fq "wchisp: \`$fake_wchisp\`" "$artifact_dir/latest.md" \
  || fail "no-ISP report does not contain fake wchisp path"
grep -Fq '## wchisp info' "$artifact_dir/latest.md" \
  || fail "no-ISP report does not contain wchisp info section"
grep -Fq 'Error: Found 0 USB devices' "$artifact_dir/latest.md" \
  || fail "no-ISP report does not contain raw wchisp info failure output"
grep -Fq -- '- wchisp info exit: `5`' "$artifact_dir/latest.md" \
  || fail "no-ISP report does not contain raw wchisp info exit code"
grep -Fq 'No wchisp flash output captured.' "$artifact_dir/latest.md" \
  || fail "no-ISP report should show flash was not attempted"

printf '%s\n' "PASS flash-script-no-isp-fake-wchisp-test"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL flash-script-failure-fake-wchisp-test: %s\n' "$1" >&2
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
    printf '%s\n' 'CH32L103K8U6[0x3225]'
    printf '%s\n' 'BTVER 02.60'
    ;;
  flash)
    printf '%s\n' 'erase failed: simulated flash failure' >&2
    exit 7
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

[[ "$status" == "1" ]] || fail "flash_powerx.sh should fail with exit 1 after wchisp flash failure, got $status"
[[ -s "$artifact_dir/latest.md" ]] \
  || fail "flash script did not write failed latest.md"
[[ ! -e "$artifact_dir/latest-success.md" ]] \
  || fail "flash script created latest-success.md after failed flash"

if [[ -n "$real_success_before" ]]; then
  real_success_after="$(shasum -a 256 "$real_success_report" | awk '{ print $1 }')"
  [[ "$real_success_after" == "$real_success_before" ]] \
    || fail "fake failure test modified real latest-success.md"
elif [[ -e "$real_success_report" ]]; then
  fail "fake failure test created real latest-success.md"
fi

grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`failed`[[:space:]]*$' \
  "$artifact_dir/latest.md" \
  || fail "failed report status is not failed"
grep -Fq "wchisp: \`$fake_wchisp\`" "$artifact_dir/latest.md" \
  || fail "failed report does not contain fake wchisp path"
grep -Fq '## wchisp info' "$artifact_dir/latest.md" \
  || fail "failed report does not contain wchisp info section"
grep -Fq -- '- wchisp info exit: `0`' "$artifact_dir/latest.md" \
  || fail "failed report does not contain wchisp info exit code"
grep -Fq 'CH32L103K8U6[0x3225]' "$artifact_dir/latest.md" \
  || fail "failed report does not contain wchisp info output"
grep -Fq '## wchisp flash' "$artifact_dir/latest.md" \
  || fail "failed report does not contain wchisp flash section"
grep -Fq 'erase failed: simulated flash failure' "$artifact_dir/latest.md" \
  || fail "failed report does not contain raw wchisp flash failure output"
grep -Fq -- '- wchisp flash exit: `7`' "$artifact_dir/latest.md" \
  || fail "failed report does not contain raw wchisp flash exit code"

printf '%s\n' "PASS flash-script-failure-fake-wchisp-test"

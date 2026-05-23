#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL flash-script-success-requires-verify-test: %s\n' "$1" >&2
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
    [[ -s "${2:-}" ]] || {
      printf '%s\n' 'missing firmware' >&2
      exit 64
    }
    printf '%s\n' 'Code flash 32560 bytes written'
    printf '%s\n' 'flash command exited zero but did not verify or reset'
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

[[ "$status" == "1" ]] \
  || fail "flash_powerx.sh should reject exit-0 flash output without Verify OK and Device reset, got $status"
[[ -s "$artifact_dir/latest.md" ]] \
  || fail "flash script did not write failed latest.md"
[[ ! -e "$artifact_dir/latest-success.md" ]] \
  || fail "flash script wrote latest-success.md without Verify OK and Device reset"

if [[ -n "$real_success_before" ]]; then
  real_success_after="$(shasum -a 256 "$real_success_report" | awk '{ print $1 }')"
  [[ "$real_success_after" == "$real_success_before" ]] \
    || fail "fake verify-missing test modified real latest-success.md"
elif [[ -e "$real_success_report" ]]; then
  fail "fake verify-missing test created real latest-success.md"
fi

grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`failed`[[:space:]]*$' \
  "$artifact_dir/latest.md" \
  || fail "verify-missing report status is not failed"
grep -Fq -- '- wchisp flash exit: `0`' "$artifact_dir/latest.md" \
  || fail "verify-missing report does not preserve raw flash exit code"
grep -Fq 'missing Verify OK or Device reset' "$artifact_dir/latest.md" \
  || fail "verify-missing report does not explain missing verification markers"
grep -Fq 'Code flash 32560 bytes written' "$artifact_dir/latest.md" \
  || fail "verify-missing report does not preserve raw flash output"

printf '%s\n' "PASS flash-script-success-requires-verify-test"

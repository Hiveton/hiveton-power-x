#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL flash-script-requires-px1-chip-test: %s\n' "$1" >&2
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
    printf '%s\n' 'CH32V203C8T6[0x2038]'
    printf '%s\n' 'BTVER 02.60'
    ;;
  flash)
    [[ -s "${2:-}" ]] || {
      printf '%s\n' 'missing firmware' >&2
      exit 64
    }
    printf '%s\n' 'Code flash 32560 bytes written'
    printf '%s\n' 'Verify OK'
    printf '%s\n' 'Device reset'
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
  || fail "flash_powerx.sh should reject a non-PX1 WCH chip even when flash output verifies, got $status"
[[ -s "$artifact_dir/latest.md" ]] \
  || fail "flash script did not write failed latest.md"
[[ ! -e "$artifact_dir/latest-success.md" ]] \
  || fail "flash script wrote latest-success.md for a non-PX1 WCH chip"

if [[ -n "$real_success_before" ]]; then
  real_success_after="$(shasum -a 256 "$real_success_report" | awk '{ print $1 }')"
  [[ "$real_success_after" == "$real_success_before" ]] \
    || fail "fake wrong-chip test modified real latest-success.md"
elif [[ -e "$real_success_report" ]]; then
  fail "fake wrong-chip test created real latest-success.md"
fi

grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`failed`[[:space:]]*$' \
  "$artifact_dir/latest.md" \
  || fail "wrong-chip report status is not failed"
grep -Fq 'CH32V203C8T6[0x2038]' "$artifact_dir/latest.md" \
  || fail "wrong-chip report does not preserve raw wchisp info output"
grep -Fq 'expected CH32L103K8U6' "$artifact_dir/latest.md" \
  || fail "wrong-chip report does not explain the expected PX1 target chip"
grep -Fq 'No wchisp flash output captured.' "$artifact_dir/latest.md" \
  || fail "wrong-chip path should fail before flash is attempted"

printf '%s\n' "PASS flash-script-requires-px1-chip-test"

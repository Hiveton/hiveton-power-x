#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL flash-script-stale-success-report-test: %s\n' "$1" >&2
  exit 1
}

tmp_dir="$(mktemp -d)"
trap 'rm -rf "$tmp_dir"' EXIT

fake_wchisp="$tmp_dir/wchisp"
firmware="$tmp_dir/FreeRTOS.bin"
old_firmware="$tmp_dir/old-FreeRTOS.bin"
artifact_dir="$tmp_dir/flash-artifacts"

mkdir -p "$artifact_dir"
printf 'PX1 current fake firmware\n' >"$firmware"
printf 'PX1 old flashed firmware\n' >"$old_firmware"
current_sha="$(shasum -a 256 "$firmware" | awk '{ print $1 }')"
old_sha="$(shasum -a 256 "$old_firmware" | awk '{ print $1 }')"

cat >"$artifact_dir/latest-success.md" <<REPORT
# PX1 Flash Attempt

- Time: 2026-05-18 07:00:00 +0800
- Firmware SHA256: \`$old_sha\`
- Status: \`flashed\`
REPORT
success_before="$(shasum -a 256 "$artifact_dir/latest-success.md" | awk '{ print $1 }')"

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

[[ "$status" == "2" ]] \
  || fail "flash_powerx.sh should return 2 when no ISP is visible, got $status"
[[ -s "$artifact_dir/latest.md" ]] \
  || fail "flash script did not write no-ISP latest.md"
[[ -s "$artifact_dir/latest-success.md" ]] \
  || fail "flash script removed previous latest-success.md"
success_after="$(shasum -a 256 "$artifact_dir/latest-success.md" | awk '{ print $1 }')"
[[ "$success_after" == "$success_before" ]] \
  || fail "no-ISP attempt modified previous latest-success.md"

grep -Fq "Firmware SHA256: \`$current_sha\`" "$artifact_dir/latest.md" \
  || fail "no-ISP report does not include current firmware SHA"
grep -Fq "Previous flash success SHA256: \`$old_sha\`" "$artifact_dir/latest.md" \
  || fail "no-ISP report does not include previous success SHA"
grep -Fq -- '- Previous flash success status: `stale`' "$artifact_dir/latest.md" \
  || fail "no-ISP report does not mark stale previous flash success"
grep -Fq "$artifact_dir/latest-success.md" "$artifact_dir/latest.md" \
  || fail "no-ISP report does not point to the previous success report"

printf '%s\n' "PASS flash-script-stale-success-report-test"

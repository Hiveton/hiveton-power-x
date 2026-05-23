#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL isp-preflight-reports-wchisp-info-exit-test: %s\n' "$1" >&2
  exit 1
}

tmp_root="$(mktemp -d)"
trap 'rm -rf "$tmp_root"' EXIT

mkdir -p "$tmp_root/tools" "$tmp_root/bin"
cp "$ROOT/tools/check_wch_isp_device.sh" "$tmp_root/tools/check_wch_isp_device.sh"
chmod +x "$tmp_root/tools/check_wch_isp_device.sh"

fake_wchisp="$tmp_root/bin/wchisp"
cat >"$fake_wchisp" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

case "${1:-}" in
  info)
    printf '%s\n' 'Error: No WCH ISP USB device found(4348:55e0 or 1a86:55e0 device not found at index #0)' >&2
    exit 1
    ;;
  *)
    printf 'unexpected wchisp command: %s\n' "${1:-}" >&2
    exit 64
    ;;
esac
SCRIPT
chmod +x "$fake_wchisp"

cat >"$tmp_root/bin/system_profiler" <<'SCRIPT'
#!/usr/bin/env bash
set -euo pipefail

case "${1:-}" in
  -listDataTypes)
    printf '%s\n' 'SPUSBHostDataType'
    ;;
  SPThunderboltDataType)
    printf '%s\n' 'Thunderbolt/USB4:'
    printf '%s\n' 'Status: No device connected'
    ;;
esac
SCRIPT
chmod +x "$tmp_root/bin/system_profiler"

cat >"$tmp_root/bin/ioreg" <<'SCRIPT'
#!/usr/bin/env bash
exit 0
SCRIPT
chmod +x "$tmp_root/bin/ioreg"

set +e
PATH="$tmp_root/bin:$PATH" \
WCHISP="$fake_wchisp" \
  "$tmp_root/tools/check_wch_isp_device.sh" >"$tmp_root/preflight.out" 2>"$tmp_root/preflight.err"
status=$?
set -e

[[ "$status" == "2" ]] \
  || fail "preflight should return blocked exit code 2 when wchisp info fails, got $status"

report="$tmp_root/artifacts/flash/isp-preflight.md"
[[ -s "$report" ]] \
  || fail "preflight report was not written"

grep -Fq -- '- wchisp info exit: `1`' "$report" \
  || fail "preflight report does not include failing wchisp info exit code"
grep -Fq 'No WCH ISP USB device found' "$report" \
  || fail "preflight report does not preserve raw wchisp error output"
grep -Fq 'PX1 WCH ISP preflight: no-isp-device' "$tmp_root/preflight.out" \
  || fail "no-isp-device status was not printed"

printf '%s\n' "PASS isp-preflight-reports-wchisp-info-exit-test"

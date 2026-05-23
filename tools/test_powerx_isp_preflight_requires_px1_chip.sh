#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL isp-preflight-requires-px1-chip-test: %s\n' "$1" >&2
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
    printf '%s\n' 'CH32V203C8T6[0x2038]'
    printf '%s\n' 'BTVER 02.60'
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
  || fail "preflight should reject a non-PX1 WCH chip with exit 2, got $status"

report="$tmp_root/artifacts/flash/isp-preflight.md"
[[ -s "$report" ]] \
  || fail "preflight report was not written"

grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`wrong-chip`[[:space:]]*$' "$report" \
  || fail "wrong-chip report status is missing"
grep -Fq 'CH32V203C8T6[0x2038]' "$report" \
  || fail "wrong-chip report does not preserve raw wchisp info output"
grep -Fq 'expected CH32L103K8U6' "$report" \
  || fail "wrong-chip report does not explain the expected PX1 target chip"
grep -Fq 'PX1 WCH ISP preflight: wrong-chip' "$tmp_root/preflight.out" \
  || fail "wrong-chip status was not printed"
! grep -Fq 'PX1 WCH ISP preflight: visible' "$tmp_root/preflight.out" \
  || fail "wrong-chip device was incorrectly reported as visible"

printf '%s\n' "PASS isp-preflight-requires-px1-chip-test"

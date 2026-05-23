#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL isp-preflight-wch-serial-not-isp-test: %s\n' "$1" >&2
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
  SPUSBHostDataType)
    cat <<'USB'
USB:
    USB 3.1 Bus:
      Location ID: 0x01000000
      Connection Type: Built-in
        USB Serial:
          Location ID: 0x01100000
          Connection Type: Removable
          Link Speed: 12 Mb/s
          USB Vendor ID: 0x1a86
          USB Product ID: 0x7523
USB
    ;;
  SPUSBDataType)
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
set -euo pipefail

cat <<'USB'
+-o USB Serial@01100000  <class IOUSBHostDevice, registered, matched>
  |   "idProduct" = 29987
  |   "USB Product Name" = "USB Serial"
  |   "USB Vendor Name" = "WCH.CN"
  |   "idVendor" = 6790
  |   "kUSBProductString" = "USB Serial"
  |   "kUSBVendorString" = "WCH.CN"
USB
SCRIPT
chmod +x "$tmp_root/bin/ioreg"

set +e
PATH="$tmp_root/bin:$PATH" \
WCHISP="$fake_wchisp" \
  "$tmp_root/tools/check_wch_isp_device.sh" >"$tmp_root/preflight.out" 2>"$tmp_root/preflight.err"
status=$?
set -e

[[ "$status" == "2" ]] \
  || fail "preflight should return blocked exit code 2 when only WCH USB serial is present, got $status"

report="$tmp_root/artifacts/flash/isp-preflight.md"
[[ -s "$report" ]] \
  || fail "preflight report was not written"

grep -Fq -- '- Status: `no-isp-device`' "$report" \
  || fail "preflight status should remain no-isp-device"
grep -Fq 'A WCH USB serial device is visible, but no WCH ISP product ID 0x55e0 was found.' "$report" \
  || fail "WCH serial should be diagnosed as non-ISP WCH USB, not PX1 ISP"
! grep -Fq 'A WCH/CH32-like USB entry is present in the probe output.' "$report" \
  || fail "WCH serial was misdiagnosed as an ISP-like WCH/CH32 entry"

printf '%s\n' "PASS isp-preflight-wch-serial-not-isp-test"

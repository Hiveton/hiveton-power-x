#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

fail() {
  printf 'FAIL isp-preflight-generic-usb-not-wch-test: %s\n' "$1" >&2
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
      Location ID: 0x02000000
      Connection Type: Built-in
      Driver: AppleT8142USBXHCI
        USB2.1 Hub:
          Location ID: 0x02100000
          Connection Type: Removable
          Manufacturer: Generic
          Link Speed: 480 Mb/s
          USB Vendor ID: 0x2717
          USB Product ID: 0x50ae
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
+-o USB2.1 Hub@02100000  <class IOUSBHostDevice, registered, matched>
  |   "idProduct" = 20654
  |   "USB Product Name" = "USB2.1 Hub"
  |   "USB Vendor Name" = "Generic"
  |   "idVendor" = 10007
  |   "kUSBProductString" = "USB2.1 Hub"
  |   "kUSBVendorString" = "Generic"
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
  || fail "preflight should return blocked exit code 2 when only a generic USB hub is present, got $status"

report="$tmp_root/artifacts/flash/isp-preflight.md"
[[ -s "$report" ]] \
  || fail "preflight report was not written"

grep -Fq -- '- Status: `no-isp-device`' "$report" \
  || fail "preflight status should remain no-isp-device"
grep -Fq 'USB devices may be present, but no WCH/CH32 ISP VID/PID was found.' "$report" \
  || fail "generic USB device should be diagnosed as present-but-not-WCH"
! grep -Fq 'A WCH/CH32-like USB entry is present in the probe output.' "$report" \
  || fail "generic USB VID/PID was misdiagnosed as a WCH/CH32-like entry"

printf '%s\n' "PASS isp-preflight-generic-usb-not-wch-test"

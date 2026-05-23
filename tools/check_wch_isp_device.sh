#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARTIFACT_DIR="$ROOT/artifacts/flash"
REPORT="$ARTIFACT_DIR/isp-preflight.md"
WAIT_SECONDS="${PX1_ISP_WAIT_SECONDS:-0}"
WCHISP_INFO_OUTPUT=""
WCHISP_INFO_STATUS=""
SYSTEM_USB_MATCHES=""
SYSTEM_USB_SNAPSHOT=""
IOREG_USB_MATCHES=""
IOREG_USB_SNAPSHOT=""
IOUSBHOST_MATCHES=""
IOUSBHOST_SNAPSHOT=""
THUNDERBOLT_SNAPSHOT=""
SERIAL_PORTS=""
STATUS="no-isp-device"
MESSAGE="No WCH ISP USB device found; expected 4348:55e0 or 1a86:55e0."

if [[ "${1:-}" == "--wait" ]]; then
  if [[ "${2:-}" =~ ^[0-9]+$ ]]; then
    WAIT_SECONDS="$2"
    shift 2
  else
    WAIT_SECONDS="30"
    shift 1
  fi
fi

find_wchisp() {
  if [[ -x "${WCHISP:-}" ]]; then
    printf '%s\n' "$WCHISP"
  elif [[ -x "$HOME/.cargo/bin/wchisp" ]]; then
    printf '%s\n' "$HOME/.cargo/bin/wchisp"
  elif command -v wchisp >/dev/null 2>&1; then
    command -v wchisp
  else
    printf '%s\n' "not found"
  fi
}

wchisp_info_matches_px1_chip() {
  grep -Fq 'CH32L103K8U6' <<<"$WCHISP_INFO_OUTPUT"
}

system_profiler_usb() {
  local types
  local legacy_snapshot
  types="$(system_profiler -listDataTypes 2>/dev/null || true)"
  if grep -qx 'SPUSBHostDataType' <<<"$types"; then
    printf '### SPUSBHostDataType\n'
    system_profiler SPUSBHostDataType 2>/dev/null || true
  fi
  legacy_snapshot="$(system_profiler SPUSBDataType 2>/dev/null || true)"
  if [[ -n "$legacy_snapshot" ]]; then
    printf '### SPUSBDataType\n'
    printf '%s\n' "$legacy_snapshot"
  fi
}

capture_usb_state() {
  if command -v system_profiler >/dev/null 2>&1; then
    SYSTEM_USB_MATCHES="$(system_profiler_usb \
      | grep -Ei -C 2 'WCH|CH32|wchisp|Vendor ID: 0x4348|Vendor ID: 0x1a86|Product ID: 0x55e0' || true)"
    SYSTEM_USB_SNAPSHOT="$(system_profiler_usb \
      | awk 'NF { print }' \
      | head -n 220 || true)"
    THUNDERBOLT_SNAPSHOT="$(system_profiler SPThunderboltDataType 2>/dev/null \
      | awk 'NF { print }' \
      | head -n 220 || true)"
  else
    SYSTEM_USB_MATCHES="system_profiler unavailable"
    SYSTEM_USB_SNAPSHOT="system_profiler unavailable"
    THUNDERBOLT_SNAPSHOT="system_profiler unavailable"
  fi

  if command -v ioreg >/dev/null 2>&1; then
    IOREG_USB_MATCHES="$(ioreg -p IOUSB -l -w 0 2>/dev/null \
      | grep -Ei -C 2 'WCH|CH32|wchisp|0x4348|0x1a86|0x55e0|17224|6790|21984' || true)"
    IOREG_USB_SNAPSHOT="$(ioreg -p IOUSB -l -w 0 2>/dev/null \
      | awk '
          /\+-o / { node=$0 }
          /"idVendor" = |"idProduct" = |"USB Product Name" = |"USB Vendor Name" = |"kUSBProductString" = |"kUSBVendorString" = / {
            if (node != last_node) {
              print node
              last_node=node
            }
            print
          }' \
      | head -n 220 || true)"
    IOUSBHOST_MATCHES="$(ioreg -r -c IOUSBHostDevice -l -w 0 2>/dev/null \
      | grep -Ei -C 2 'WCH|CH32|wchisp|0x4348|0x1a86|0x55e0|17224|6790|21984' || true)"
    IOUSBHOST_SNAPSHOT="$(ioreg -r -c IOUSBHostDevice -l -w 0 2>/dev/null \
      | awk '
          /\+-o / { node=$0 }
          /"idVendor" = |"idProduct" = |"USB Product Name" = |"USB Vendor Name" = |"kUSBProductString" = |"kUSBVendorString" = / {
            if (node != last_node) {
              print node
              last_node=node
            }
            print
          }' \
      | head -n 220 || true)"
  else
    IOREG_USB_MATCHES="ioreg unavailable"
    IOREG_USB_SNAPSHOT="ioreg unavailable"
    IOUSBHOST_MATCHES="ioreg unavailable"
    IOUSBHOST_SNAPSHOT="ioreg unavailable"
  fi

  SERIAL_PORTS="$(ls /dev/cu.* 2>/dev/null || true)"
  [[ -n "$SERIAL_PORTS" ]] || SERIAL_PORTS="No /dev/cu.* ports visible."
}

write_report() {
  local wchisp_bin="$1"
  local usb_host_diagnosis
  local usb_probe_text

  usb_probe_text="$(printf '%s\n%s\n%s\n' \
    "$SYSTEM_USB_SNAPSHOT" \
    "$IOREG_USB_SNAPSHOT" \
    "$IOUSBHOST_SNAPSHOT")"

  if [[ -n "$SYSTEM_USB_SNAPSHOT" ]] &&
     ! grep -Eq 'Vendor ID:|Product ID:' <<<"$SYSTEM_USB_SNAPSHOT" &&
     [[ -z "$IOREG_USB_SNAPSHOT" ]] &&
     [[ -z "$IOUSBHOST_SNAPSHOT" ]]; then
    if grep -q 'Status: No device connected' <<<"$THUNDERBOLT_SNAPSHOT"; then
      usb_host_diagnosis="No external USB device with VID/PID is visible. macOS currently shows only built-in USB host controllers, and USB4 ports report no connected device."
    else
      usb_host_diagnosis="No external USB device with VID/PID is visible. macOS currently shows only built-in USB host controllers."
    fi
  elif grep -Eiq '0x55e0|21984' <<<"$usb_probe_text"; then
    usb_host_diagnosis="A WCH ISP product ID 0x55e0-like USB entry is present, but wchisp did not open a PX1 ISP target."
  elif grep -Eiq 'WCH|CH32|wchisp|0x4348|0x1a86|17224|6790' <<<"$usb_probe_text"; then
    usb_host_diagnosis="A WCH USB serial device is visible, but no WCH ISP product ID 0x55e0 was found."
  elif [[ -z "$SYSTEM_USB_MATCHES" && -z "$IOREG_USB_MATCHES" && -z "$IOUSBHOST_MATCHES" ]]; then
    usb_host_diagnosis="USB devices may be present, but no WCH/CH32 ISP VID/PID was found."
  else
    usb_host_diagnosis="A WCH/CH32-like USB entry is present in the probe output."
  fi

  mkdir -p "$ARTIFACT_DIR"
  {
    printf '# PX1 WCH ISP Preflight\n\n'
    printf '%s\n' "- Time: $(date '+%Y-%m-%d %H:%M:%S %z')"
    printf '%s\n' "- Status: \`$STATUS\`"
    printf '%s\n' "- Message: $MESSAGE"
    printf '%s\n' "- Wait seconds: \`$WAIT_SECONDS\`"
    printf '%s\n\n' "- wchisp: \`$wchisp_bin\`"
    printf '## wchisp info\n\n'
    if [[ -n "$WCHISP_INFO_STATUS" ]]; then
      printf '%s\n\n' "- wchisp info exit: \`$WCHISP_INFO_STATUS\`"
    fi
    if [[ -n "$WCHISP_INFO_OUTPUT" ]]; then
      printf '```text\n%s\n```\n\n' "$WCHISP_INFO_OUTPUT"
    else
      printf '```text\nNo wchisp info output captured.\n```\n\n'
    fi
    printf '## system_profiler USB Probe\n\n'
    if [[ -n "$SYSTEM_USB_MATCHES" ]]; then
      printf '```text\n%s\n```\n\n' "$SYSTEM_USB_MATCHES"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in system_profiler.\n```\n\n'
    fi
    printf '## system_profiler USB Snapshot\n\n'
    if [[ -n "$SYSTEM_USB_SNAPSHOT" ]]; then
      printf '```text\n%s\n```\n\n' "$SYSTEM_USB_SNAPSHOT"
    else
      printf '```text\nNo USB snapshot captured from system_profiler.\n```\n\n'
    fi
    printf '## ioreg USB Probe\n\n'
    if [[ -n "$IOREG_USB_MATCHES" ]]; then
      printf '```text\n%s\n```\n\n' "$IOREG_USB_MATCHES"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in ioreg.\n```\n\n'
    fi
    printf '## ioreg USB Snapshot\n\n'
    if [[ -n "$IOREG_USB_SNAPSHOT" ]]; then
      printf '```text\n%s\n```\n\n' "$IOREG_USB_SNAPSHOT"
    else
      printf '```text\nNo USB device VID/PID snapshot captured from ioreg.\n```\n\n'
    fi
    printf '## IOUSBHostDevice Probe\n\n'
    if [[ -n "$IOUSBHOST_MATCHES" ]]; then
      printf '```text\n%s\n```\n\n' "$IOUSBHOST_MATCHES"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in IOUSBHostDevice.\n```\n\n'
    fi
    printf '## IOUSBHostDevice Snapshot\n\n'
    if [[ -n "$IOUSBHOST_SNAPSHOT" ]]; then
      printf '```text\n%s\n```\n\n' "$IOUSBHOST_SNAPSHOT"
    else
      printf '```text\nNo USB host device VID/PID snapshot captured from IOUSBHostDevice.\n```\n\n'
    fi
    printf '## Serial Ports\n\n'
    printf '```text\n%s\n```\n\n' "$SERIAL_PORTS"
    printf '## Thunderbolt/USB4 Port Snapshot\n\n'
    if [[ -n "$THUNDERBOLT_SNAPSHOT" ]]; then
      printf '```text\n%s\n```\n\n' "$THUNDERBOLT_SNAPSHOT"
    else
      printf '```text\nNo Thunderbolt/USB4 snapshot captured.\n```\n\n'
    fi
    printf '## USB Host Diagnosis\n\n'
    printf '%s\n\n' "$usb_host_diagnosis"
    printf '## ISP Entry Notes\n\n'
    printf '%s\n' '- Expected WCH ISP USB IDs: `4348:55e0` or `1a86:55e0`.'
    printf '%s\n' '- PX1 schematic labels `BTN2 (ISP)` on `PB9/BOOT0`; `PB2/BOOT1` is also present on the MCU.'
    printf '%s\n' '- Manual entry: hold `BTN2/BOOT` while reconnecting USB, then run this script again before flashing.'
    printf '%s\n' '- Keep `BTN2/BOOT` pressed before USB insertion, hold for 1-2 seconds after insertion, then release.'
    printf '%s\n' '- If no USB VID/PID device is visible, test a known data-capable USB-C cable and direct Mac port before debugging firmware.'
    printf '%s\n' '- If another USB device appears but not WCH ISP, re-check BOOT0 level on `PB9/BOOT0`, reset timing, and whether the MCU is entering user firmware instead of Bootloader.'
  } >"$REPORT"
}

probe_wchisp() {
  local wchisp_bin="$1"
  local info_status

  if [[ "$wchisp_bin" == "not found" ]]; then
    WCHISP_INFO_OUTPUT="wchisp not found"
    WCHISP_INFO_STATUS="127"
    STATUS="failed"
    MESSAGE="wchisp not found; install it or set WCHISP=/path/to/wchisp."
    return 2
  fi

  set +e
  WCHISP_INFO_OUTPUT="$("$wchisp_bin" info 2>&1)"
  info_status=$?
  set -e
  WCHISP_INFO_STATUS="$info_status"

  if [[ "$info_status" == "0" ]]; then
    if wchisp_info_matches_px1_chip; then
      STATUS="visible"
      MESSAGE="PX1 WCH ISP device is visible through wchisp."
      return 0
    fi

    STATUS="wrong-chip"
    MESSAGE="WCH ISP device responded, but target chip is not PX1; expected CH32L103K8U6."
    return 2
  fi

  STATUS="no-isp-device"
  MESSAGE="No WCH ISP USB device found; expected 4348:55e0 or 1a86:55e0."
  return 1
}

WCHISP_BIN="$(find_wchisp)"
probe_wchisp "$WCHISP_BIN" || {
  if [[ "$STATUS" == "no-isp-device" ]] &&
     [[ "$WAIT_SECONDS" =~ ^[0-9]+$ ]] &&
     (( WAIT_SECONDS > 0 )); then
    printf 'Waiting up to %ss for WCH ISP device...\n' "$WAIT_SECONDS" >&2
    printf 'Hold BTN2/BOOT, reconnect USB-C, keep it pressed for 1-2 seconds, then release.\n' >&2
    deadline=$((SECONDS + WAIT_SECONDS))
    attempts=0
    while (( SECONDS < deadline )); do
      sleep 1
      attempts=$((attempts + 1))
      probe_wchisp "$WCHISP_BIN" && break
      if (( attempts % 5 == 0 )); then
        printf '[%s] still waiting for 4348:55e0 or 1a86:55e0... (%ss elapsed)\n' \
          "$(date '+%H:%M:%S')" "$attempts" >&2
      fi
    done
  fi
}

capture_usb_state
write_report "$WCHISP_BIN"

printf 'PX1 WCH ISP preflight: %s\n' "$STATUS"
printf 'Report: %s\n' "$REPORT"
if [[ "$STATUS" == "visible" ]]; then
  printf '%s\n' "$WCHISP_INFO_OUTPUT"
  exit 0
fi

printf '%s\n' "$MESSAGE" >&2
exit 2

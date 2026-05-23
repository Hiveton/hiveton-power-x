#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WAIT_SECONDS="${PX1_FLASH_WAIT_SECONDS:-0}"
WAIT_FOREVER=0
FLASH_ARTIFACT_DIR="${PX1_FLASH_ARTIFACT_DIR:-$ROOT/artifacts/flash}"
FLASH_REPORT="$FLASH_ARTIFACT_DIR/latest.md"
FLASH_SUCCESS_REPORT="$FLASH_ARTIFACT_DIR/latest-success.md"
WCHISP_INFO_OUTPUT=""
WCHISP_INFO_STATUS=""
WCHISP_FLASH_OUTPUT=""
WCHISP_FLASH_STATUS=""

info_output_matches_px1_chip() {
  grep -Fq 'CH32L103K8U6' <<<"$WCHISP_INFO_OUTPUT"
}

flash_output_has_verify_markers() {
  grep -Fq 'Verify OK' <<<"$WCHISP_FLASH_OUTPUT" &&
    grep -Fq 'Device reset' <<<"$WCHISP_FLASH_OUTPUT"
}

read_report_status() {
  local report="$1"

  awk -F'`' '/Status:/ { print $2; exit }' "$report" 2>/dev/null || true
}

read_report_sha() {
  local report="$1"

  awk '
    /Firmware SHA256:/ {
      line=$0
      gsub("`", "", line)
      sub(/^.*Firmware SHA256:[[:space:]]*/, "", line)
      sub(/[[:space:]].*$/, "", line)
      print line
      exit
    }
  ' "$report" 2>/dev/null || true
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

write_flash_report() {
  local status="$1"
  local message="$2"
  local sha
  local system_usb_matches
  local system_usb_snapshot
  local ioreg_usb_matches
  local ioreg_usb_snapshot
  local iousbhost_matches
  local iousbhost_snapshot
  local ports
  local thunderbolt_snapshot
  local usb_host_diagnosis
  local previous_success_sha
  local previous_success_status

  mkdir -p "$FLASH_ARTIFACT_DIR"
  sha="$(shasum -a 256 "$BIN" 2>/dev/null | awk '{ print $1 }')"
  previous_success_sha="$(read_report_sha "$FLASH_SUCCESS_REPORT")"
  [[ -n "$previous_success_sha" ]] || previous_success_sha="missing"
  previous_success_status="$(read_report_status "$FLASH_SUCCESS_REPORT")"
  [[ -n "$previous_success_status" ]] || previous_success_status="missing"
  if [[ "$previous_success_status" == "flashed" &&
        -n "$sha" &&
        "$previous_success_sha" != "$sha" ]]; then
    previous_success_status="stale"
  elif [[ "$previous_success_status" == "flashed" &&
          -n "$sha" &&
          "$previous_success_sha" == "$sha" ]]; then
    previous_success_status="current"
  fi

  if command -v system_profiler >/dev/null 2>&1; then
    system_usb_matches="$(system_profiler_usb \
      | grep -Ei -C 2 'WCH|CH32|wchisp|Vendor ID: 0x4348|Vendor ID: 0x1a86|Product ID: 0x55e0' || true)"
    system_usb_snapshot="$(system_profiler_usb \
      | awk 'NF { print }' \
      | head -n 220 || true)"
    thunderbolt_snapshot="$(system_profiler SPThunderboltDataType 2>/dev/null \
      | awk 'NF { print }' \
      | head -n 220 || true)"
  else
    system_usb_matches="system_profiler unavailable"
    system_usb_snapshot="system_profiler unavailable"
    thunderbolt_snapshot="system_profiler unavailable"
  fi

  if command -v ioreg >/dev/null 2>&1; then
    ioreg_usb_matches="$(ioreg -p IOUSB -l -w 0 2>/dev/null \
      | grep -Ei -C 2 'WCH|CH32|wchisp|0x4348|0x1a86|0x55e0|17224|6790|21984|idVendor|idProduct' || true)"
    ioreg_usb_snapshot="$(ioreg -p IOUSB -l -w 0 2>/dev/null \
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
    iousbhost_matches="$(ioreg -r -c IOUSBHostDevice -l -w 0 2>/dev/null \
      | grep -Ei -C 2 'WCH|CH32|wchisp|0x4348|0x1a86|0x55e0|17224|6790|21984|idVendor|idProduct' || true)"
    iousbhost_snapshot="$(ioreg -r -c IOUSBHostDevice -l -w 0 2>/dev/null \
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
    ioreg_usb_matches="ioreg unavailable"
    ioreg_usb_snapshot="ioreg unavailable"
    iousbhost_matches="ioreg unavailable"
    iousbhost_snapshot="ioreg unavailable"
  fi

  ports="$(ls /dev/cu.* 2>/dev/null || true)"
  [[ -n "$ports" ]] || ports="No /dev/cu.* ports visible."

  if [[ -n "$system_usb_snapshot" ]] &&
     ! grep -Eq 'Vendor ID:|Product ID:' <<<"$system_usb_snapshot" &&
     [[ -z "$ioreg_usb_snapshot" ]] &&
     [[ -z "$iousbhost_snapshot" ]]; then
    if grep -q 'Status: No device connected' <<<"$thunderbolt_snapshot"; then
      usb_host_diagnosis="No external USB device with VID/PID is visible. macOS currently shows only built-in USB host controllers, and USB4 ports report no connected device."
    else
      usb_host_diagnosis="No external USB device with VID/PID is visible. macOS currently shows only built-in USB host controllers."
    fi
  elif [[ -z "$system_usb_matches" && -z "$ioreg_usb_matches" && -z "$iousbhost_matches" ]]; then
    usb_host_diagnosis="USB devices may be present, but no WCH/CH32 ISP VID/PID was found."
  else
    usb_host_diagnosis="A WCH/CH32-like USB entry is present in the probe output."
  fi

  {
    printf '# PX1 Flash Attempt\n\n'
    printf '%s\n' "- Time: $(date '+%Y-%m-%d %H:%M:%S %z')"
    printf '%s\n' "- Firmware: \`$BIN\`"
    printf '%s\n' "- Firmware SHA256: \`$sha\`"
    printf '%s\n' "- wchisp: \`$WCHISP_BIN\`"
    printf '%s\n' "- Wait seconds: \`$WAIT_SECONDS\`"
    printf '%s\n' "- Status: \`$status\`"
    printf '%s\n\n' "- Message: $message"
    printf '## Previous Flash Success\n\n'
    printf '%s\n' "- Previous flash success report: \`$FLASH_SUCCESS_REPORT\`"
    printf '%s\n' "- Previous flash success SHA256: \`$previous_success_sha\`"
    printf '%s\n\n' "- Previous flash success status: \`$previous_success_status\`"
    printf '## wchisp info\n\n'
    if [[ -n "$WCHISP_INFO_STATUS" ]]; then
      printf '%s\n\n' "- wchisp info exit: \`$WCHISP_INFO_STATUS\`"
    fi
    if [[ -n "$WCHISP_INFO_OUTPUT" ]]; then
      printf '```text\n%s\n```\n\n' "$WCHISP_INFO_OUTPUT"
    else
      printf '```text\nNo wchisp info output captured.\n```\n\n'
    fi
    printf '## wchisp flash\n\n'
    if [[ -n "$WCHISP_FLASH_STATUS" ]]; then
      printf '%s\n\n' "- wchisp flash exit: \`$WCHISP_FLASH_STATUS\`"
    fi
    if [[ -n "$WCHISP_FLASH_OUTPUT" ]]; then
      printf '```text\n%s\n```\n\n' "$WCHISP_FLASH_OUTPUT"
    else
      printf '```text\nNo wchisp flash output captured.\n```\n\n'
    fi
    printf '## system_profiler USB Probe\n\n'
    if [[ -n "$system_usb_matches" ]]; then
      printf '```text\n%s\n```\n\n' "$system_usb_matches"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in system_profiler.\n```\n\n'
    fi
    printf '## system_profiler USB Snapshot\n\n'
    if [[ -n "$system_usb_snapshot" ]]; then
      printf '```text\n%s\n```\n\n' "$system_usb_snapshot"
    else
      printf '```text\nNo USB snapshot captured from system_profiler.\n```\n\n'
    fi
    printf '## ioreg USB Probe\n\n'
    if [[ -n "$ioreg_usb_matches" ]]; then
      printf '```text\n%s\n```\n\n' "$ioreg_usb_matches"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in ioreg.\n```\n\n'
    fi
    printf '## ioreg USB Snapshot\n\n'
    if [[ -n "$ioreg_usb_snapshot" ]]; then
      printf '```text\n%s\n```\n\n' "$ioreg_usb_snapshot"
    else
      printf '```text\nNo USB device VID/PID snapshot captured from ioreg.\n```\n\n'
    fi
    printf '## IOUSBHostDevice Probe\n\n'
    if [[ -n "$iousbhost_matches" ]]; then
      printf '```text\n%s\n```\n\n' "$iousbhost_matches"
    else
      printf '```text\nNo WCH/CH32 ISP USB match found in IOUSBHostDevice.\n```\n\n'
    fi
    printf '## IOUSBHostDevice Snapshot\n\n'
    if [[ -n "$iousbhost_snapshot" ]]; then
      printf '```text\n%s\n```\n\n' "$iousbhost_snapshot"
    else
      printf '```text\nNo USB host device VID/PID snapshot captured from IOUSBHostDevice.\n```\n\n'
    fi
    printf '## Serial Ports\n\n'
    printf '```text\n%s\n```\n' "$ports"
    printf '\n## Thunderbolt/USB4 Port Snapshot\n\n'
    if [[ -n "$thunderbolt_snapshot" ]]; then
      printf '```text\n%s\n```\n' "$thunderbolt_snapshot"
    else
      printf '```text\nNo Thunderbolt/USB4 snapshot captured.\n```\n'
    fi
    printf '\n## USB Host Diagnosis\n\n'
    printf '%s\n' "$usb_host_diagnosis"
    printf '\n## ISP Entry Notes\n\n'
    printf '%s\n' '- Expected WCH ISP USB IDs: `4348:55e0` or `1a86:55e0`.'
    printf '%s\n' '- PX1 schematic labels `BTN2 (ISP)` on `PB9/BOOT0`; `PB2/BOOT1` is also present on the MCU.'
    printf '%s\n' '- Manual entry: hold `BTN2/BOOT` while reconnecting USB, then run this script while the device remains in ISP mode.'
    printf '%s\n' '- Keep `BTN2/BOOT` pressed before USB insertion, hold for 1-2 seconds after insertion, then release.'
    printf '%s\n' '- If no USB VID/PID device is visible, test a known data-capable USB-C cable and direct Mac port before debugging firmware.'
    printf '%s\n' '- If another USB device appears but not WCH ISP, re-check BOOT0 level on `PB9/BOOT0`, reset timing, and whether the MCU is entering user firmware instead of Bootloader.'
  } >"$FLASH_REPORT"

  if [[ "$status" == "flashed" ]]; then
    cp "$FLASH_REPORT" "$FLASH_SUCCESS_REPORT"
  fi
}

if [[ "${1:-}" == "--wait-forever" ]]; then
  WAIT_SECONDS="forever"
  WAIT_FOREVER=1
  shift
elif [[ "${1:-}" == "--wait" ]]; then
  if [[ "${2:-}" =~ ^[0-9]+$ ]]; then
    WAIT_SECONDS="$2"
    shift 2
  else
    WAIT_SECONDS="30"
    shift 1
  fi
fi

BIN="${1:-$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin}"

if [[ ! -f "$BIN" ]]; then
  printf 'Firmware not found: %s\n' "$BIN" >&2
  exit 1
fi

if [[ -x "${WCHISP:-}" ]]; then
  WCHISP_BIN="$WCHISP"
elif [[ -x "$HOME/.cargo/bin/wchisp" ]]; then
  WCHISP_BIN="$HOME/.cargo/bin/wchisp"
elif command -v wchisp >/dev/null 2>&1; then
  WCHISP_BIN="$(command -v wchisp)"
else
  printf 'wchisp not found. Install it or set WCHISP=/path/to/wchisp.\n' >&2
  WCHISP_BIN="not found"
  write_flash_report "failed" "wchisp not found"
  exit 1
fi

printf 'Firmware: %s\n' "$BIN"
shasum -a 256 "$BIN"
printf 'Probe: %s info\n' "$WCHISP_BIN"

if WCHISP_INFO_OUTPUT="$("$WCHISP_BIN" info 2>&1)"; then
  WCHISP_INFO_STATUS=0
else
  WCHISP_INFO_STATUS=$?
  printf '%s\n' "$WCHISP_INFO_OUTPUT" >&2
  if [[ "$WAIT_FOREVER" == "1" ]] || { [[ "$WAIT_SECONDS" =~ ^[0-9]+$ ]] && (( WAIT_SECONDS > 0 )); }; then
    if [[ "$WAIT_FOREVER" == "1" ]]; then
      printf 'Waiting indefinitely for WCH ISP device. Press Ctrl-C to stop.\n' >&2
    else
      printf 'Waiting up to %ss for WCH ISP device...\n' "$WAIT_SECONDS" >&2
      deadline=$((SECONDS + WAIT_SECONDS))
    fi
    printf 'Hold BTN2/BOOT, reconnect USB-C, keep it pressed for 1-2 seconds, then release.\n' >&2
    attempts=0
    while [[ "$WAIT_FOREVER" == "1" ]] || (( SECONDS < deadline )); do
      sleep 1
      attempts=$((attempts + 1))
      WCHISP_INFO_OUTPUT="$("$WCHISP_BIN" info 2>&1)" && {
        WCHISP_INFO_STATUS=0
        if ! info_output_matches_px1_chip; then
          write_flash_report "failed" "wchisp info succeeded, but target chip is not PX1; expected CH32L103K8U6."
          exit 1
        fi
        printf 'WCH ISP device appeared.\n' >&2
        printf '%s\n' "$WCHISP_INFO_OUTPUT"
        printf 'Flash: %s flash %s\n' "$WCHISP_BIN" "$BIN"
        if WCHISP_FLASH_OUTPUT="$("$WCHISP_BIN" flash "$BIN" 2>&1)"; then
          WCHISP_FLASH_STATUS=0
          printf '%s\n' "$WCHISP_FLASH_OUTPUT"
          if ! flash_output_has_verify_markers; then
            write_flash_report "failed" "wchisp flash exited 0, but output is missing Verify OK or Device reset."
            exit 1
          fi
          write_flash_report "flashed" "WCH ISP device appeared during wait and wchisp flash completed successfully."
          exit 0
        fi
        WCHISP_FLASH_STATUS=$?
        printf '%s\n' "$WCHISP_FLASH_OUTPUT" >&2
        write_flash_report "failed" "WCH ISP device appeared during wait, but wchisp flash command failed."
        exit 1
      }
      if (( attempts % 5 == 0 )); then
        printf '[%s] still waiting for 4348:55e0 or 1a86:55e0... (%ss elapsed)\n' \
          "$(date '+%H:%M:%S')" "$attempts" >&2
      fi
    done
  fi

  cat >&2 <<'MSG'
No WCH ISP device is visible.
Put the PX1 board into ISP/BOOT mode, reconnect USB, then run this script again.
For the PX1 board flow, hold BTN2/BOOT while reconnecting USB if manual ISP entry is needed.
You can also run: ./tools/flash_powerx.sh --wait 30
For hands-on board entry, run: ./tools/flash_powerx.sh --wait-forever
Expected USB IDs include 4348:55e0 or 1a86:55e0.
MSG
  write_flash_report "no-isp-device" "No WCH ISP USB device found; expected 4348:55e0 or 1a86:55e0."
  exit 2
fi

if ! info_output_matches_px1_chip; then
  write_flash_report "failed" "wchisp info succeeded, but target chip is not PX1; expected CH32L103K8U6."
  exit 1
fi

printf '%s\n' "$WCHISP_INFO_OUTPUT"

printf 'Flash: %s flash %s\n' "$WCHISP_BIN" "$BIN"
if WCHISP_FLASH_OUTPUT="$("$WCHISP_BIN" flash "$BIN" 2>&1)"; then
  WCHISP_FLASH_STATUS=0
  printf '%s\n' "$WCHISP_FLASH_OUTPUT"
  if ! flash_output_has_verify_markers; then
    write_flash_report "failed" "wchisp flash exited 0, but output is missing Verify OK or Device reset."
    exit 1
  fi
  write_flash_report "flashed" "wchisp flash completed successfully."
else
  WCHISP_FLASH_STATUS=$?
  printf '%s\n' "$WCHISP_FLASH_OUTPUT" >&2
  write_flash_report "failed" "wchisp flash command failed."
  exit 1
fi

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OBJ_DIR="$ROOT/PowerXCode/FreeRTOS/obj"
BIN="$OBJ_DIR/FreeRTOS.bin"
DEFAULT_TOOLCHAIN="/Applications/MounRiver Studio 2.app/Contents/Resources/app/resources/darwin/components/WCH/Toolchain/RISC-V Embedded GCC/bin"
WAIT_SECONDS="${PX1_FLASH_WAIT_SECONDS:-3}"
SKIP_FLASH=0
HARDWARE_REPORT=""

print_px1_usb_preflight() {
  printf '\n== PX1 USB preflight ==\n'
  "$ROOT/tools/check_wch_isp_device.sh" --wait 1 || true
  printf 'Preflight report: %s\n' "$ROOT/artifacts/flash/isp-preflight.md"
}

usage() {
  cat <<'MSG'
Usage: tools/run_powerx_goal_gate.sh [--wait seconds] [--no-flash] [--hardware-report report.md]

Runs the PX1 end-to-end gate:
  1. host tests and UI artifact checks
  2. WCH RISC-V cross build
  3. product ELF contract check
  4. firmware hash/size report
  5. optional filled hardware validation report check
  6. WCH ISP flash attempt, unless --no-flash is used
MSG
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --wait)
      [[ "${2:-}" =~ ^[0-9]+$ ]] || {
        printf 'Invalid --wait value: %s\n' "${2:-}" >&2
        exit 64
      }
      WAIT_SECONDS="$2"
      shift 2
      ;;
    --no-flash)
      SKIP_FLASH=1
      shift
      ;;
    --hardware-report)
      [[ -n "${2:-}" ]] || {
        printf 'Missing --hardware-report path\n' >&2
        exit 64
      }
      HARDWARE_REPORT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      printf 'Unknown argument: %s\n' "$1" >&2
      usage >&2
      exit 64
      ;;
  esac
done

printf '== PX1 host tests ==\n'
"$ROOT/tools/run_host_tests.sh"

printf '\n== PX1 cross build ==\n'
if [[ -d "$DEFAULT_TOOLCHAIN" ]]; then
  PATH="$DEFAULT_TOOLCHAIN:$PATH" make -C "$OBJ_DIR" clean all
else
  make -C "$OBJ_DIR" clean all
fi

printf '\n== PX1 product binary contract ==\n'
"$ROOT/tools/check_product_binary_contract.sh" "$OBJ_DIR/FreeRTOS.elf"

printf '\n== PX1 firmware artifact ==\n'
[[ -s "$BIN" ]] || {
  printf 'Firmware missing after build: %s\n' "$BIN" >&2
  exit 1
}
ls -lh "$BIN"
shasum -a 256 "$BIN"

printf '\n== PX1 gate self-tests ==\n'
"$ROOT/tools/test_powerx_hardware_validation_checker.sh"
"$ROOT/tools/test_powerx_objective_coverage_strict_hardware.sh"
"$ROOT/tools/test_powerx_objective_coverage_flash_evidence_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_full_success_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_key_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_cc_emark_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_settings_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_protocol_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_pd_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_qc_contract.sh"
"$ROOT/tools/test_powerx_objective_coverage_ui_contract.sh"
"$ROOT/tools/test_powerx_goal_completion_contract.sh"
"$ROOT/tools/test_powerx_goal_completion_flash_evidence_contract.sh"
"$ROOT/tools/test_powerx_goal_completion_full_success_contract.sh"
"$ROOT/tools/test_powerx_goal_completion_isp_diagnosis_report.sh"
"$ROOT/tools/test_powerx_prepare_hardware_report.sh"
"$ROOT/tools/test_powerx_record_flash_hardware_evidence.sh"
"$ROOT/tools/test_powerx_real_device_closure_report_contract.sh"
"$ROOT/tools/test_powerx_real_device_closure_success_contract.sh"
"$ROOT/tools/test_powerx_real_device_closure_failure_report.sh"
"$ROOT/tools/test_powerx_real_device_closure_isp_preflight_status_report.sh"
"$ROOT/tools/test_powerx_real_device_closure_refreshes_isp_preflight.sh"
"$ROOT/tools/test_powerx_real_device_closure_reports_isp_diagnosis.sh"
"$ROOT/tools/test_powerx_real_device_closure_skip_flash_blocks.sh"
"$ROOT/tools/test_powerx_real_device_closure_skip_flash_current_report_status.sh"
"$ROOT/tools/test_powerx_real_device_closure_stale_latest_attempt_report.sh"
"$ROOT/tools/test_powerx_real_device_closure_stale_success_report.sh"
"$ROOT/tools/test_powerx_flash_report_contract.sh"
"$ROOT/tools/test_powerx_flash_script_fake_wchisp.sh"
"$ROOT/tools/test_powerx_flash_script_failure_fake_wchisp.sh"
"$ROOT/tools/test_powerx_flash_script_no_isp_fake_wchisp.sh"
"$ROOT/tools/test_powerx_flash_script_stale_success_report.sh"
"$ROOT/tools/test_powerx_flash_script_success_requires_verify.sh"
"$ROOT/tools/test_powerx_flash_script_requires_px1_chip.sh"
"$ROOT/tools/test_powerx_isp_preflight_generic_usb_not_wch.sh"
"$ROOT/tools/test_powerx_isp_preflight_reports_wchisp_info_exit.sh"
"$ROOT/tools/test_powerx_isp_preflight_requires_px1_chip.sh"
"$ROOT/tools/test_powerx_isp_preflight_wch_serial_not_isp.sh"

if [[ -n "$HARDWARE_REPORT" ]]; then
  printf '\n== PX1 hardware validation report ==\n'
  "$ROOT/tools/check_powerx_hardware_validation.sh" "$HARDWARE_REPORT" "$BIN"
fi

if [[ "$SKIP_FLASH" == "1" ]]; then
  printf '\n== PX1 flash skipped ==\n'
  exit 0
fi

print_px1_usb_preflight

printf '\n== PX1 WCH ISP flash ==\n'
"$ROOT/tools/flash_powerx.sh" --wait "$WAIT_SECONDS" "$BIN"

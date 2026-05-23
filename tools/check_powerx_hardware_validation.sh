#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPORT="${1:-$ROOT/artifacts/hardware-validation/latest.md}"
BIN="${2:-$ROOT/PowerXCode/FreeRTOS/obj/FreeRTOS.bin}"
FLASH_REPORT="${PX1_FLASH_REPORT:-$ROOT/artifacts/flash/latest-success.md}"

fail() {
  printf 'FAIL hardware-validation: %s\n' "$1" >&2
  exit 1
}

usage() {
  cat <<'MSG'
Usage: tools/check_powerx_hardware_validation.sh [report.md] [firmware.bin]

Checks a filled PX1 hardware validation report against the current firmware.
The report must contain:
  Firmware SHA256: <sha256 of firmware.bin>
  artifacts/flash/latest-success.md with Status: `flashed` and the same firmware SHA256
  Raw wchisp info and wchisp flash output with Verify OK and Device reset evidence
  Non-empty board, charger, PD/QC measurement, cable, and stability notes
  - [x] flash-wchisp
  - [x] boot-backlight
  - [x] lcd-main-no-garble
  - [x] ui-all-9-pages
  - [x] buttons-browse-and-action
  - [x] pd-detect-source-cap
  - [x] pd-trigger-request-vbus
  - [x] qc-detect-dpdm
  - [x] qc-trigger-request-vbus
  - [x] cc-orientation
  - [x] cable-emarker
  - [x] settings-brightness-rotation-mode
  - [x] stability-no-freeze-5min
MSG
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

[[ -s "$BIN" ]] || fail "missing or empty firmware: $BIN"
[[ -s "$REPORT" ]] || fail "missing or empty report: $REPORT"

CURRENT_SHA="$(shasum -a 256 "$BIN" | awk '{ print $1 }')"

grep -Fq "Firmware SHA256: $CURRENT_SHA" "$REPORT" \
  || fail "report does not match current firmware SHA256: $CURRENT_SHA"

require_checked_item() {
  local item="$1"

  grep -Eq "^[[:space:]]*-[[:space:]]*\\[[xX]\\][[:space:]]+${item}([[:space:]]|$)" "$REPORT" \
    || fail "missing checked hardware item: $item"
}

require_checked_item "flash-wchisp"
require_checked_item "boot-backlight"
require_checked_item "lcd-main-no-garble"
require_checked_item "ui-all-9-pages"
require_checked_item "buttons-browse-and-action"
require_checked_item "pd-detect-source-cap"
require_checked_item "pd-trigger-request-vbus"
require_checked_item "qc-detect-dpdm"
require_checked_item "qc-trigger-request-vbus"
require_checked_item "cc-orientation"
require_checked_item "cable-emarker"
require_checked_item "settings-brightness-rotation-mode"
require_checked_item "stability-no-freeze-5min"

require_nonempty_field() {
  local label="$1"
  local value

  value="$(awk -F': *' -v label="$label" '
    $0 ~ "^[[:space:]]*-[[:space:]]*" label ":" {
      sub("^[[:space:]]*-[[:space:]]*" label ":[[:space:]]*", "", $0)
      print
      exit
    }
  ' "$REPORT")"

  [[ -n "$value" ]] || fail "missing non-empty hardware report field: $label"
  [[ ! "$value" =~ ^(TODO|TBD|N/A|NA|待填|无数据)$ ]] \
    || fail "hardware report field is not real evidence: $label"
  [[ ! "$value" =~ ^([Oo][Kk]|[Pp][Aa][Ss][Ss]|[Yy][Ee][Ss]|[Dd][Oo][Nn][Ee]|已测|通过|正常)$ ]] \
    || fail "hardware report field is too generic to prove real evidence: $label"
  if grep -Eq '(^PX1-$|__|YYYY-MM-DD|品牌/型号|需支持|类型/速度)' <<<"$value"; then
    fail "hardware report field still contains template placeholder text: $label"
  fi
}

field_value() {
  local label="$1"

  awk -F': *' -v label="$label" '
    $0 ~ "^[[:space:]]*-[[:space:]]*" label ":" {
      sub("^[[:space:]]*-[[:space:]]*" label ":[[:space:]]*", "", $0)
      print
      exit
    }
  ' "$REPORT"
}

require_field_regex() {
  local label="$1"
  local regex="$2"
  local message="$3"
  local value

  value="$(field_value "$label")"
  [[ -n "$value" ]] || fail "missing non-empty hardware report field: $label"
  if ! grep -Eiq "$regex" <<<"$value"; then
    fail "$message: $label"
  fi
}

require_nonempty_field "板子编号"
require_nonempty_field "烧录时间"
require_nonempty_field "电源/PD 充电器型号"
require_nonempty_field "QC 充电器型号"
require_nonempty_field "PD 请求目标与 INA226 实测"
require_nonempty_field "QC 请求目标与 INA226 实测"
require_nonempty_field "线缆/E-marker 结果"
require_nonempty_field "UI 页面确认"
require_nonempty_field "按键确认"
require_nonempty_field "设置确认"
require_nonempty_field "5分钟稳定性"
require_nonempty_field "证据文件"

require_field_regex "烧录时间" '[0-9]{4}-[0-9]{2}-[0-9]{2}' \
  "hardware report burn time must include a concrete date"
require_field_regex "PD 请求目标与 INA226 实测" '([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv)).*([0-9]+([.][0-9]+)?[[:space:]]*(A|a|mA|ma))|([0-9]+([.][0-9]+)?[[:space:]]*(A|a|mA|ma)).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' \
  "PD evidence must include measured voltage and current"
require_field_regex "PD 请求目标与 INA226 实测" '(Source[_ -]?Cap|PDO|APDO|PPS).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' \
  "PD evidence must include detected Source_Cap/PDO voltage evidence"
require_field_regex "PD 请求目标与 INA226 实测" '(目标|target|request|请求).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' \
  "PD evidence must include the requested trigger target voltage"
require_field_regex "QC 请求目标与 INA226 实测" '(DP|D[+＋]|DM|D[-－]).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv)).*(VBUS|BUS|vbus).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' \
  "QC evidence must include DP/DM and VBUS voltage"
require_field_regex "QC 请求目标与 INA226 实测" '([0-9]+([.][0-9]+)?[[:space:]]*(A|a|mA|ma))' \
  "QC evidence must include measured current"
require_field_regex "QC 请求目标与 INA226 实测" '(QC[[:space:]]*[23]([.]0)?|Quick[[:space:]-]?Charge)' \
  "QC evidence must include detected QC2/QC3 mode evidence"
require_field_regex "QC 请求目标与 INA226 实测" '(目标|target|request|请求).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' \
  "QC evidence must include the requested trigger target voltage"
require_field_regex "线缆/E-marker 结果" '(CC1|CC2|cc1|cc2).*(E-marker|E-Marker|emark|EMARK|线缆|电流|A|a)|((E-marker|E-Marker|emark|EMARK).*(CC1|CC2|cc1|cc2))' \
  "cable evidence must include CC orientation and E-marker/cable result"
require_field_regex "UI 页面确认" '(main|scope|protocol|trigger|pdo|qc|cc|cable|settings).*(9|九|全部|逐页)|9.*(main|scope|protocol|trigger|pdo|qc|cc|cable|settings)' \
  "UI evidence must name the 9-page sweep"
for page in main scope protocol trigger pdo qc cc cable settings; do
  require_field_regex "UI 页面确认" "$page" \
    "UI evidence must explicitly include every product page, missing $page"
done
require_field_regex "按键确认" '(BTN1|SW1).*(BTN2|SW2).*(BTN3|SW3)|(BTN1|SW1).*(BTN3|SW3).*(BTN2|SW2)' \
  "button evidence must include BTN1/BTN2/BTN3 or SW1/SW2/SW3"
require_field_regex "设置确认" '(brightness|亮度).*(rotation|旋转).*(TRIG|trigger|诱骗)|(TRIG|trigger|诱骗).*(brightness|亮度).*(rotation|旋转)' \
  "settings evidence must include brightness, rotation, and trigger mode"
require_field_regex "5分钟稳定性" '5[[:space:]]*(min|分钟|m).*?(无卡死|不卡死|无死机|稳定|正常|无花屏)|([五]分钟).*?(无卡死|不卡死|无死机|稳定|正常|无花屏)' \
  "stability evidence must include a 5-minute no-freeze observation"

require_evidence_files() {
  local value
  local normalized
  local raw_path
  local evidence_path
  local count=0
  local evidence_blob=""
  local page

  value="$(field_value "证据文件")"
  [[ -n "$value" ]] || fail "missing non-empty hardware report field: 证据文件"

  normalized="${value//；/;}"
  normalized="${normalized//，/;}"
  normalized="${normalized//,/;}"

  IFS=';' read -r -a evidence_paths <<<"$normalized"
  for raw_path in "${evidence_paths[@]}"; do
    evidence_path="$(awk '{$1=$1; print}' <<<"${raw_path//\`/}")"
    [[ -n "$evidence_path" ]] || continue
    if [[ "$evidence_path" != /* ]]; then
      evidence_path="$ROOT/$evidence_path"
    fi
    [[ -s "$evidence_path" ]] \
      || fail "hardware evidence file is missing or empty: $evidence_path"
    if grep -Eiq '(TODO|TBD|待填|示例|example|REPLACE|__|YYYY-MM-DD)' "$evidence_path"; then
      fail "hardware evidence file still contains placeholder text: $evidence_path"
    fi
    grep -Fq "$CURRENT_SHA" "$evidence_path" \
      || fail "hardware evidence file does not reference current firmware SHA256: $evidence_path"
    evidence_blob="${evidence_blob}"$'\n'"$(cat "$evidence_path")"
    count=$((count + 1))
  done

  [[ "$count" -ge 3 ]] \
    || fail "hardware report must reference at least 3 non-empty evidence files"

  for page in main scope protocol trigger pdo qc cc cable settings; do
    if ! grep -Eiq "$page" <<<"$evidence_blob"; then
      fail "hardware evidence files must include UI page evidence, missing $page"
    fi
  done
  if ! grep -Eiq '(Source[_ -]?Cap|PDO|APDO|PPS).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' <<<"$evidence_blob"; then
    fail "hardware evidence files must include PD Source_Cap/PDO voltage evidence"
  fi
  if ! grep -Eiq '(目标|target|request|请求).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' <<<"$evidence_blob"; then
    fail "hardware evidence files must include requested trigger target voltage"
  fi
  if ! grep -Eiq '(VBUS|vbus|BUS).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv)).*([0-9]+([.][0-9]+)?[[:space:]]*(A|a|mA|ma))' <<<"$evidence_blob"; then
    fail "hardware evidence files must include VBUS voltage and current evidence"
  fi
  if ! grep -Eiq '(QC[[:space:]]*[23]([.]0)?|Quick[[:space:]-]?Charge)' <<<"$evidence_blob"; then
    fail "hardware evidence files must include QC2/QC3 mode evidence"
  fi
  if ! grep -Eiq '(DP|D[+＋]).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv)).*(DM|D[-－]).*([0-9]+([.][0-9]+)?[[:space:]]*(V|v|mV|mv))' <<<"$evidence_blob"; then
    fail "hardware evidence files must include DP/DM voltage evidence"
  fi
  if ! grep -Eiq '(BTN1|SW1).*(BTN2|SW2).*(BTN3|SW3)|(BTN1|SW1).*(BTN3|SW3).*(BTN2|SW2)' <<<"$evidence_blob"; then
    fail "hardware evidence files must include BTN1/BTN2/BTN3 evidence"
  fi
  if ! grep -Eiq '(brightness|亮度).*(rotation|旋转).*(TRIG|trigger|诱骗)|(TRIG|trigger|诱骗).*(brightness|亮度).*(rotation|旋转)' <<<"$evidence_blob"; then
    fail "hardware evidence files must include brightness, rotation, and trigger setting evidence"
  fi
  if ! grep -Eiq '(CC1|CC2|cc1|cc2).*(active|方向|orientation)' <<<"$evidence_blob"; then
    fail "hardware evidence files must include CC orientation evidence"
  fi
  if ! grep -Eiq '(E-marker|E-Marker|emark|EMARK).*(cable|线缆|current|电流|[0-9]+([.][0-9]+)?[[:space:]]*(A|a))' <<<"$evidence_blob"; then
    fail "hardware evidence files must include E-marker/cable current evidence"
  fi
}

require_evidence_files

require_flash_evidence_field() {
  local expected
  local value

  expected="$FLASH_REPORT"
  if [[ "$expected" == "$ROOT/"* ]]; then
    expected="${expected#"$ROOT/"}"
  fi

  value="$(awk -F': *' '
    $0 ~ "^[[:space:]]*-[[:space:]]*Flash evidence:" {
      sub("^[[:space:]]*-[[:space:]]*Flash evidence:[[:space:]]*", "", $0)
      gsub("`", "", $0)
      print
      exit
    }
  ' "$REPORT")"

  [[ -n "$value" ]] || fail "missing flash evidence field in hardware report"
  [[ "$value" == "$expected" || "$value" == "$FLASH_REPORT" ]] \
    || fail "hardware report flash evidence field must point to $expected"
}

require_flash_evidence() {
  [[ -s "$FLASH_REPORT" ]] || fail "missing flash evidence report: $FLASH_REPORT"

  grep -Fq "Firmware SHA256: \`$CURRENT_SHA\`" "$FLASH_REPORT" \
    || fail "flash evidence does not match current firmware SHA256: $CURRENT_SHA"

  grep -Eq '^[[:space:]]*-[[:space:]]*Status:[[:space:]]*`flashed`[[:space:]]*$' "$FLASH_REPORT" \
    || fail "flash evidence is not successful: $FLASH_REPORT"

  grep -Eq '^[[:space:]]*-[[:space:]]*Time:[[:space:]]*[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2}:[0-9]{2}[[:space:]]+[+-][0-9]{4}[[:space:]]*$' "$FLASH_REPORT" \
    || fail "flash evidence is missing a concrete timestamp: $FLASH_REPORT"

  grep -Eq '^[[:space:]]*-[[:space:]]*wchisp:[[:space:]]*`[^`]+wchisp`[[:space:]]*$' "$FLASH_REPORT" \
    || fail "flash evidence is missing the wchisp executable path: $FLASH_REPORT"

  grep -Fq '## wchisp info' "$FLASH_REPORT" \
    || fail "flash evidence is missing wchisp info output: $FLASH_REPORT"

  grep -Fq 'CH32L103K8U6' "$FLASH_REPORT" \
    || fail "flash evidence is missing PX1 CH32L103K8U6 target chip output: $FLASH_REPORT"

  grep -Fq 'wchisp info exit: `0`' "$FLASH_REPORT" \
    || fail "flash evidence is missing successful wchisp info exit code: $FLASH_REPORT"

  grep -Fq '## wchisp flash' "$FLASH_REPORT" \
    || fail "flash evidence is missing raw wchisp flash output: $FLASH_REPORT"

  grep -Fq 'wchisp flash exit: `0`' "$FLASH_REPORT" \
    || fail "flash evidence is missing successful wchisp flash exit code: $FLASH_REPORT"

  grep -Fq 'Verify OK' "$FLASH_REPORT" \
    || fail "flash evidence is missing Verify OK output: $FLASH_REPORT"

  grep -Fq 'Device reset' "$FLASH_REPORT" \
    || fail "flash evidence is missing Device reset output: $FLASH_REPORT"
}

extract_timestamp() {
  local value="$1"

  grep -Eo '[0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2}(:[0-9]{2})?([[:space:]]+[+-][0-9]{4})?' <<<"$value" | head -n 1 || true
}

parse_timestamp_epoch() {
  local timestamp="$1"
  local normalized
  local epoch
  local fmt

  normalized="$timestamp"
  if [[ "$normalized" =~ ^([0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2})([[:space:]]+[+-][0-9]{4})$ ]]; then
    normalized="${BASH_REMATCH[1]}:00${BASH_REMATCH[2]}"
  elif [[ "$normalized" =~ ^([0-9]{4}-[0-9]{2}-[0-9]{2}[[:space:]]+[0-9]{2}:[0-9]{2})$ ]]; then
    normalized="${BASH_REMATCH[1]}:00"
  fi

  for fmt in '%Y-%m-%d %H:%M:%S %z' '%Y-%m-%d %H:%M:%S'; do
    if epoch="$(date -j -f "$fmt" "$normalized" '+%s' 2>/dev/null)"; then
      printf '%s\n' "$epoch"
      return 0
    fi
  done

  return 1
}

require_hardware_burn_time_not_before_flash() {
  local burn_raw
  local burn_timestamp
  local burn_epoch
  local flash_raw
  local flash_timestamp
  local flash_epoch

  burn_raw="$(field_value "烧录时间")"
  burn_timestamp="$(extract_timestamp "$burn_raw")"
  [[ -n "$burn_timestamp" ]] || fail "hardware report burn time must include YYYY-MM-DD HH:MM"

  flash_raw="$(awk '
    $0 ~ "^[[:space:]]*-[[:space:]]*Time:" {
      sub("^[[:space:]]*-[[:space:]]*Time:[[:space:]]*", "", $0)
      print
      exit
    }
  ' "$FLASH_REPORT")"
  flash_timestamp="$(extract_timestamp "$flash_raw")"
  [[ -n "$flash_timestamp" ]] || fail "flash evidence time cannot be parsed: $FLASH_REPORT"

  if ! burn_epoch="$(parse_timestamp_epoch "$burn_timestamp")"; then
    fail "hardware report burn time cannot be parsed: $burn_raw"
  fi
  if ! flash_epoch="$(parse_timestamp_epoch "$flash_timestamp")"; then
    fail "flash evidence time cannot be parsed: $flash_raw"
  fi

  (( burn_epoch >= flash_epoch )) \
    || fail "hardware report burn time is earlier than flash evidence time"
}

require_flash_evidence_field
require_flash_evidence
require_hardware_burn_time_not_before_flash

printf '%s\n' "PASS hardware-validation"

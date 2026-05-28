#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/artifacts/ui-preview"
RENDER_BIN="/tmp/render_current_ui"
LOCK_DIR="$OUT_DIR/.ui-preview.lock"

mkdir -p "$OUT_DIR"

acquire_ui_preview_lock() {
  local waited

  if [[ "${PX1_UI_PREVIEW_LOCK_HELD:-0}" == "1" ]]; then
    return
  fi

  waited=0
  while ! mkdir "$LOCK_DIR" 2>/dev/null; do
    sleep 0.1
    waited=$((waited + 1))
    if (( waited >= 600 )); then
      printf 'FAIL render-ui-preview: timed out waiting for lock: %s\n' "$LOCK_DIR" >&2
      exit 1
    fi
  done

  trap 'rmdir "$LOCK_DIR" 2>/dev/null || true' EXIT
}

acquire_ui_preview_lock

cc -std=c99 -Wall -Wextra \
  -I"$OUT_DIR" \
  -I"$ROOT/PowerXCode/FreeRTOS/Bsp/include" \
  -I"$ROOT/PowerXCode/FreeRTOS/UI/include" \
  -I"$ROOT/PowerXCode/FreeRTOS/Service/include" \
  -I"$ROOT/PowerXCode/FreeRTOS/Assets" \
  "$OUT_DIR/render_current_ui.c" \
  "$ROOT/PowerXCode/FreeRTOS/UI/ui_renderer.c" \
  "$ROOT/PowerXCode/FreeRTOS/UI/ui_model.c" \
  "$ROOT/PowerXCode/FreeRTOS/UI/ui_widgets.c" \
  -o "$RENDER_BIN"

"$RENDER_BIN" "$OUT_DIR/current-ui-atlas.ppm" "$OUT_DIR/current-ui-protocol-warning.ppm"
sips -s format png "$OUT_DIR/current-ui-atlas.ppm" --out "$OUT_DIR/current-ui-atlas.png" >/dev/null
sips -s format png "$OUT_DIR/current-ui-protocol-warning.ppm" --out "$OUT_DIR/current-ui-protocol-warning-160x80.png" >/dev/null
sips -z 320 640 "$OUT_DIR/current-ui-protocol-warning-160x80.png" --out "$OUT_DIR/current-ui-protocol-warning-640x320.png" >/dev/null
sips -z 1280 2560 "$OUT_DIR/current-ui-atlas.png" --out "$OUT_DIR/current-ui-atlas-4x.png" >/dev/null

crop_tile() {
  local name="$1"
  local x="$2"
  local y="$3"
  local png="$OUT_DIR/current-ui-${name}-160x80.png"
  local png4x="$OUT_DIR/current-ui-${name}-640x320.png"

  ffmpeg -y -v error \
    -i "$OUT_DIR/current-ui-atlas.png" \
    -vf "crop=160:80:${x}:${y}" \
    "$png"
  sips -z 320 640 "$png" --out "$png4x" >/dev/null
}

crop_tile main 0 0
crop_tile dpdm 160 0
crop_tile power 320 0
crop_tile capacity 480 0
crop_tile protocol 0 80
crop_tile pdo 160 80
crop_tile emark 320 80
crop_tile scope 480 80
crop_tile ripple 0 160
crop_tile settings 160 160
crop_tile menu 320 160
crop_tile trigger-select 480 160
crop_tile trigger-adjust 0 240

sips -g pixelWidth -g pixelHeight \
  "$OUT_DIR/current-ui-atlas.png" \
  "$OUT_DIR/current-ui-atlas-4x.png" \
  "$OUT_DIR/current-ui-main-160x80.png" \
  "$OUT_DIR/current-ui-menu-160x80.png"

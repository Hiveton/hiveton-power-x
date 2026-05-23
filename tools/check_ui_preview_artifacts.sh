#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/artifacts/ui-preview"
REPORT="$OUT_DIR/current-ui-report.md"
LOCK_DIR="$OUT_DIR/.ui-preview.lock"

fail() {
  printf 'FAIL ui-preview-artifacts: %s\n' "$1" >&2
  exit 1
}

acquire_ui_preview_lock() {
  local waited

  mkdir -p "$OUT_DIR"
  waited=0
  while ! mkdir "$LOCK_DIR" 2>/dev/null; do
    sleep 0.1
    waited=$((waited + 1))
    if (( waited >= 600 )); then
      fail "timed out waiting for UI preview lock: $LOCK_DIR"
    fi
  done

  export PX1_UI_PREVIEW_LOCK_HELD=1
  trap 'rmdir "$LOCK_DIR" 2>/dev/null || true' EXIT
}

read_dimension() {
  local key="$1"
  local file="$2"

  sips -g "$key" "$file" 2>/dev/null | awk -v key="${key}:" '$1 == key { print $2; exit }'
}

require_png_size() {
  local file="$1"
  local expected_width="$2"
  local expected_height="$3"
  local width
  local height

  [[ -s "$file" ]] || fail "missing or empty artifact: $file"
  width="$(read_dimension pixelWidth "$file")"
  height="$(read_dimension pixelHeight "$file")"

  [[ "$width" == "$expected_width" ]] || fail "$file width is $width, expected $expected_width"
  [[ "$height" == "$expected_height" ]] || fail "$file height is $height, expected $expected_height"
}

count_nonblack_pixels() {
  local file="$1"

  ffmpeg -v error -i "$file" -f rawvideo -pix_fmt rgb24 - 2>/dev/null |
    perl -e '
      binmode STDIN;
      my $nonblack = 0;
      while (read(STDIN, my $rgb, 3) == 3) {
        my ($r, $g, $b) = unpack("C3", $rgb);
        $nonblack++ if ($r > 8 || $g > 8 || $b > 8);
      }
      print "$nonblack\n";
    '
}

require_png_nonblank() {
  local file="$1"
  local page="$2"
  local nonblack

  nonblack="$(count_nonblack_pixels "$file")"
  [[ "$nonblack" =~ ^[0-9]+$ ]] || fail "$file nonblack count is not numeric"
  [[ "$nonblack" -ge 200 ]] || fail "$page page has only $nonblack nonblack pixels"
}

count_frame_pixels() {
  local file="$1"

  ffmpeg -v error -i "$file" -f rawvideo -pix_fmt rgb24 - 2>/dev/null |
    perl -e '
      binmode STDIN;
      my ($width, $height) = (160, 80);
      my $frame = 0;
      my $index = 0;
      while (read(STDIN, my $rgb, 3) == 3) {
        my $x = $index % $width;
        my $y = int($index / $width);
        if (($x == 0 || $x == $width - 1 || $y == 0 || $y == $height - 1)) {
          my ($r, $g, $b) = unpack("C3", $rgb);
          $frame++ if ($b >= 35 && $g >= 20 && $r <= 40);
        }
        ++$index;
      }
      print "$frame\n";
    '
}

count_named_color_pixels() {
  local file="$1"
  local color="$2"

  ffmpeg -v error -i "$file" -f rawvideo -pix_fmt rgb24 - 2>/dev/null |
    perl -e '
use strict;
use warnings;

my ($color) = @ARGV;
binmode STDIN;
my $count = 0;

sub matches_color {
  my ($name, $r, $g, $b) = @_;
  return ($r < 60 && $g > 180 && $b > 180) if $name eq "cyan";
  return ($r < 80 && $g > 180 && $b < 140) if $name eq "green";
  return ($r > 220 && $g > 150 && $b < 120) if $name eq "amber";
  return ($r > 220 && $g > 220 && $b > 220) if $name eq "white";
  return 0;
}

while (read(STDIN, my $rgb, 3) == 3) {
  my ($r, $g, $b) = unpack("C3", $rgb);
  ++$count if matches_color($color, $r, $g, $b);
}

print "$count\n";
' "$color"
}

require_page_color_content() {
  local page="$1"
  local color="$2"
  local min_count="$3"
  local file="$OUT_DIR/current-ui-${page}-160x80.png"
  local count

  count="$(count_named_color_pixels "$file" "$color")"
  [[ "$count" =~ ^[0-9]+$ ]] || fail "$page $color count is not numeric"
  [[ "$count" -ge "$min_count" ]] || fail "$page page has only $count $color pixels, expected >= $min_count"
}

require_png_frame() {
  local file="$1"
  local page="$2"
  local frame

  frame="$(count_frame_pixels "$file")"
  [[ "$frame" =~ ^[0-9]+$ ]] || fail "$file frame count is not numeric"
  [[ "$frame" -ge 300 ]] || fail "$page page has only $frame product-frame pixels"
}

write_ui_preview_report() {
  local page
  local file
  local width
  local height
  local nonblack
  local frame
  local cyan
  local green
  local amber
  local white

  {
    printf '# PX1 UI Preview Report\n\n'
    printf 'Atlas: `%s`\n\n' "$OUT_DIR/current-ui-atlas.png"
    printf '| page | size | nonblack | frame | cyan | green | amber | white |\n'
    printf '| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |\n'

    for page in main protocol trigger cc cable settings scope pdo qc; do
      file="$OUT_DIR/current-ui-${page}-160x80.png"
      width="$(read_dimension pixelWidth "$file")"
      height="$(read_dimension pixelHeight "$file")"
      nonblack="$(count_nonblack_pixels "$file")"
      frame="$(count_frame_pixels "$file")"
      cyan="$(count_named_color_pixels "$file" cyan)"
      green="$(count_named_color_pixels "$file" green)"
      amber="$(count_named_color_pixels "$file" amber)"
      white="$(count_named_color_pixels "$file" white)"
      printf '| %s | %sx%s | %s | %s | %s | %s | %s | %s |\n' \
        "$page" "$width" "$height" "$nonblack" "$frame" "$cyan" "$green" "$amber" "$white"
    done

    printf '\nGenerated by `tools/check_ui_preview_artifacts.sh` after all artifact checks passed.\n'
  } >"$REPORT"
}

acquire_ui_preview_lock
"$ROOT/tools/render_ui_preview.sh" >/dev/null

require_png_size "$OUT_DIR/current-ui-atlas.png" 480 240
require_png_size "$OUT_DIR/current-ui-atlas-4x.png" 1920 960

for page in main protocol trigger cc cable settings scope pdo qc; do
  require_png_size "$OUT_DIR/current-ui-${page}-160x80.png" 160 80
  require_png_nonblank "$OUT_DIR/current-ui-${page}-160x80.png" "$page"
  require_png_frame "$OUT_DIR/current-ui-${page}-160x80.png" "$page"
  require_png_size "$OUT_DIR/current-ui-${page}-640x320.png" 640 320
done

require_page_color_content scope cyan 250
require_page_color_content scope green 300
require_page_color_content scope amber 120
require_page_color_content pdo cyan 150
require_page_color_content pdo green 60
require_page_color_content pdo amber 150
require_page_color_content pdo white 250
require_page_color_content qc cyan 180
require_page_color_content qc green 180
require_page_color_content qc amber 350
require_page_color_content qc white 60

write_ui_preview_report

printf '%s\n' "PASS ui-preview-artifacts"

#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT_DIR="$ROOT/artifacts/ui-preview"
LOCK_DIR="$OUT_DIR/.ui-preview.lock"

fail() {
  printf 'FAIL ui-reference-delta: %s\n' "$1" >&2
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

PIXEL_DELTA_TOLERANCE=16

metric_for_pair() {
  local reference="$1"
  local current="$2"

  perl - "$reference" "$current" "$PIXEL_DELTA_TOLERANCE" <<'PERL'
use strict;
use warnings;

my ($reference, $current, $tolerance) = @ARGV;

sub raw_rgb {
    my ($file) = @_;
    open(my $fh, "-|", "ffmpeg", "-v", "error", "-i", $file, "-f", "rawvideo", "-pix_fmt", "rgb24", "-")
      or die "ffmpeg failed for $file\n";
    binmode($fh);
    local $/;
    my $data = <$fh>;
    close($fh);
    return $data;
}

my $a = raw_rgb($reference);
my $b = raw_rgb($current);
die "raw sizes differ\n" if length($a) != length($b);

my $pixels = length($a) / 3;
my $diff_pixels = 0;
my $sum_delta = 0;
my $max_delta = 0;

for (my $i = 0; $i < length($a); $i += 3) {
    my ($ar, $ag, $ab) = unpack("C3", substr($a, $i, 3));
    my ($br, $bg, $bb) = unpack("C3", substr($b, $i, 3));
    my $delta = abs($ar - $br) + abs($ag - $bg) + abs($ab - $bb);
    if ($delta > $tolerance) {
        ++$diff_pixels;
    }
    $sum_delta += $delta;
    $max_delta = $delta if $delta > $max_delta;
}

printf("%u %.2f %.2f %u\n",
       $diff_pixels,
       ($diff_pixels * 100.0) / $pixels,
       $sum_delta / $pixels,
       $max_delta);
PERL
}

require_delta_under() {
  local page="$1"
  local max_diff_pct="$2"
  local reference="$OUT_DIR/powerx-ui-${page}-160x80.png"
  local current="$OUT_DIR/current-ui-${page}-160x80.png"
  local metrics
  local diff_pixels
  local diff_pct
  local mean_delta
  local max_delta

  [[ -s "$reference" ]] || fail "missing reference image: $reference"
  [[ -s "$current" ]] || fail "missing current image: $current"

  metrics="$(metric_for_pair "$reference" "$current")"
  read -r diff_pixels diff_pct mean_delta max_delta <<<"$metrics"
  printf '%s diff_pixels=%s diff_pct=%s mean_rgb_delta=%s max_delta=%s tolerance=%s max_allowed=%s\n' \
    "$page" "$diff_pixels" "$diff_pct" "$mean_delta" "$max_delta" "$PIXEL_DELTA_TOLERANCE" "$max_diff_pct"

  awk -v actual="$diff_pct" -v max="$max_diff_pct" 'BEGIN { exit !(actual <= max) }' \
    || fail "$page differs by ${diff_pct}%, expected <= ${max_diff_pct}%"
}

acquire_ui_preview_lock
"$ROOT/tools/render_ui_preview.sh" >/dev/null

for page in main dpdm power capacity protocol pdo emark scope ripple settings menu; do
  require_delta_under "$page" 1.0
done

printf '%s\n' "PASS ui-reference-delta"

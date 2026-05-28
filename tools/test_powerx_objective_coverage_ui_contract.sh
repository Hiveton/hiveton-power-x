#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_powerx_objective_coverage.sh"

fail() {
  printf 'FAIL objective-coverage-ui-contract-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing objective coverage checker: $CHECKER"

required_renderer_symbols=(
  "ui_renderer_draw_main_page"
  "ui_renderer_draw_dpdm_page"
  "ui_renderer_draw_power_stats_page"
  "ui_renderer_draw_capacity_page"
  "ui_renderer_draw_scope_page"
  "ui_renderer_draw_ripple_page"
  "ui_renderer_draw_protocol_page"
  "ui_renderer_draw_pdo_page"
  "ui_renderer_draw_emark_page"
  "ui_renderer_draw_menu_page"
  "ui_renderer_draw_settings_page"
)

for symbol in "${required_renderer_symbols[@]}"; do
  rg -Fq "$symbol" "$CHECKER" \
    || fail "objective coverage does not require 9-page renderer symbol: $symbol"
done

required_behavior_tests=(
  "test_main_page_routes_to_main_renderer"
  "test_extra_pages_route_to_dedicated_renderers"
  "test_invalid_page_falls_back_to_product_home"
)

for test_name in "${required_behavior_tests[@]}"; do
  rg -Fq "$test_name" "$CHECKER" \
    || fail "objective coverage does not require UI page routing behavior test: $test_name"
done

printf '%s\n' "PASS objective-coverage-ui-contract-test"

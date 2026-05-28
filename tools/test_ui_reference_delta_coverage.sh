#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHECKER="$ROOT/tools/check_ui_reference_delta.sh"

fail() {
  printf 'FAIL ui-reference-delta-coverage-test: %s\n' "$1" >&2
  exit 1
}

[[ -s "$CHECKER" ]] || fail "missing checker: $CHECKER"

for page in main dpdm power capacity protocol pdo emark scope ripple settings menu; do
  rg -q "for page in main dpdm power capacity protocol pdo emark scope ripple settings menu" "$CHECKER" \
    || fail "checker does not compare page against design reference: $page"
done

printf '%s\n' "PASS ui-reference-delta-coverage-test"

#!/usr/bin/env bash
set -euo pipefail

# Resolve script directory
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
CPP_DIR="$SCRIPT_DIR/../cpp"
CASES_DIR="$SCRIPT_DIR/../public_test_cases"

echo "[build_all_public] Building converter..."
make -C "$CPP_DIR"

MODE_FLAG="${1:---naive}"
if [[ "$MODE_FLAG" != "--naive" && "$MODE_FLAG" != "--greedy" ]]; then
  echo "Usage: $0 [--naive|--greedy]" >&2
  exit 1
fi

echo "[build_all_public] Converting all .ir under public_test_cases to .s in cpp/ ($MODE_FLAG)"
while IFS= read -r -d '' ir; do
  base="$(basename "${ir}" .ir)"
  out="$CPP_DIR/${base}.s"
  echo "  -> $ir -> $out"
  "$CPP_DIR/bin/ir_to_mips" "$ir" "$out" "$MODE_FLAG"
done < <(find "$CASES_DIR" -type f -name '*.ir' -print0)

echo "[build_all_public] Done."



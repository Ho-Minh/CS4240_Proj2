#!/usr/bin/env bash
set -euo pipefail

# Resolve script directory
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
CPP_DIR="$SCRIPT_DIR/../cpp"
CASES_DIR="$SCRIPT_DIR/../public_test_cases"

echo "[build_all_public] Building converter..."
make -C "$CPP_DIR"

echo "[build_all_public] Converting all .ir under public_test_cases to .s in cpp/"
while IFS= read -r -d '' ir; do
  base="$(basename "${ir}" .ir)"
  out="$CPP_DIR/${base}.s"
  echo "  -> $ir -> $out"
  "$CPP_DIR/bin/ir_to_mips" "$ir" "$out"
done < <(find "$CASES_DIR" -type f -name '*.ir' -print0)

echo "[build_all_public] Done."



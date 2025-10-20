#!/usr/bin/env bash
set -euo pipefail

# Usage: run.sh <input.ir> <output.s> [--naive|--greedy]

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo "Usage: $0 <input.ir> <output.s> [--naive|--greedy]" >&2
  exit 1
fi

IN_IR="$1"
OUT_S="$2"
MODE_FLAG="${3:---naive}"

echo "Building (make) ..."
make -C "$SCRIPT_DIR"

"$SCRIPT_DIR/bin/ir_to_mips" "$IN_IR" "$OUT_S" "$MODE_FLAG"
echo "Wrote: $OUT_S"

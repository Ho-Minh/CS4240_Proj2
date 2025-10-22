#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
make -C "$SCRIPT_DIR"
echo "Build complete: $SCRIPT_DIR/bin/ir_to_mips"
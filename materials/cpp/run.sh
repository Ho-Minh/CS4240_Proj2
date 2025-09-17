#!/usr/bin/env bash
set -euo pipefail

# Runner for the C++ optimizer built with Make
# Usage: ./run.sh <input_ir_file> <output_ir_file>
# Example: ./run.sh ../public_test_cases/quicksort/quicksort.ir optimized_quicksort.ir

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <input_ir_file> <output_ir_file>"
  echo "Example: $0 ../public_test_cases/quicksort/quicksort.ir optimized_quicksort.ir"
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
BUILD_DIR="$SCRIPT_DIR/bin"

INPUT_IR="$1"
OUTPUT_IR="$2"

# Build if executable missing
EXECUTABLE="$BUILD_DIR/ir_optimizer"
if [[ ! -x "$EXECUTABLE" ]]; then
  echo "Building project with make..."
  make -C "$SCRIPT_DIR"
  if [[ ! -x "$EXECUTABLE" ]]; then
    echo "Cannot find built executable 'ir_optimizer' in $BUILD_DIR"
    exit 1
  fi
fi

"$EXECUTABLE" "$INPUT_IR" "$OUTPUT_IR"
echo "Optimized IR written to: $OUTPUT_IR"

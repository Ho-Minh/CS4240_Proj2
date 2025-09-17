#!/usr/bin/env bash
set -euo pipefail

# Runner for the C++ optimizer from the materials/cpp directory
# Usage: ./run.sh <input_ir_file> <output_ir_file>
# Example: ./run.sh ../public_test_cases/quicksort/quicksort.ir optimized_quicksort.ir

if [[ $# -lt 2 ]]; then
  echo "Usage: $0 <input_ir_file> <output_ir_file>"
  echo "Example: $0 ../public_test_cases/quicksort/quicksort.ir optimized_quicksort.ir"
  exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

INPUT_IR="$1"
OUTPUT_IR="$2"

# Build if the executable is missing
if [[ ! -d "$BUILD_DIR" ]]; then
  "$SCRIPT_DIR/build.sh"
fi

# Resolve executable across single-config and multi-config generators
EXECUTABLE="$BUILD_DIR/ir_optimizer"
if [[ ! -x "$EXECUTABLE" ]]; then
  if [[ -x "$BUILD_DIR/Release/ir_optimizer" ]]; then
    EXECUTABLE="$BUILD_DIR/Release/ir_optimizer"
  elif [[ -x "$BUILD_DIR/Debug/ir_optimizer" ]]; then
    EXECUTABLE="$BUILD_DIR/Debug/ir_optimizer"
  elif [[ -x "$BUILD_DIR/ir_optimizer.exe" ]]; then
    EXECUTABLE="$BUILD_DIR/ir_optimizer.exe"
  elif [[ -x "$BUILD_DIR/Release/ir_optimizer.exe" ]]; then
    EXECUTABLE="$BUILD_DIR/Release/ir_optimizer.exe"
  elif [[ -x "$BUILD_DIR/Debug/ir_optimizer.exe" ]]; then
    EXECUTABLE="$BUILD_DIR/Debug/ir_optimizer.exe"
  else
    # Try building now
    "$SCRIPT_DIR/build.sh"
    if [[ -x "$BUILD_DIR/ir_optimizer" ]]; then
      EXECUTABLE="$BUILD_DIR/ir_optimizer"
    elif [[ -x "$BUILD_DIR/Release/ir_optimizer" ]]; then
      EXECUTABLE="$BUILD_DIR/Release/ir_optimizer"
    elif [[ -x "$BUILD_DIR/Debug/ir_optimizer" ]]; then
      EXECUTABLE="$BUILD_DIR/Debug/ir_optimizer"
    elif [[ -x "$BUILD_DIR/ir_optimizer.exe" ]]; then
      EXECUTABLE="$BUILD_DIR/ir_optimizer.exe"
    elif [[ -x "$BUILD_DIR/Release/ir_optimizer.exe" ]]; then
      EXECUTABLE="$BUILD_DIR/Release/ir_optimizer.exe"
    elif [[ -x "$BUILD_DIR/Debug/ir_optimizer.exe" ]]; then
      EXECUTABLE="$BUILD_DIR/Debug/ir_optimizer.exe"
    else
      echo "Cannot find built executable 'ir_optimizer' in $BUILD_DIR"
      exit 1
    fi
  fi
fi

"$EXECUTABLE" "$INPUT_IR" "$OUTPUT_IR"
echo "Optimized IR written to: $OUTPUT_IR"



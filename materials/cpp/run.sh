#!/usr/bin/env bash
set -euo pipefail

# Runner script for IR library
# Note: The ir_optimizer executable has been removed from this project
# This script now only builds the IR library

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)"

echo "Building IR library with make..."
make -C "$SCRIPT_DIR"

echo "IR library build completed."
echo "Note: The ir_optimizer executable has been removed from this project."
echo "The project now only provides the IR library (libircpp.a)."

#!/usr/bin/env bash
set -euo pipefail

here="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
proj_root="$(cd "$here/.." && pwd)"
materials_root="$(cd "$proj_root/.." && pwd)"

echo "========================================"
echo "Control Flow Graph Testing Suite"
echo "========================================"

echo
echo "1. Building CFG library..."

if [[ -f "$proj_root/Makefile" || -f "$proj_root/makefile" || -f "$proj_root/GNUmakefile" ]]; then
  make -C "$proj_root"
  build_dir="$proj_root/build"
elif [[ -f "$proj_root/CMakeLists.txt" ]]; then
  cmake -S "$proj_root" -B "$proj_root/build" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$proj_root/build" -j
  build_dir="$proj_root/build"
else
  echo "No Makefile or CMakeLists.txt in $proj_root"
  echo "Provide a build system or adjust this script to compile sources directly."
  exit 2
fi

cd "$build_dir"

lib="./libircpp.a"
inc="$proj_root/include"

if [[ ! -f "$lib" ]]; then
  echo "Expected $lib not found. Adjust path or build rules."
  exit 3
fi
if [[ ! -d "$inc" ]]; then
  echo "Include dir $inc not found."
  exit 4
fi

echo
echo "2. Testing CFG Construction..."
echo "g++ -std=c++17 -I$inc $here/test_cfg.cpp $lib -o test_cfg"
g++ -std=c++17 -I"$inc" "$here/test_cfg.cpp" "$lib" -o test_cfg
./test_cfg

echo
echo "3. Testing CFG Demo with Example IR..."
echo "g++ -std=c++17 -I$inc $proj_root/examples/cfg_demo.cpp $lib -o cfg_demo"
g++ -std=c++17 -I"$inc" "$proj_root/examples/cfg_demo.cpp" "$lib" -o cfg_demo
./cfg_demo "$materials_root/example/example.ir" example_cfg.dot

echo
echo "4. Testing CFG Demo with Square Root IR..."
./cfg_demo "$materials_root/public_test_cases/sqrt/sqrt.ir" sqrt_cfg.dot

echo
echo "5. Testing CFG Demo with Quicksort IR..."
./cfg_demo "$materials_root/public_test_cases/quicksort/quicksort.ir" quicksort_cfg.dot

echo
echo "========================================"
echo "CFG Testing completed successfully!"
echo "========================================"
echo
echo "Generated DOT files:"
echo "- example_cfg.dot"
echo "- sqrt_cfg.dot"
echo "- quicksort_cfg.dot"
echo
echo "To visualize CFGs (requires graphviz 'dot'):"
echo "  dot -Tpng example_cfg.dot -o example_cfg.png"
echo "  dot -Tpng sqrt_cfg.dot -o sqrt_cfg.png"
echo "  dot -Tpng quicksort_cfg.dot -o quicksort_cfg.png"
echo "========================================"

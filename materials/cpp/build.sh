#!/usr/bin/env bash
echo "Building the Middle-End IR"
echo "Generating build directory"
cmake -B build
cmake --build build
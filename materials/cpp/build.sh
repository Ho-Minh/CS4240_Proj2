#!/bin/bash

echo "Building the Middle-End IR"
echo "Current working directory:"
pwd
cd materials/cpp/

echo "Generating build directory"
cmake -B build
cmake --build build


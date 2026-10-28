#!/usr/bin/env bash
set -e

cmake -S . -B build -DENABLE_PYTHON=ON
cmake --build build -j"$(sysctl -n hw.ncpu)"

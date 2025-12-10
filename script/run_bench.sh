#!/usr/bin/env bash
set -euo pipefail

rm -rf build

cmake -S . -B build  \
      -D CMAKE_BUILD_TYPE=Release  \
      -D CMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build --parallel

pytest --benchmark-only benchmark/test_bench.py \
       --benchmark-min-rounds=10 \
       --benchmark-json=benchmark.json

python script/plot.py benchmark.json --out-dir docs/figs
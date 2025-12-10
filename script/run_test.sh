#!/usr/bin/env bash
set -euo pipefail

rm -rf build

cmake -S . -B build  \
      -D CMAKE_BUILD_TYPE=Debug \
      -D CMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build --parallel

ctest --test-dir build --output-on-failure

export HYPOTHESIS_PROFILE=dev
export PYTHONWARNINGS=error
pytest -v -W error tests
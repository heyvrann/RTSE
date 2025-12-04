# RTSE

Build
=====

cmake -S . -B build  \
      -D CMAKE_BUILD_TYPE=Debug  \
      -D CMAKE_CXX_FLAGS="-Wall -Wextra -Wpedantic -Werror"
cmake --build build --parallel

Run tests
=========

**ctest**
ctest --test-dir build --output-on-failure

**Python tests**
pip install -U pytest hypothesis
export HYPOTHESIS_PROFILE=dev
export PYTHONWARNINGS=error
pytest -v -W error tests

Benchmarks
==========

Notice that running baseline should be configured with ''CMAKE_BUILD_TYPE=Release''. 

pip install pytest-benchmark
* store local baseline
pytest --benchmark-only benchmark/test_bench.py \
       --benchmark-min-rounds=10 \
       --benchmark-save=rtse-baseline-YYYYMMDD

* comparison with baseline
pytest --benchmark-only benchmark/test_bench.py \
       --benchmark-compare=rtse-baseline-YYYYMMDD
# RTSE

Build
=====

cmake -S . -B build -D CMAKE_BUILD_TYPE=Release
cmake --build build -j

Run tests
=========

**ctest**
ctest --test-dir build --output-on-failure

**Python tests**
pip install -U pytest hypothesis
HYPOTHESIS_PROFILE=dev pytest -q

Benchmarks
==========

pip install pytest-benchmark
* store local baseline
pytest --benchmark-only benchmark/test_bench.py \
       --benchmark-min-rounds=10 \
       --benchmark-save=rtse-baseline-YYYYMMDD

* comparison with baseline
pytest --benchmark-only benchmark/test_bench.py \
       --benchmark-compare=rtse-baseline-YYYYMMDD
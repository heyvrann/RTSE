# RTSE

RTSE is a dynamic R-tree implementation (insert, erase, update, query_range)
with C++17 core and Python bindings, including full correctness tests,
property-based tests, and performance benchmarks.

Test
=========

**script/run_test.sh**

This script:

- builds the project in Debug mode
- runs C++ tests via ctest
- runs Python tests via python test

Benchmark
==========

**script/run_bench.sh**

This script:

- builds the project in Release mode
- runs the full pytest-benchmark suite
- generates ''benchmark.json''
- produces plots in ''docs/figs''

**Note: both scripts remove the build/ directory before building.**

Documentation
=============

Full documentation: docs/build/html/index.html
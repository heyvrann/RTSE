import math, random, platform, os
import pytest
import rtse
from time import perf_counter
from itertools import cycle

COORD_MIN, COORD_MAX = 0.0, 10000.0


def gen_uniform_boxes(n, rng):
    for _ in range(n):
        x1 = rng.uniform(COORD_MIN, COORD_MAX)
        x2 = rng.uniform(COORD_MIN, COORD_MAX)
        y1 = rng.uniform(COORD_MIN, COORD_MAX)
        y2 = rng.uniform(COORD_MIN, COORD_MAX)
        yield rtse.Box2(rtse.Point2(x1, y1), rtse.Point2(x2, y2))


def gen_data_and_queries(N, Q, win_frac, seed=314551132):
    rng = random.Random(seed)
    data = [(box, id) for id, box in enumerate(gen_uniform_boxes(N, rng))]
    queries = [rand_query(win_frac, rng) for _ in range(Q)]
    return data, queries


def build_index(pairs):
    tree = rtse.RTree()
    for box, id in pairs:
        tree.insert(box, id)
    return tree


def linear_scan_ids(data, query_box):
    hits = []
    for box, id in data:
        if query_box.overlap(box):
            hits.append(id)
    return hits


def rand_query(win_frac, rng):
    side = math.sqrt(win_frac) * (COORD_MAX - COORD_MIN)
    cx = rng.uniform(COORD_MIN, COORD_MAX - side)
    cy = rng.uniform(COORD_MIN, COORD_MAX - side)
    return rtse.Box2(rtse.Point2(cx, cy), rtse.Point2(cx + side, cy + side))


@pytest.mark.parametrize("N", [1_000, 10_000, 100_000])
def test_build_time(benchmark, N):
    rng = random.Random(314551132)
    data = [(box, id) for id, box in enumerate(gen_uniform_boxes(N, rng))]

    def _build():
        build_index(data)

    benchmark(_build)


@pytest.mark.parametrize(
    "N, Q, win_frac",
    [
        (10_000, 1_000, 0.01),
        (100_000, 1_000, 0.01),
        (100_000, 1_000, 0.05),
    ],
)
def test_query_latency_and_qps(benchmark, N, Q, win_frac):
    data, queries = gen_data_and_queries(N, Q, win_frac)
    tree = build_index(data)
    it = cycle(queries)

    for _ in range(200):
        _ = tree.query_range(next(it))

    def run_one():
        return len(tree.query_range(next(it)))

    benchmark(run_one)
    st = benchmark.stats.stats

    mean_s = st.mean
    median_s = st.median
    qps = 1.0 / mean_s if mean_s > 0 else float("inf")
    print(
        f"\n[summary] N={N} Q={Q} win={win_frac*100:.0f}% "
        f"mean={mean_s*1e3:.3f} ms  median={median_s*1e3:.3f} ms  QPS≈{qps:.1f}"
    )


@pytest.mark.parametrize(
    "N, Q, win_frac",
    [
        (10_000, 1_000, 0.01),
        (100_000, 1_000, 0.01),
        (100_000, 1_000, 0.05),
    ],
)
def test_linear_baseline_latency_and_qps(benchmark, N, Q, win_frac):
    data, queries = gen_data_and_queries(N, Q, win_frac)
    it = cycle(queries)

    for _ in range(200):
        _ = linear_scan_ids(data, next(it))

    def run_one():
        return len(linear_scan_ids(data, next(it)))

    benchmark(run_one)
    st = benchmark.stats.stats

    mean_s = st.mean
    median_s = st.median
    qps = 1.0 / mean_s if mean_s > 0 else float("inf")
    print(
        f"\n[linear baseline] N={N} Q={Q} win={win_frac*100:.0f}% "
        f"mean={mean_s*1e3:.3f} ms  median={median_s*1e3:.3f} ms  QPS≈{qps:.1f}"
    )


@pytest.mark.parametrize(
    "N_acitive, steps",
    [
        (1_000, 5_000),
        (10_000, 10_000),
        (100_000, 10_000),
    ],
)
def test_mixed_workload_scaling(benchmark, N_acitive, steps):
    rng = random.Random(314551132)
    data = [(box, id) for id, box in enumerate(gen_uniform_boxes(N_acitive, rng))]
    tree = build_index(data)
    active_ids = list(range(N_acitive))
    max_id = N_acitive - 1

    def do_ops():
        nonlocal max_id
        for _ in range(steps):
            op = rng.random()
            if op < 0.5:
                q = rand_query(0.01, rng)
                _ = tree.query_range(q)
            elif op < 0.75 and active_ids:
                id_ = rng.choice(active_ids)
                new_box = next(gen_uniform_boxes(1, rng))
                tree.update(id_, new_box)
            else:
                if not active_ids or rng.random() < 0.5:
                    max_id += 1
                    new_box = next(gen_uniform_boxes(1, rng))
                    tree.insert(new_box, max_id)
                    active_ids.append(max_id)
                else:
                    id_ = rng.choice(active_ids)
                    tree.erase(id_)
                    active_ids.remove(id_)

    do_ops()

    def run_batch():
        do_ops()

    benchmark(run_batch)
    st = benchmark.stats.stats

    mean_s = st.mean
    median_s = st.median
    ops = steps / mean_s if mean_s > 0 else float("inf")
    print(
        f"\n[mixed] N={N_acitive} steps={steps} "
        f"mean={mean_s*1e3:.3f} ms  median={median_s*1e3:.3f} ms  OPS≈{ops:.1f}"
    )

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


def rand_query(win_frac, rng):
    side = math.sqrt(win_frac) * (COORD_MAX - COORD_MIN)
    cx = rng.uniform(COORD_MIN, COORD_MAX - side)
    cy = rng.uniform(COORD_MIN, COORD_MAX - side)
    return rtse.Box2(rtse.Point2(cx, cy), rtse.Point2(cx + side, cy + side))

@pytest.mark.ci
@pytest.mark.parametrize("N", [200, 500])
def test_build_time(benchmark, N):
    rng = random.Random(314551132)
    data = [(box, id) for id, box in enumerate(gen_uniform_boxes(N, rng))]

    def _build():
        build_index(data)

    benchmark(_build)

@pytest.mark.ci
@pytest.mark.parametrize(
    "N, Q, win_frac",
    [
        (1_000, 10, 0.01),
        (2_000, 10, 0.05),
    ],
)
def test_query_latency_and_qps(benchmark, N, Q, win_frac):
    data, queries = gen_data_and_queries(N, Q, win_frac)
    tree = build_index(data)
    it = cycle(queries)

    for _ in range(10):
        _ = tree.query_range(next(it))

    def run_one():
        return len(tree.query_range(next(it)))

    benchmark(run_one)

@pytest.mark.ci
@pytest.mark.parametrize(
    "N_acitive, steps",
    [
        (500, 200),
        (800, 200),
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
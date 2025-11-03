import pytest

def test_rtse_import():
    import rtse
    assert hasattr(rtse, "RTree")
    assert hasattr(rtse, "Box2")

def test_basic_roundtrip():
    import rtse
    tree = rtse.RTree()
    box1 = rtse.Box2(rtse.Point2(0, 0), rtse.Point2(1, 1))
    box2 = rtse.Box2(rtse.Point2(10, 10), rtse.Point2(11, 11))

    tree.insert(box1, 7)
    ids = set(tree.query_range(rtse.Box2(rtse.Point2(0, 0), rtse.Point2(2, 2))))
    assert 7 in ids

    tree.update(7, box2)
    ids2 = set(tree.query_range(rtse.Box2(rtse.Point2(0, 0), rtse.Point2(2, 2))))
    assert 7 not in ids2

    tree.erase(7)
    ids3 = set(tree.query_range(rtse.Box2(rtse.Point2(9, 9), rtse.Point2(12, 12))))
    assert ids3 == set()

def test_random_vs_oracle_small():
    import rtse
    import random
    random.seed(314551132)

    tree = rtse.RTree()
    oracle = {}

    for i in range(20):
        box = rtse.Box2(rtse.Point2(random.random() * 10, random.random() * 10),
                        rtse.Point2(random.random() * 10, random.random() * 10))
        oracle[i] = box
        tree.insert(box, i)

    for _ in range(50):
        rand_query = rtse.Box2(rtse.Point2(random.random() * 12, random.random() * 12),
                               rtse.Point2(random.random() * 12, random.random() * 12))
        oracle_ids = {i for i, b in oracle.items() if rand_query.overlap(b)}
        tree_idx = set(tree.query_range(rand_query))
        assert oracle_ids == tree_idx
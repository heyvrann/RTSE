import pytest
import rtse
from hypothesis import given, strategies as st, settings

finite_float = st.floats(
    min_value=-1e6, max_value=1e6, allow_nan=False, allow_infinity=False, width=64
)


@st.composite
def box2s(draw):
    x1 = draw(finite_float)
    y1 = draw(finite_float)
    x2 = draw(finite_float)
    y2 = draw(finite_float)
    return rtse.Box2(rtse.Point2(x1, y1), rtse.Point2(x2, y2))


@st.composite
def pairs_unique_ids(draw, min_size=0, max_size=60):
    ids = st.integers(min_value=0, max_value=10**9)
    return draw(
        st.lists(
            st.tuples(box2s(), ids),
            min_size=min_size,
            max_size=max_size,
            unique_by=lambda t: t[1],
        )
    )


@st.composite
def nested_query_boxes(draw):
    xlo = draw(finite_float)
    xhi = draw(finite_float)
    ylo = draw(finite_float)
    yhi = draw(finite_float)
    xmin, xmax = min(xlo, xhi), max(xlo, xhi)
    ymin, ymax = min(ylo, yhi), max(ylo, yhi)
    x1 = draw(
        st.floats(min_value=xmin, max_value=xmax, allow_nan=False, allow_infinity=False)
    )
    x2 = draw(
        st.floats(min_value=xmin, max_value=xmax, allow_nan=False, allow_infinity=False)
    )
    y1 = draw(
        st.floats(min_value=ymin, max_value=ymax, allow_nan=False, allow_infinity=False)
    )
    y2 = draw(
        st.floats(min_value=ymin, max_value=ymax, allow_nan=False, allow_infinity=False)
    )
    return (
        rtse.Box2(rtse.Point2(x1, y1), rtse.Point2(x2, y2)),
        rtse.Box2(rtse.Point2(xmin, ymin), rtse.Point2(xmax, ymax)),
    )


@given(pairs_unique_ids(), box2s())
@settings(deadline=None)
def test_prop_query_matches_bruteforce(pairs, query_box):
    tree = rtse.RTree()
    for box, id in pairs:
        tree.insert(box, id)
    oracle = {id for (box, id) in pairs if query_box.overlap(box)}
    assert set(tree.query_range(query_box)) == oracle


@given(box2s(), box2s())
@settings(deadline=None)
def test_prop_update_moves_id(box1, box2):
    tree = rtse.RTree()
    tree.insert(box1, 42)
    assert 42 in set(tree.query_range(box1))
    tree.update(42, box2)
    assert 42 in set(tree.query_range(box2))
    if not box2.overlap(box1):
        assert 42 not in set(tree.query_range(box1))


@given(box2s())
@settings(deadline=None)
def test_prop_erase_removes_everywhere(box):
    tree = rtse.RTree()
    tree.insert(box, 99)
    assert 99 in set(tree.query_range(box))
    tree.erase(99)
    assert 99 not in set(tree.query_range(box))


@given(pairs_unique_ids(), box2s())
@settings(deadline=None)
def test_prop_insertion_order_invariance(pairs, query_box):
    tree1, tree2 = rtse.RTree(), rtse.RTree()
    for box, id in pairs:
        tree1.insert(box, id)
    for box, id in reversed(pairs):
        tree2.insert(box, id)
    assert set(tree1.query_range(query_box)) == set(tree2.query_range(query_box))


@given(st.lists(box2s(), min_size=0, max_size=25), nested_query_boxes())
@settings(deadline=None)
def test_prop_monotonically_nested_queries(boxes, query_pairs):
    query_small, query_large = query_pairs
    tree = rtse.RTree()
    for id, box in enumerate(boxes):
        tree.insert(box, id)
    assert set(tree.query_range(query_small)).issubset(
        set(tree.query_range(query_large))
    )

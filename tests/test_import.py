def test_rtse_import():
    import sys
    sys.path.append("build")
    import rtse
    r = rtse.RTree()
    b = rtse.Box2()
    r.insert(b, 1)
    r.query_range(b)
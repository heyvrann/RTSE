API Description
===============

**Basic geometric**
   
1. ``Point2``: :math:`(x, y)`.
2. ``Box2``: :math:`(x_{min}, y_{min}, x_{max}, y_{max})`.

**API**

1. ``insert``: add ``(geometry, id)`` to the index and ``id`` should be unique.
2. ``erase``: remove by ``id``.
3. ``update``: replace geometry for an existing ``id``.
4. ``query_range``: axis-aligned window search 
then returns matching ``id`` (order not guaranteed).
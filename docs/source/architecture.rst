System Architecture
===================

**Overview**

* C++ R-tree spaial index with nodes storing **MBR**; mirrored Python bindings expose the same API.
* 2D ``point/segment/box``.

**Inputs / Outputs**

* Input: ``(Geometry, id)`` pairs from CSV/WKT or in-memory streams.
* Output: list of **ID**; the host app materializes objects by ID.

**Constraints & Assumptions**

* Axis-aligned window queries only.
* Minimal returns (IDs, optional distances) 
so applications control materialization and downstream work.
* Future extensions possible: polygons, orientaed boxes, concurrent quries

**Modularization**

* ``core``: geometry and R-tree wrapper.
* ``query engine``: range strategies, pruning, priority queues.
* ``I/O layer``: CSV/WKT/in-memory ingestion; optional serialization.
* ``binding``: pybind11 layer exposing the API to Python.
* ``tests``: correctness vs. brute force and latency measurement.
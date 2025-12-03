Problem to Solve
================

The project addresses the need for scalable, 
low-latency spatial search over large 2D geometric datasets. 
As the data grow and queries become frequent, 
a linear scan is no longer viable for domains 
such as robotic collision and game or simulation workloads. 
We therefore focus on a spatial index that maintains stable performance 
under mixed streams of updates and queries.

Our approach employs an R-tree whose nodes store minimum bounding rectangles, 
enabling sub-linear range searches by pruning irrelevant branches 
of the hierarchy. 
The indexed entities include points, segments, and boxes, 
each bound to an integer identifier for application-level bookkeeping. 
Because real-world datasets are dynamic, the index must support online insert, 
remove, and move operations. 
For large initial datasets, bulk loading is used to shorten build time 
and improve query quality. 
The end goal is to deliver reproducible spatial queries through 
a clear C++/Python API that integrates easily 
into the target application domains.

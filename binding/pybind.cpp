#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "../core/rtree.cpp"

namespace py = pybind11;

PYBIND11_MODULE(rtse, m) {
    m.doc() = "R-Tree Search Engine core bindings";
    
    py::class_<rtse::Point2>(m, "Point2")
        .def(py::init<double, double>())
        .def_readwrite("x", &rtse::Point2::x)
        .def_readwrite("y", &rtse::Point2::y);

    py::class_<rtse::Box2>(m, "Box2")
        .def(py::init<>())
        .def_readwrite("min", &rtse::Box2::min)
        .def_readwrite("max", &rtse::Box2::max);

    py::class_<rtse::RTree>(m, "RTree")
        .def(py::init<>())
        .def("insert", &rtse::RTree::insert)
        .def("erase", &rtse::RTree::erase)
        .def("update", &rtse::RTree::update)
        .def("query_range", &rtse::RTree::query_range);
}
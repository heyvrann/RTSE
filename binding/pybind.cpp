#include <pybind11/pybind11.h>
#include <pybind11/operators.h>
#include <pybind11/stl.h>
#include "../core/rtree.h"
#include <string>

namespace py = pybind11;

PYBIND11_MODULE(rtse, m) {
    m.doc() = "R-Tree Search Engine core bindings";
    
    py::class_<rtse::Point2>(m, "Point2", "2D point (x, y).")
        .def(py::init<>())
        .def(py::init<double, double>(), py::arg("x"), py::arg("y"))
        .def_property_readonly("x", &rtse::Point2::x)
        .def_property_readonly("y", &rtse::Point2::y)
        .def("__repr__", [](const rtse::Point2& p) {
            return "Point2(x=" + std::to_string(p.x()) + ", y=" + std::to_string(p.y()) + ")";
        });

    py::class_<rtse::Box2>(m, "Box2", "Axis-aligned bounding box [min, max].")
        .def(py::init<>())
        .def(py::init<const rtse::Point2&, const rtse::Point2&>(), py::arg("pmin"), py::arg("pmax"))
        .def_property_readonly("min", &rtse::Box2::min, py::return_value_policy::reference_internal)
        .def_property_readonly("max", &rtse::Box2::max, py::return_value_policy::reference_internal)
        .def_property_readonly("is_empty", &rtse::Box2::is_empty)
        .def_static("from_point", &rtse::Box2::from_point, py::arg("p"))
        .def("area", &rtse::Box2::area)
        .def("overlap", &rtse::Box2::overlap, py::arg("other"))
        .def_static("merge", &rtse::Box2::merge, py::arg("box1"), py::arg("box2"))
        .def("enlarge_area", &rtse::Box2::enlarge_area, py::arg("other"))
        .def(py::self == py::self)
        .def(py::self != py::self)
        .def("__repr__", [](const rtse::Box2& box) {
            return "Box2(min=(" + std::to_string(box.min().x()) + ", " + std::to_string(box.min().y()) +
                   "), max=(" + std::to_string(box.max().x()) + ", " + std::to_string(box.max().y()) + "))"; 
        });

    py::class_<rtse::RTree>(m, "RTree")
        .def(py::init<>())
        .def("insert", &rtse::RTree::insert, py::arg("box"), py::arg("id"))
        .def("erase", &rtse::RTree::erase, py::arg("id"))
        .def("update", &rtse::RTree::update, py::arg("id"), py::arg("new_box"))
        .def("query_range", &rtse::RTree::query_range, py::arg("query_box"));
}
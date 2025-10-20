#include <pybind11/pybind11.h>

void hello_core();

namespace py = pybind11;

PYBIND11_MODULE(rtse, m) {
    m.doc() = "R-Tree Search Engine core bindings";
    m.def("hello_core", &hello_core, "Print hello message from C++ core");
}
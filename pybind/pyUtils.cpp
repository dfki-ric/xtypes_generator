#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>

#include "utils.hpp"

namespace py = pybind11;
namespace nl = nlohmann;

using namespace xtypes;

PYBIND11_EXPORT
void PYBIND11_INIT_XTYPES_GENERATOR__UTILS(py::module_& m) {
    m.def("parseJson", &xtypes::parseJson,
                py::arg("string"),
                py::arg("info")="")
     .def("uri_to_uuid", &xtypes::uri_to_uuid,
                py::arg("uri")
                );
}

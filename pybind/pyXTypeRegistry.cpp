#include <pybind11/pybind11.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
#include <pybind11/stl.h>
#include <pybind11/functional.h>

#include "XType.hpp"
#include "XTypeRegistry.hpp"

namespace py = pybind11;
namespace nl = nlohmann;
using namespace xtypes;

PYBIND11_EXPORT
void PYBIND11_INIT_REGISTRY(py::module_& m) {
    // NOTE: The 3rd argument is a different default holder. Default is std::unique_ptr but we need std::shared_ptr.
    py::class_<XTypeRegistry, std::shared_ptr<XTypeRegistry> >(m, "XTypeRegistry")
        .def(py::init())
        .def("register_alias", &XTypeRegistry::register_alias)
        .def("knows_class", &XTypeRegistry::knows_class)
        .def("instantiate_from", &XTypeRegistry::instantiate_from)
        .def("import_from", &XTypeRegistry::import_from);
}

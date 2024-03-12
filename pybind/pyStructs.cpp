#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <nlohmann/json.hpp>
#include <pybind11_json/pybind11_json.hpp>
//#include <pybind11/operators.h>

#include "structs.hpp"
#include "XType.hpp"

namespace py = pybind11;
namespace nl = nlohmann;

using namespace xtypes;

// NOTE: std::weak ptr is not supported by default in pybind11 and requires custom implementation as shown below...
PYBIND11_DECLARE_HOLDER_TYPE(T, std::weak_ptr<T>);
// Only needed if the type's `.get()` goes by another name
namespace pybind11 { namespace detail {
    template <typename T>
    struct holder_helper<std::weak_ptr<T>> { // <-- specialization
        static const T *get(const std::weak_ptr<T> &p) { return p.lock().get(); }
    };
}}

PYBIND11_EXPORT
void PYBIND11_INIT_XTYPES_GENERATOR__STRUCTS(py::module_& m) {
    py::class_<PropertySchema>(m, "PropertySchema")
        .def(py::init())
        .def_readwrite("property_types", &PropertySchema::property_types)
        .def_readwrite("allowed_values", &PropertySchema::allowed_values)
        .def_readwrite("default_values", &PropertySchema::default_values)
        .def("define_property", &PropertySchema::define_property)
        .def("has_property", &PropertySchema::has_property)
        .def("get_property_type", &PropertySchema::get_property_type)
        .def("get_allowed_property_values", &PropertySchema::get_allowed_property_values)
        .def("is_allowed_value", &PropertySchema::is_allowed_value)
        .def("is_type_matching", &PropertySchema::is_type_matching)
        .def("to_json", &PropertySchema::to_json);

    py::class_<Relation>(m, "Relation")
        .def(py::init())
        .def("to_json", &Relation::to_json)
        .def_readwrite("relation_type", &Relation::relation_type)
        .def_readwrite("subrelation_of", &Relation::subrelation_of)
        .def_readwrite("from_classnames", &Relation::from_classnames)
        .def_readwrite("to_classnames", &Relation::to_classnames)
        .def_readwrite("constraint", &Relation::constraint)
        .def_readwrite("delete_policy", &Relation::delete_policy)
        .def_readwrite("property_schema", &Relation::property_schema)
        .def("__getitem__", &Relation::operator[])
        .def("__eq__", &Relation::operator==)
        .def("__ne__", &Relation::operator!=);

    py::class_<Fact>(m, "Fact")
        .def(py::init<std::weak_ptr< XType >, nl::json>(),
             py::arg("target"),
             py::arg("edge_properties"))
        .def_readwrite("target", &Fact::target)
        .def_readwrite("edge_properties", &Fact::edge_properties)
        .def("__getitem__", [](const Fact& fact, std::string key) {
            if (key == "target" )
                return py::object(py::cast(fact.target));
            if (key == "edge_properties")
                return py::object(fact.edge_properties);
            throw pybind11::key_error(key);
        })
        .def("__getitem__", [](const Fact& fact, int key) {
            if (key == 0)
                return py::object(py::cast(fact.target));
            if (key == 1)
                return py::object(fact.edge_properties);
            throw pybind11::index_error(std::to_string(key));
        })
        .def("__eq__", &Fact::operator==)
        .def("__ne__", &Fact::operator!=);
}

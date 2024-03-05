#include <pybind11/pybind11.h>
#include <pybind11_json/pybind11_json.hpp>

#include "enums.hpp"
#include "XType.hpp"

namespace py = pybind11;

using namespace xtypes;

PYBIND11_EXPORT
void PYBIND11_INIT_XTYPES_GENERATOR__ENUMS(py::module_& m) {
    py::enum_<Constraint>(m, "Constraint")
        .value("MANY2MANY", Constraint::MANY2MANY)
        .value("ONE2MANY", Constraint::ONE2MANY)
        .value("MANY2ONE", Constraint::MANY2ONE)
        .value("ONE2ONE", Constraint::ONE2ONE);
    py::enum_<DeletePolicy>(m, "DeletePolicy")
        .value("DELETENONE", DeletePolicy::DELETENONE)
        .value("DELETESOURCE", DeletePolicy::DELETESOURCE)
        .value("DELETETARGET", DeletePolicy::DELETETARGET)
        .value("DELETEBOTH", DeletePolicy::DELETEBOTH);
    py::enum_<RelationType>(m, "RelationType")
        //.value("NONE", RelationType::NONE)
        .value("HAS", RelationType::HAS)
        .value("PART_OF_COMPOSITION", RelationType::PART_OF_COMPOSITION)
        .value("SUBCLASS_OF", RelationType::SUBCLASS_OF)
        .value("INSTANCE_OF", RelationType::INSTANCE_OF)
        .value("CONNECTED_TO", RelationType::CONNECTED_TO)
        .value("ALIAS_OF", RelationType::ALIAS_OF)
        .value("NEEDS", RelationType::NEEDS)
        .value("PROVIDES", RelationType::PROVIDES)
        .value("CONTAINED_BY", RelationType::CONTAINED_BY)
        .value("EXISTS_IN", RelationType::EXISTS_IN)
        .value("GENERATED", RelationType::GENERATED)
        .value("DEPENDS_ON", RelationType::DEPENDS_ON)
        .value("CONSTRAINED_BY", RelationType::CONSTRAINED_BY)
        .value("INTERFACE_TO", RelationType::INTERFACE_TO)
        .value("SPANS", RelationType::SPANS)
        .value("HAS_UNIQUE", RelationType::HAS_UNIQUE)
        .value("ANNOTATES", RelationType::ANNOTATES)
        .value("ATTACHED_TO", RelationType::ATTACHED_TO)
        .value("CAN_SAMPLE", RelationType::CAN_SAMPLE)
        .value("CONDITIONABLE_ON", RelationType::CONDITIONABLE_ON);
    py::enum_<nlohmann::json::value_t>(m, "JsonValueType")
        .value("NULL", nlohmann::json::value_t::null)
        .value("BOOLEAN", nlohmann::json::value_t::boolean)
        .value("STRING", nlohmann::json::value_t::string)
        .value("INT", nlohmann::json::value_t::number_integer)
        .value("UNSIGNED", nlohmann::json::value_t::number_unsigned)
        .value("FLOAT", nlohmann::json::value_t::number_float)
        .value("OBJECT", nlohmann::json::value_t::object)
        .value("ARRAY", nlohmann::json::value_t::array)
        .value("BINARY", nlohmann::json::value_t::binary)
        .value("DISCARDED", nlohmann::json::value_t::discarded);
}

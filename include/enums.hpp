// In here all enumeratable types inherited from DROCK are stored
//These will mostly be used by
//ComponentModel, Component, InterfaceModel and Interface class
#pragma once
#include <nlohmann/json.hpp>

namespace xtypes {
    enum class Constraint
    {
        MANY2MANY,
        ONE2MANY,
        MANY2ONE,
        ONE2ONE
    };
    static const char* Constraint2Str[] = {
        "MANY2MANY",
        "ONE2MANY",
        "MANY2ONE",
        "ONE2ONE"
    };

    enum class DeletePolicy
    {
        DELETENONE = 0,
        DELETESOURCE = 1,
        DELETETARGET = 2,
        DELETEBOTH = 3
    };
    static const char* DeletePolicy2Str[] = {
        "DELETENONE",
        "DELETESOURCE",
        "DELETETARGET",
        "DELETEBOTH"
    };

    enum class RelationType
    {
        NONE = -1,
        HAS = 0,
        PART_OF_COMPOSITION = 1,
        SUBCLASS_OF = 2,
        INSTANCE_OF = 3,
        CONNECTED_TO = 4,
        ALIAS_OF = 5,
        NEEDS = 6,
        PROVIDES = 7,
        CONTAINED_BY = 8,
        EXISTS_IN = 9,
        GENERATED = 10,
        DEPENDS_ON = 11,
        CONSTRAINED_BY = 12,
        INTERFACE_TO = 13,
        SPANS = 14,
        HAS_UNIQUE = 15,
        ANNOTATES = 16,
        ATTACHED_TO = 17,
        CAN_SAMPLE = 18,
        CONDITIONABLE_ON = 19,
        IMPLEMENTS = 20,
        CONFIGURED_FOR = 21
    };
    static const char* RelationType2Str[] = {
        "HAS",
        "PART_OF_COMPOSITION",
        "SUBCLASS_OF",
        "INSTANCE_OF",
        "CONNECTED_TO",
        "ALIAS_OF",
        "NEEDS",
        "PROVIDES",
        "CONTAINED_BY",
        "EXISTS_IN",
        "GENERATED",
        "DEPENDS_ON",
        "CONSTRAINED_BY",
        "INTERFACE_TO",
        "SPANS",
        "HAS_UNIQUE",
        "ANNOTATES",
        "ATTACHED_TO",
        "CAN_SAMPLE",
        "CONDITIONABLE_ON",
        "IMPLEMENTS",
        "CONFIGURED_FOR"
    };

    static const std::map<nlohmann::json::value_t, std::string> value_t2string = {
      {nlohmann::json::value_t::null, "null"},
      {nlohmann::json::value_t::boolean, "boolean"},
      {nlohmann::json::value_t::string, "string"},
      {nlohmann::json::value_t::number_integer, "int"},
      {nlohmann::json::value_t::number_unsigned, "unsigned int"},
      {nlohmann::json::value_t::number_float, "float"},
      {nlohmann::json::value_t::object, "object"},
      {nlohmann::json::value_t::array, "array"},
      {nlohmann::json::value_t::binary, "binary"},
      {nlohmann::json::value_t::discarded, "discarded"}
    };
}

#pragma once

#include <nlohmann/json.hpp>

#include "enums.hpp"
#include <set>
#include <algorithm>
#include <iostream>

namespace nl = nlohmann;

namespace xtypes {
    /// Holds information about property schemata
    struct PropertySchema
    {
        nl::json property_types;
        nl::json allowed_values;
        nl::json default_values;

        nl::json to_json() const {
            nl::json out;
            out["property_types"] = property_types;
            out["allowed_values"] = allowed_values;
            out["default_values"] = default_values;
            return out;
        }

        static nl::json::json_pointer to_pointer(const std::string& path_to_key)
        {
            nl::json::json_pointer jptr(path_to_key.front() == '/' ? path_to_key : "/"+path_to_key);
            return jptr;
        }

        void define_property(const std::string& path_to_key,
                     const nl::json::value_t& type = nl::json::value_t::discarded,
                     const std::set<nl::json>& allowed_values = {},
                     const nl::json& default_value = {},
                     const bool& override = false)
        {
            if (this->has_property(path_to_key) && !override)
            {
                throw std::invalid_argument("PropertySchema::define_property(): Property " + path_to_key + " already defined!");
            }
            nl::json::json_pointer jptr(to_pointer(path_to_key));
            this->property_types[jptr] = type;
            this->allowed_values[jptr] = allowed_values;
            if (!default_value.is_null())
            {
                // Check if type matches
                if (!this->is_type_matching(path_to_key, default_value))
                {
                    throw std::invalid_argument("PropertySchema::define_property(): Invalid default value " + default_value.dump() + " for " + path_to_key);
                }
            }
            this->default_values[jptr] = default_value;
        }

        bool has_property(const std::string& path_to_key) const
        {
            nl::json::json_pointer jptr(to_pointer(path_to_key));
            return this->property_types.contains(jptr);
        }

        nl::json::value_t get_property_type(const std::string& path_to_key) const
        {
            nl::json::json_pointer jptr(to_pointer(path_to_key));
            return this->property_types.at(jptr);
        }

        std::set<nl::json> get_allowed_property_values(const std::string& path_to_key) const
        {
            nl::json::json_pointer jptr(to_pointer(path_to_key));
            return this->allowed_values.at(jptr);
        }

        bool is_allowed_value(const std::string& path_to_key, const nl::json& value) const
        {
            auto allowed_values = this->get_allowed_property_values(path_to_key);
            if (allowed_values.size() < 1)
            {
                // No constraint set, so allow it
                return true;
            }
            if (allowed_values.count(value) > 0)
            {
                // value is explicitly allowed
                return true;
            }
            // The value is not allowed
            return false;
        }

        bool is_type_matching(const std::string& path_to_key, const nl::json &value) const
        {
            // Types match directly
            if (this->get_property_type(path_to_key) == value.type())
                return true;
            // Sometimes an unsigned/signed integer shall be assigned to an signed/unsigned value
            // NOTE: This can happen if nlohmann::json interprets an signed value as being an unsigned value
            if (this->get_property_type(path_to_key) == nl::json::value_t::number_integer && value.type() == nl::json::value_t::number_unsigned)
                return true; // TODO: Check that unsigned value < signed positive max!
            if (this->get_property_type(path_to_key) == nl::json::value_t::number_unsigned && value.type() == nl::json::value_t::number_integer)
                return value >= 0U;
            // The initial type is discarded, so we match anything
            if (this->get_property_type(path_to_key) == nl::json::value_t::discarded)
                return true;
            // For dictionaries we have to handle the empty dict initialization case
            if (this->get_property_type(path_to_key) == nl::json::value_t::object && value.type() == nl::json::value_t::null)
                return true;
            return false;
        }
    };

    /// Holds the informations about a Relation
    struct Relation
    {
        RelationType relation_type;
        RelationType subrelation_of;
        std::set<std::string> from_classnames;
        std::set<std::string> to_classnames;
        Constraint constraint;
        DeletePolicy delete_policy;
        PropertySchema property_schema;

        bool operator!=(const Relation& other) const
        {
            return !(*(this) == other);
        }

        bool operator==(const Relation& other) const
        {
            bool from_in_other = std::includes(from_classnames.begin(), from_classnames.end(), other.from_classnames.begin(), other.from_classnames.end());
            bool other_in_from = std::includes(other.from_classnames.begin(), other.from_classnames.end(), from_classnames.begin(), from_classnames.end());
            bool to_in_other = std::includes(to_classnames.begin(), to_classnames.end(), other.to_classnames.begin(), other.to_classnames.end());
            bool other_in_to = std::includes(other.to_classnames.begin(), other.to_classnames.end(), to_classnames.begin(), to_classnames.end());
            return relation_type == other.relation_type &&
                   subrelation_of == other.subrelation_of &&
                   // NOTE: Relations are considered to be equal to another if the domain and codomain are subsets of the other domains or vice versa
                   (from_in_other || other_in_from) &&
                   (to_in_other || other_in_to) &&
                   constraint == other.constraint &&
                   delete_policy == other.delete_policy;
        }

        nl::json operator[](std::string key) const {
            return this->to_json()[key];
        }

        nl::json to_json() const {
            nl::json out;
            out["relation_type"] = relation_type;
            out["subrelation_of"] = subrelation_of;
            out["from_classnames"] = from_classnames;
            out["to_classnames"] = to_classnames;
            out["constraint"] = Constraint2Str[(int)constraint];
            out["delete_policy"] = DeletePolicy2Str[(int)delete_policy];
            out["property_schema"] = property_schema.to_json();
            return out;
        }
    };

    class XType;

    /// A fact is referring to an target Xtype
    /// target == nullptr is an empty fact
    /// target != nullptr is a valid fact
    struct Fact {
        std::weak_ptr< XType > target;
        nl::json edge_properties;

        Fact(std::weak_ptr<XType> target, const nl::json& edge_properties);

        bool operator!=(const Fact& other) const;
        bool operator==(const Fact& other) const;
    };

    /// An extended fact is referring to an target Xtype
    /// The target is either specified by an URI or by an weak pointer
    /// target_uri == empty and target == nullptr is an empty fact
    /// target_uri != empty and target == nullptr means that the fact has to be resolved (e.g. by the registry)
    /// target_uri == empty and target != nullptr means that the fact is pending and the target might not yet been fully valid (e.g. is_uri_valid() is false)
    /// target_uri != empty and target != nullptr means that the fact has been resolved and target_uri matches target.lock()->uri()
    struct ExtendedFact : public Fact {
        ExtendedFact(const std::string& target_uri, const nl::json& edge_properties);
        ExtendedFact(std::weak_ptr<XType> target, const nl::json& edge_properties);

        bool operator!=(const ExtendedFact& other) const;
        bool operator==(const ExtendedFact& other) const;

        // getter and setter for _target_uri
        // the getter always tries target.lock()->uri() before returning _target_uri
        const std::string target_uri() const;
        // the setter will invalidate the target iff
        // uri is not empty and target uri is invalid OR does not match
        void target_uri(const std::string& uri);

        private:
            std::string _target_uri;
    };
}

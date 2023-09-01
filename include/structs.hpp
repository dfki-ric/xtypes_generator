#pragma once

#include <nlohmann/json.hpp>

#include "enums.hpp"
#include <set>
#include <algorithm>
#include <iostream>

namespace nl = nlohmann;

namespace xtypes {
    /// Holds the informations about a Relation
    struct Relation
    {
        RelationType relation_type;
        RelationType subrelation_of;
        std::set<std::string> from_classnames;
        std::set<std::string> to_classnames;
        Constraint constraint;
        DeletePolicy delete_policy;
        nl::json properties;

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
            //for (const auto& e : from_classnames)
            //    std::cout << e << " ";
            //std::cout << "\n";
            //for (const auto& e : other.from_classnames)
            //    std::cout << e << " ";
            //std::cout << "\n";
            //for (const auto& e : to_classnames)
            //    std::cout << e << " ";
            //std::cout << "\n";
            //for (const auto& e : other.to_classnames)
            //    std::cout << e << " ";
            //std::cout << "\n";
            //std::cout << from_in_other << " " << other_in_from << " " << to_in_other << " " << other_in_to << "\n";
            return relation_type == other.relation_type &&
                   subrelation_of == other.subrelation_of &&
                   // NOTE: Relations are considered to be equal to another if the domain and codomain are subsets of the other domains or vice versa
                   (from_in_other || other_in_from) &&
                   (to_in_other || other_in_to) &&
                   constraint == other.constraint &&
                   delete_policy == other.delete_policy &&
                   properties == other.properties;
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
            out["properties"] = properties;
            return out;
        }
    };

    class XType;

    /// Holds a weak pointer of the XType to which this edge points and the generic properties about this connection as json
    struct Fact {
        std::weak_ptr< XType > target;
        nl::json edge_properties;

        Fact(std::weak_ptr<XType> target, const nl::json& edge_properties);

        bool operator!=(const Fact& other) const;
        bool operator==(const Fact& other) const;
    };
    using Edge = Fact;
}

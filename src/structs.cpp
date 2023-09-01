#include "structs.hpp"
#include "XType.hpp"


using namespace xtypes;

xtypes::Fact::Fact(std::weak_ptr<XType> target, const nl::json& edge_properties)
: target{target}, edge_properties(edge_properties)
{}

bool xtypes::Fact::operator!=(const Fact& other) const
{
    return !(*(this) == other);
}

bool xtypes::Fact::operator==(const Fact& other) const
{
    return edge_properties == other.edge_properties &&
           (target.expired() || other.target.expired() || target.lock()->uri() == other.target.lock()->uri());
}

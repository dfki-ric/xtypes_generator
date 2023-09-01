#include "structs.hpp"
#include "XType.hpp"


using namespace xtypes;

Fact::Fact(std::weak_ptr<XType> target, const nl::json& edge_properties)
: target{target}, edge_properties(edge_properties)
{}

bool Fact::operator!=(const Fact& other) const
{
    return !(*(this) == other);
}

bool Fact::operator==(const Fact& other) const
{
    // pointers point to the same object
    if (!other.target.expired() && !this->target.expired() && (this->target.lock() == other.target.lock()))
        return true;
    // target.lock()->uri() == other.target.lock()->uri()
    if (!this->target.expired() && this->target.lock()->is_uri_valid() &&
            !other.target.expired() && other.target.lock()->is_uri_valid() &&
            (this->target.lock()->uri() == other.target.lock()->uri()))
        return true;
    // NOTE: edge_properties should not checked for equality.
    // This is because per relation only ONE fact with the same URI is allowed.
    // Different edge_properties could produce two facts with the same URI in an e.g. std::set which would be wrong.
    return false;
}

ExtendedFact::ExtendedFact(const std::string& target_uri, const nl::json& edge_properties)
: Fact({}, edge_properties), _target_uri{target_uri}
{}

ExtendedFact::ExtendedFact(std::weak_ptr<XType> target, const nl::json& edge_properties)
: Fact(target, edge_properties)
{
    _target_uri = target_uri();
}

const std::string ExtendedFact::target_uri() const
{
    return ((!target.expired() && target.lock()->is_uri_valid()) ? target.lock()->uri() : _target_uri);
}

void ExtendedFact::target_uri(const std::string& uri)
{
    // Never allow destruction of the inner uri
    if (uri.empty())
        return;
    _target_uri = uri;
    // Check if we have to invalidate the weak ptr
    if (!target.expired())
    {
        if (!target.lock()->is_uri_valid() || (target.lock()->uri() != _target_uri))
        {
            // Invalidate the weak reference
            target.reset();
        }
    }
}

bool ExtendedFact::operator!=(const ExtendedFact& other) const
{
    return !(*(this) == other);
}

bool ExtendedFact::operator==(const ExtendedFact& other) const
{
    // Call base class operator first
    if (Fact::operator==(other))
        return true;
    // target_uri() == other.target_uri()
    // NOTE: This also uses up-to-date uris (see target_uri()) so the case (this->_target_uri == other.target.lock()->uri()) and vice versa are automatically handled as well
    if (!this->target_uri().empty() && !other.target_uri().empty() && (this->target_uri() == other.target_uri()))
        return true;
    // NOTE: edge_properties should not checked for equality.
    // This is because per relation only ONE fact with the same URI is allowed.
    // Different edge_properties could produce two facts with the same URI in an e.g. std::set which would be wrong.
    return false;
}

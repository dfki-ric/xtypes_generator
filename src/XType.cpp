#include "XType.hpp"
#include "XTypeRegistry.hpp"
#include <iostream>
#include "utils.hpp"

using namespace xtypes;

// Static identifier
const std::string xtypes::XType::classname = "xtypes::XType";

xtypes::XType::XType(const std::string &classname)
    : m_classname(classname.empty() ? typeid(this).name() : classname)
{
    // NOTE: A pointer to the registry is given when instantiated from the registry OR when we add a valid instance
}

std::string xtypes::XType::get_classname() const noexcept
{
    return m_classname;
}

std::string xtypes::XType::uri() const
{
    return std::string("xtypes://generic");
}

bool xtypes::XType::is_uri_valid() const
{
    try {
        uri();
    } catch (...) {
        return false;
    }
    return true;
}

std::size_t xtypes::XType::uuid() const
{
    return xtypes::uri_to_uuid(uri());
}

/// Sets the registry of the XType if not already set
void XType::set_registry_once(XTypeRegistryCPtr reg)
{
    if (registry.expired())
    {
        registry = reg;
    }
}

/// Overwrite the registry
void XType::overwrite_registry(XTypeRegistryCPtr reg)
{
    registry = reg;
}

/// Returns the registry of this XType if set. Otherwise nullptr
XTypeRegistryCPtr XType::get_registry() const
{
    return registry.lock();
}

// max_depth == 0: Only the current XType properties are exported (partial export)
// max_depth == 1: Current XType properties AND relations are exported, neighbours only partially (without relations)
// max_depth > 1: and so forth
std::map< std::string, nl::json > xtypes::XType::export_to(const int max_depth)
{
    std::map< std::string, nl::json > result;
    XTypeRegistryCPtr reg = this->registry.lock();
    // This queue stores all XTypes which have to be visited at a certain depth
    std::deque<std::pair<unsigned, XTypePtr>> to_visit = {{0U, shared_from_this()}};
    while (to_visit.size() > 0)
    {
        // Get the current xtype to be visited
        auto [depth, xtype] = to_visit.front();
        to_visit.pop_front();
        const std::string xtype_uri = xtype->uri();
        // Check if the current xtype has already been handled
        if (result.count(xtype_uri) > 0)
            continue;
        // Commit that xtype to the registry (if available)
        // We HAVE TO overwrite existing models (because references, properties and/or URI's could have changed)
        if (reg) 
        {
            reg->commit(xtype, true);
        }
        // Place xtype into result set
        result[xtype_uri]["properties"] = xtype->get_properties();
        result[xtype_uri]["uri"] = xtype_uri;
        result[xtype_uri]["uuid"] = std::to_string(xtype->uuid());
        result[xtype_uri]["classname"] = xtype->get_classname();
        result[xtype_uri]["relations"] = nl::json::object();
        // Check if we explore further or not
        if ((max_depth >= 0) && (depth >= max_depth))
            continue;
        // Resolve relations
        const auto &rels(xtype->get_relations());
        for (const auto &[rel_name, rel] : rels)
        {
            // Check if there are facts to export or not
            // NOTE: This can happen if we have partially loaded/defined models
            if (!xtype->has_facts(rel_name))
                continue;
            const bool rel_dir_fwd = xtype->get_relations_dir(rel_name);
            const std::string rel_del_pol = std::string(DeletePolicy2Str[static_cast<int>(rel.delete_policy)]);
            result[xtype_uri]["relations"][rel_name] = nl::json::array();
            const auto &fs(xtype->get_facts(rel_name));
            for (const auto &f : fs)
            {
                nl::json entry;
                const auto &other_xtype = f.target.lock();
                const auto &rel_props = f.edge_properties;
                // HINT: assertion may e.g. fail if the instance is not instantiated from or contained by the ProjectRegistry
                assert(other_xtype.get());
                const std::string other_uri = other_xtype->uri();
                entry["target"] = other_uri;
                entry["edge_properties"] = rel_props;
                entry["delete_policy"] = rel_del_pol;
                entry["relation_dir_forward"] = rel_dir_fwd;
                result[xtype_uri]["relations"][rel_name].push_back(entry);
                // Check if we already visited it
                if (result.count(other_uri) > 0)
                    continue;
                // Register new xtype to be visited
                to_visit.push_back({depth + 1, other_xtype});
            }
        }
    }
    return result;
}

XTypeCPtr xtypes::XType::import_from(const nl::json& spec, XTypeRegistryCPtr reg)
{
    // Check if URI exists
    if (spec.empty())
    {
        // TODO: Should we better throw here?
        std::cerr << "xtypes::Xtype::import_from(): Spec empty. Skipping\n";
        return nullptr;
    }
    if (!spec.contains("uri"))
    {
        // This is a true error, we should throw
        throw std::runtime_error("xtypes::Xtype::import_from(): Missing URI in " + spec.dump());
    }
    if (!spec.contains("classname"))
    {
        // This is a true error, we should throw
        throw std::runtime_error("xtypes::Xtype::import_from(): Missing classname in " + spec.dump());
    }
    // Parse URI
    const std::string &uri(spec["uri"]);
    // Parse classname
    const std::string &classname(spec["classname"]);
    // Check for registry
    if (!reg)
    {
        // This is a true error, we should throw
        throw std::runtime_error("xtypes::Xtype::import_from(): no registry given");
    }
    // Instantiate the correct XType
    if (!reg->knows_class(classname))
    {
        // This is a true error, we should throw
        throw std::runtime_error("xtypes::Xtype::import_from(): registry does not know classname " + classname);
    }
    XTypeCPtr result = reg->instantiate_from(classname);
    if (!result)
    {
        // This is a true error, we should throw
        throw std::runtime_error("xtypes::Xtype::import_from(): registry could not instantiate class " + classname);
    }
    // NOTE: We updated the serialization to group properties and relations, but we want to be downwards compatible to the old version
    if (spec.contains("properties"))
    {
        result->set_properties(spec["properties"]);
    } else {
        std::cerr << "xtypes::import_from(): WARNING: Falling back to old import of properties for " << uri << "\n";
        result->set_properties(spec, false);
    }
    // Resolve dependencies to other XTypes
    // NOTE: We updated the serialization to group properties and relations, but we want to be downwards compatible to the old version
    nl::json relation_spec;
    if (spec.contains("relations"))
    {
        relation_spec = spec["relations"];
    } else {
        std::cerr << "xtypes::import_from(): WARNING: Falling back to old import of relations for " << uri << "\n";
        relation_spec = spec;
    }
    const auto &rels(result->get_relations());
    for (const auto &[rel_name, rel] : rels)
    {
        // A missing entry means that the facts of a certain relation could not be resolved and are UNKNOWN
        if (!relation_spec.contains(rel_name))
            continue;
        // Initialize KNOWN fact entry (could still be EMPTY though)
        result->set_unknown_fact_empty(rel_name);
        for (const auto &entry : relation_spec[rel_name])
        {
            const std::string &other_uri(entry["target"]);
            const nl::json &props(entry["edge_properties"]);
            nl::json updated_props = rel.properties;
            for (auto& [k,v] : updated_props.items())
                if (props.contains(k)) v = props.at(k);
            // NOTE: Here we cannot load the other xtype by URI! Otherwise we would trigger a full load of the whole (sub)graph
            // That means, that we cannot use add_fact() but have to add a raw fact
            ExtendedFact new_fact(other_uri, updated_props);
            // TODO: Check if already existent?
            // TODO: Check for cardinality constraints?
            result->facts[rel_name].push_back(new_fact);
        }
    }
    // Make sure that the resulting xtype gets into _valid_instances of the registry
    if (!result->is_uri_valid())
    {
        throw std::runtime_error("xtypes::XType::import_from(): Could not import a valid xtype from spec. URI is invalid");
    }
    // We now commit() and return a temporary copy with get_by_uri().
    // We overwrite any existing entity with the new info
    reg->commit(result, true);
    return reg->get_by_uri(result->uri());
}

void xtypes::XType::define_property(const std::string& path_to_key,
                     const nl::json::value_t& type,
                     const std::set<nl::json>& allowed_values,
                     const nl::json& default_value,
                     const bool& override)
{
    this->property_schema.define_property(path_to_key, type, allowed_values, default_value, override);
    // Make sure that the key exists in properties (type has already been checked before)
    this->properties[PropertySchema::to_pointer(path_to_key)] = default_value;
}

bool xtypes::XType::has_property(const std::string& path_to_key) const
{
    return this->property_schema.has_property(path_to_key);
}

nl::json::value_t xtypes::XType::get_property_type(const std::string& path_to_key) const
{
    return this->property_schema.get_property_type(path_to_key);
}

std::set<nl::json> xtypes::XType::get_allowed_property_values(const std::string& path_to_key) const
{
    return this->property_schema.get_allowed_property_values(path_to_key);
}

bool xtypes::XType::is_allowed_value(const std::string& path_to_key, const nl::json& value) const
{
    return this->property_schema.is_allowed_value(path_to_key, value);
}

bool xtypes::XType::is_type_matching(const std::string& path_to_key, const nl::json &value) const
{
    return this->property_schema.is_type_matching(path_to_key, value);
}

void xtypes::XType::set_property(const std::string& path_to_key, const nl::json &new_value, const bool shall_throw)
{
    // Check if the property has been defined
    if (!this->has_property(path_to_key))
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Property " + path_to_key + " not found.");
        }
        return;
    }
    // We should not be able to change the type here, so we check it
    if (shall_throw && this->get_property_type(path_to_key) == nl::json::value_t::discarded)
    {
        std::cerr << this->get_classname() + "::set_property: No type defined for property " + path_to_key + ". Type safety isn't assured." << std::endl;
    }
    else if (!is_type_matching(path_to_key, new_value)) // handle empty dict
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Property " + path_to_key + ": Type mismatch. " +
                                        "Expected " + value_t2string.at(this->get_property_type(path_to_key)) + ", but received " + value_t2string.at(new_value.type()));
        }
        return;
    }
    else if (!is_allowed_value(path_to_key, new_value))
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Value " + new_value.dump() + " not allowed for property " + path_to_key);
        }
        return;
    }
    this->properties[PropertySchema::to_pointer(path_to_key)] = new_value;
}

nl::json xtypes::XType::get_property(const std::string& path_to_key) const
{
    if (!this->has_property(path_to_key))
    {
        throw std::invalid_argument(this->get_classname() + "::get_property: Property " + path_to_key + " not found.");
    }
    return this->properties.at(PropertySchema::to_pointer(path_to_key));
}

nl::json xtypes::XType::get_properties() const
{
    return this->properties;
}

void xtypes::XType::set_properties(const nl::json &properties, const bool shall_throw)
{
    nl::json flattened(this->property_schema.property_types.flatten());
    for (const auto &[k,v] : flattened.items())
    {
        nl::json::json_pointer jptr(k);
        if (properties.contains(jptr))
            set_property(k, properties.at(jptr), shall_throw);
    }
}

void xtypes::XType::define_relation(const std::string& name,
                          const RelationType& relation_type,
                          std::set<std::string> from_classnames,
                          std::set<std::string> to_classnames,
                          const Constraint& constraint,
                          const DeletePolicy& delpolicy,
                          const nl::json& properties,
                          const RelationType& super_relation_type,
                          const bool& inverse,
                          const bool& override)
{
    if (inverse)
    {
        std::swap(from_classnames, to_classnames);
    }
    Relation relation = {
        relation_type,
        super_relation_type,
        from_classnames,
        to_classnames,
        constraint,
        delpolicy,
        properties};

    // Bind relation definition to name and store the direction of the facts/relation instances
    // Also initialize empty list in facts
    if (!this->has_relation(name) || override)
    {
        this->relations[name] = relation;
        this->relation_dir_forward[name] = !inverse;
        // NOTE: We do NOT create empty facts here, because we do not know if the user wants to define an XType only partially
    }
    else
    {
        const xtypes::Relation &existing_relation = this->relations[name];
        if (existing_relation != relation)
        {
            throw std::invalid_argument(this->get_classname() + "::define_relation: A different relation for " + name + " already exists!");
        }
        bool existing_direction = this->relation_dir_forward[name];
        if (existing_direction != inverse)
        {
            throw std::invalid_argument(this->get_classname() + "::define_relation: A different relation direction has already been defined for " + name);
        }
    }
}

bool xtypes::XType::has_relation(const std::string &name) const
{
    if (this->relations.count(name) > 0)
        return true;
    return false;
}

// Returns all the currently defined relations on the XType
std::map<std::string, Relation> xtypes::XType::get_relations() const
{
    return this->relations;
}

Relation xtypes::XType::get_relation(const std::string &name) const
{
    if (!this->has_relation(name))
        throw std::invalid_argument(this->get_classname() + "::get_relation: No relation on " + name + " not defined");
    return this->relations.at(name);
}

// Get the direction in which the attribute gets filled with dependent XType(s)
bool xtypes::XType::get_relations_dir(const std::string &name) const
{
    if (relation_dir_forward.find(name) == relation_dir_forward.end())
        throw std::invalid_argument(this->get_classname() + "::get_relations_dir: Could not find relation direction for " + name);
    return relation_dir_forward.at(name);
}

/* Facts Interface (Facts are Instances of Relations) */

bool xtypes::XType::has_facts(const std::string &name) const
{
    if (!this->has_relation(name))
        throw std::invalid_argument(this->get_classname() + "::has_facts: No relation definition found for " + name);
    if (this->facts.count(name) > 0)
        return true;
    return false;
}

/// Initialize an UNKNOWN fact to be KNOWN and EMPTY
void xtypes::XType::set_unknown_fact_empty(const std::string& name)
{
    if (this->has_relation(name) && !this->has_facts(name))
    {
      this->facts[name] = std::vector<ExtendedFact>{};
    }
}

/// Initializes all UNKNOWN facts to be KNOWN and EMPTY
void xtypes::XType::set_all_unknown_facts_empty()
{
    for (const auto& it : this->relations)
    {
        this->set_unknown_fact_empty(it.first);
    }
}

const std::vector<Fact> xtypes::XType::get_facts(const std::string &name)
{
    std::vector<Fact> result;
    if (!this->has_facts(name))
    {
        throw std::runtime_error(this->get_classname() + "::get_facts("+name+"): Facts unknown");
    }

    // For every fact we have to see if we have a valid pointer
    // If we do, we add that fact to result and are done
    // If we don't, but have a valid target_uri, we ask the registry to get us that thing
    // In that case, we also want to store that pointer in our private facts (that's why get_facts() is not const anymore)
    // Furthermore, we need to do what add_fact() does and auto-fill any matching inverse relation to the new XType
    for (auto& fact : this->facts.at(name))
    {
        if (!fact.target.expired())
        {
            // Update target uri
            fact.target_uri(fact.target_uri());
            result.push_back(fact);
            continue;
        }
        // We do not have a valid target (yet)
        if (fact.target_uri().empty())
        {
            // ... and we dont have an uri, TODO: so we throw or skip? we could also delete it
            throw std::runtime_error(this->get_classname() + "::get_facts("+name+"): Empty fact (both uri and ptr missing)");
        }

        // Now we need the registry
        XTypeRegistryPtr reg = registry.lock();
        if (!reg)
        {
            throw std::runtime_error(this->get_classname() + "::get_facts("+name+"): No registry");
        }
        // We have an uri, so we ask the registry to resolve this
        XTypeCPtr other = reg->load_by_uri(fact.target_uri());
        if (!other)
        {
            // TODO: Throw or skip?
            throw std::runtime_error(this->get_classname() + "::get_facts("+name+"): Registry could not resolve " + fact.target_uri());
        }
        // Store that pointer back
        fact.target = other;
        // Update target uri
        fact.target_uri(fact.target_uri());
        result.push_back(fact);

        // Auto-fill a matching inverse relation
        const Relation our_rel = this->get_relation(name);
        const bool our_forward = this->get_relations_dir(name);
        // Try to find a matching relation at other
        const std::map<std::string, Relation> &other_relations(other->get_relations());
        for (const auto &[other_name, other_rel] : other_relations)
        {
            // If relation directions are the same, they cannot match (the other has to be in the opposite dir)
            const bool other_forward(other->get_relations_dir(other_name));
            if (our_forward == other_forward)
            {
                continue;
            }
            // Check if relation definitions match
            if (our_rel != other_rel)
            {
                continue;
            }
            // We found a match, so we auto-fill the other
            other->add_fact(other_name, shared_from_this(), fact.edge_properties);
        }
    }

    return result;
}

void xtypes::XType::add_fact(const std::string &name, XTypeCPtr other, const nl::json &props)
{
    // FIXME: add_fact() is incomplete:
    // * It has to check if xtype matches the to (or from) domain of the relation definition

    if (!other)
    {
        throw std::invalid_argument(this->get_classname() + "::add_fact("+name+"): Given xtype ptr is invalid");
    }

    // use default relation properties and update it by given properties
    const Relation &rel(this->get_relation(name));
    nl::json updated_props = rel.properties;
    for (auto& [k,v] : updated_props.items())
        if (props.contains(k)) v = props.at(k);

    const bool is_fact_known(this->has_facts(name));
    ExtendedFact new_fact(other, updated_props);
    if (is_fact_known)
    {
        // If fact is already known, update edge properties and return
        auto it = std::find(facts.at(name).begin(), facts.at(name).end(), new_fact);
        if (it != facts.at(name).end())
        {
            // Always update target uri
            // NOTE: If we dont do this, an (now) valid fact stays invalid
            it->target_uri(it->target_uri());
            it->edge_properties = updated_props;
            return;
        }
    }

    // We have a new fact
    // Get cardinality
    Constraint constraint(rel.constraint);
    // Adjust cardinality by relation_dir
    const bool forward(this->get_relations_dir(name));
    if (!forward)
    {
        if (constraint == Constraint::ONE2MANY)
            constraint = Constraint::MANY2ONE;
        else if (constraint == Constraint::MANY2ONE)
            constraint = Constraint::ONE2MANY;
    }
    // Check cardinality constraints
    if (((constraint == Constraint::MANY2ONE) || (constraint == Constraint::ONE2ONE)) && is_fact_known && (this->facts.at(name).size() > 0))
        throw std::length_error(this->get_classname() + "::add_fact("+name+"): Cardinality constraint on does not allow adding another fact");
    this->facts[name].push_back(new_fact);

    // Auto-fill a matching inverse relation
    const Relation our_rel = this->get_relation(name);
    const bool our_forward = this->get_relations_dir(name);
    // Try to find a matching relation at other
    const std::map<std::string, Relation> &other_relations(other->get_relations());
    for (const auto &[other_name, other_rel] : other_relations)
    {
        // If relation directions are the same, they cannot match (the other has to be in the opposite dir)
        const bool other_forward(other->get_relations_dir(other_name));
        if (our_forward == other_forward)
        {
            continue;
        }
        // Check if relation definitions match
        if (our_rel != other_rel)
        {
            continue;
        }
        // We found a match, so we auto-fill the other
        other->add_fact(other_name, shared_from_this(), updated_props);
    }
}

void xtypes::XType::remove_fact(const std::string& name, XTypeCPtr other)
{
    if (!other)
    {
        throw std::invalid_argument(this->get_classname() + "::remove_fact("+name+"): Given xtype ptr is invalid");
    }
    if (!this->has_facts(name))
    {
        return;
    }
    ExtendedFact to_be_removed(other, {});
    auto it = std::remove(this->facts.at(name).begin(), this->facts.at(name).end(), to_be_removed);
    this->facts.at(name).erase(it, this->facts.at(name).end());
    // TODO: We have to remove every matching fact from this to other
    // AND also check if an inverse relation exists at other from which this has to be removed
}


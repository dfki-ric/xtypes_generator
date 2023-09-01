#include "XType.hpp"
#include "XTypeRegistry.hpp"
#include <deque>
#include <iostream>
#include "utils.hpp"

using namespace xtypes;

// Static identifier
const std::string xtypes::XType::classname = "xtypes::XType";

xtypes::XType::XType(const std::string &classname)
    : m_classname(classname.empty() ? typeid(this).name() : classname)
{
    registry = std::make_shared<XTypeRegistry>();
    registry->register_class<XType>();
}

std::string xtypes::XType::get_classname() const noexcept
{
    return m_classname;
}

std::string xtypes::XType::uri() const
{
    return std::string("xtypes://generic");
}

std::size_t xtypes::XType::uuid() const
{
    return xtypes::uri_to_uuid(uri());
}

// recursion_limit == 0: Only the current XType properties are exported (partial export)
// recursion_limit == 1: Current XType properties AND relations are exported, neighbours only partially (without relations)
// recursion_limit > 1: and so forth
URI2Spec xtypes::XType::export_to(const int recursion_limit, const bool export_relation_properties) const
{
    URI2Spec result;
    // This queue stores all XTypes which have to be visited at a certain depth
    std::deque<std::pair<unsigned, ConstXTypePtr>> to_visit = {{0U, shared_from_this()}};
    while (to_visit.size() > 0)
    {
        // Get the current xtype to be visited
        auto [depth, xtype] = to_visit.front();
        to_visit.pop_front();
        std::string xtype_uri = xtype->uri();
        // Check if the current xtype has already been handled
        if (result.count(xtype_uri) > 0)
            continue;
        // NOTE: only_full recursion_limit was here before
        // Place xtype into result set
        result[xtype_uri]["properties"] = xtype->get_properties();
        result[xtype_uri]["uri"] = xtype_uri;
        result[xtype_uri]["uuid"] = std::to_string(xtype->uuid());
        result[xtype_uri]["classname"] = xtype->get_classname();
        result[xtype_uri]["relations"] = nl::json::object();
        // Check if we explore further or not
        if ((recursion_limit >= 0) && (depth >= recursion_limit))
            continue;
        // NOTE: standard recursion_limit was here before
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
                std::string other_uri = other_xtype->uri();
                entry["target"] = other_uri;
                entry["edge_properties"] = rel_props;
                if (export_relation_properties)
                {
                    entry["delete_policy"] = rel_del_pol;
                    entry["relation_dir_forward"] = rel_dir_fwd;
                }
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

// recursion_limit == 0: Only resolve current xtype properties (but no relations)
// recursion_limit == 1: Fully resolve root xtype but only the properties of the nearest neighbours
// recursion_limit > 1: And so forth
XTypePtr xtypes::XType::import_from(const std::string &toplvl_uri, ImportByURIFunc load_spec_by_uri, XTypeRegistry& project_registry, const int recursion_limit)
{
    // Initialize needed variables
    std::map<std::string, XTypePtr> resolved_xtypes;
    std::map<std::string, nl::json> already_loaded_specs;
    using EdgeEntry = std::tuple<std::string, std::string, std::string, nl::json>;
    std::vector<EdgeEntry> found_edges;
    // This queue contains elements of current_depth, the current json spec to deserialize and the previously discovered xtype uri
    // The depth will be used if a recursion_limit has been specified and the previously discovered xtype ('parent') uri will be used to get a registry for instantiation
    std::deque<std::tuple<unsigned, nl::json, std::string>> to_import = {{0, load_spec_by_uri(toplvl_uri), ""}};
    // First pass: resolve all xtypes and note all dependencies
    while (to_import.size() > 0)
    {
        // Get the current spec to be imported
        auto [depth, current_spec, parent_uri] = to_import.front();
        to_import.pop_front();
        // Check if URI exists
        if (current_spec.empty())
        {
            // TODO: Should we better throw here?
            std::cerr << "xtypes::Xtype::import_from: Spec empty. Skipping\n";
            continue;
        }
        if (!current_spec.contains("uri"))
        {
            // This is a true error, we should throw
            throw std::runtime_error("xtypes::Xtype::import_from: Missing URI in " + current_spec.dump());
        }
        // Parse URI
        const std::string &uri(current_spec["uri"]);
        // Check if we already created that xtype
        if (resolved_xtypes.count(uri) > 0)
            continue;
        // Parse classname
        const std::string &classname(current_spec["classname"]);
        // Instantiate the correct XType
        XTypePtr current_xtype(nullptr);
        // Check if parent Xtype exists
        if (!parent_uri.empty())
        {
            // Use registry of 'parent' XType to instantiate dependent XTypes
            current_xtype = resolved_xtypes.at(parent_uri)->registry->instantiate_from(classname);
            if (!current_xtype)
            {
                throw std::runtime_error("xtypes::XType::import_from: Instantiation from parent registry failed. Is the class " + classname + " registered in constructor of " + resolved_xtypes.at(parent_uri)->get_classname() + "?");
            }
        }
        else if (uri == toplvl_uri)
        {
            // Use a temporary Registry to instantiate the toplvl XType
            current_xtype = project_registry.instantiate_from(classname);
            if (!current_xtype)
            {
                throw std::runtime_error("xtypes::XType::import_from: Instantiation from given project registry failed. Is the class " + classname + " registered?");
            }
        }
        else
        {
            throw std::logic_error("xtypes::XType::import_from: Cannot instantiate " + uri + " because i have no registry");
        }
        // NOTE: We updated the serialization to group properties and relations, but we want to be downwards compatible to the old version
        if (current_spec.contains("properties"))
        {
            current_xtype->set_properties(current_spec["properties"]);
        } else {
            std::cerr << "xtypes::import_from: WARNING: Falling back to old import of properties for " << uri << "\n";
            current_xtype->set_properties(current_spec, false);
        }
        // Register new XType as being resolved
        resolved_xtypes[uri] = current_xtype;
        // Resolve dependencies to other XTypes
        // NOTE: We updated the serialization to group properties and relations, but we want to be downwards compatible to the old version
        nl::json relation_spec;
        if (current_spec.contains("relations"))
        {
            relation_spec = current_spec["relations"];
        } else {
            std::cerr << "xtypes::import_from: WARNING: Falling back to old import of relations for " << uri << "\n";
            relation_spec = current_spec;
        }
        const auto &rels(current_xtype->get_relations());
        for (const auto &[rel_name, rel] : rels)
        {
            // Check if we have hit the recursion limit
            if (recursion_limit >= 0 && depth >= recursion_limit)
                continue;
            // A missing entry means that the facts of a certain relation could not be resolved and are UNKNOWN
            if (!relation_spec.contains(rel_name))
                continue;
            // Initialize KNOWN fact entry (could still be EMPTY though)
            current_xtype->set_unknown_fact_empty(rel_name);
            // Note all relations to other xtypes which have to be fully resolved in the second pass
            for (const auto &entry : relation_spec[rel_name])
            {
                const std::string &other_uri(entry["target"]);
                found_edges.push_back(EdgeEntry{uri, rel_name, other_uri, entry["edge_properties"]});
                if (resolved_xtypes.count(other_uri))
                    continue;
                // Populate spec cache to prevent duplicate loads (which might be from file or something slow)
                if (!already_loaded_specs.count(other_uri))
                {
                    const nl::json& other_spec(load_spec_by_uri(other_uri));
                    if (other_spec.empty())
                    {
                        // TODO: Should we better throw here?
                        std::cerr << "xtypes::Xtype::import_from: Could not import reference" << other_uri << ". Skipping\n";
                        continue;
                    }
                    already_loaded_specs[other_uri] = other_spec;
                }
                to_import.push_back({depth + 1, already_loaded_specs.at(other_uri), uri});
            }
        }
    }
    // Second pass: Resolve dependencies between resolved xtypes
    for (const auto &[uri, rel_name, other_uri, edge_props] : found_edges)
    {
        // Get the source and destination xtype(s)
        xtypes::XTypePtr src_xtype(resolved_xtypes.at(uri));
        // It could be that the destination XType has not been imported properly. Shall we raise here?
        if (resolved_xtypes.count(other_uri) < 1)
            continue;
        const xtypes::XTypePtr dest_xtype(resolved_xtypes.at(other_uri));
        src_xtype->add_fact(rel_name, dest_xtype, edge_props);
    }
    return resolved_xtypes[toplvl_uri];
}

//void xtypes::XType::define_property(const std::string &name, const nl::json &default_value, const nl::json::value_t &type, const bool &override)
void xtypes::XType::define_property(const std::string& name,
                     const nl::json::value_t& type,
                     const std::set<nl::json>& allowed_values,
                     const nl::json& default_value,
                     const bool& override)
{
    if (this->has_property(name) && !override)
    {
        throw std::invalid_argument(this->get_classname() + "::define_property(): Property " + name + " already defined!");
    }
    this->property_types[name] = type;
    if (!is_type_matching(name, default_value))
    {
        throw std::invalid_argument(this->get_classname() + "::define_property: Default value type mismatch. " +
                                    "Defined type " + value_t2string.at(type) + ", but received " + value_t2string.at(default_value.type()));
    }
    this->allowed_property_values[name] = allowed_values;
    if (!is_allowed_value(name, default_value))
    {
        throw std::invalid_argument(this->get_classname() + "::define_property: Default value " + default_value.dump() + " is not allowed");
    }
    this->properties[name] = default_value;
}

bool xtypes::XType::has_property(const std::string &name) const
{
    return this->properties.count(name) > 0;
}

nl::json::value_t xtypes::XType::get_property_type(const std::string &name) const
{
    return this->property_types.at(name);
}

std::set<nl::json> xtypes::XType::get_allowed_property_values(const std::string& name) const
{
    return this->allowed_property_values.at(name);
}

bool xtypes::XType::is_allowed_value(const std::string& name, const nl::json& value)
{
    auto allowed_values = this->get_allowed_property_values(name);
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

bool xtypes::XType::is_type_matching(const std::string &name, const nl::json &value)
{
    // Types match directly
    if (this->get_property_type(name) == value.type())
        return true;
    // Sometimes an unsigned/signed integer shall be assigned to an signed/unsigned value
    // NOTE: This can happen if nlohmann::json interprets an signed value as being an unsigned value
    if (this->get_property_type(name) == nl::json::value_t::number_integer && value.type() == nl::json::value_t::number_unsigned)
        return true; // TODO: Check that unsigned value < signed positive max!
    if (this->get_property_type(name) == nl::json::value_t::number_unsigned && value.type() == nl::json::value_t::number_integer)
        return value >= 0U;
    // The initial type is discarded, so we match anything
    if (this->get_property_type(name) == nl::json::value_t::discarded)
        return true;
    // For dictionaries we have to handle the empty dict initialization case
    if (this->get_property_type(name) == nl::json::value_t::object && value.type() == nl::json::value_t::null)
        return true;
    return false;
}

void xtypes::XType::set_property(const std::string &name, const nl::json &new_value, const bool shall_throw)
{
    // Check if the property has been defined
    if (!this->has_property(name))
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Property " + name + " not found.");
        }
        return;
    }
    // We should not be able to change the type here, so we check it
    if (shall_throw && this->get_property_type(name) == nl::json::value_t::discarded)
    {
        std::cerr << this->get_classname() + "::set_property: No type defined for property " + name + ". Type safety isn't assured." << std::endl;
    }
    else if (!is_type_matching(name, new_value)) // handle empty dict
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Property " + name + ": Type mismatch. " +
                                        "Expected " + value_t2string.at(this->get_property_type(name)) + ", but received " + value_t2string.at(new_value.type()));
        }
        return;
    }
    else if (!is_allowed_value(name, new_value))
    {
        if (shall_throw)
        {
            throw std::invalid_argument(this->get_classname() + "::set_property: Value " + new_value.dump() + " not allowed for property " + name);
        }
        return;
    }
    this->properties[name] = new_value;
}

nl::json xtypes::XType::get_property(const std::string &name) const
{
    if (!this->has_property(name))
    {
        throw std::invalid_argument(this->get_classname() + "::get_property: Property " + name + " not found.");
    }
    return this->properties.at(name);
}

nl::json xtypes::XType::get_properties() const
{
    nl::json result;
    for (const auto &[name, value] : this->properties)
    {
        result[name] = value;
    }
    return result;
}

void xtypes::XType::set_properties(const nl::json &properties, const bool shall_throw)
{
    for (const auto &[name, val] : properties.items()) // requires c++17 and up
    {
        this->set_property(name, val, shall_throw);
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

std::vector<Fact> &xtypes::XType::get_facts(const std::string &name)
{
    return const_cast<std::vector<Fact> &>(const_cast<const XType *>(this)->get_facts(name));
}

const std::vector<Fact> &xtypes::XType::get_facts(const std::string &name) const
{
    // NOTE: Empty fact entries mean that there are no facts indeed, but a missing entry means that we do not know if there are facts or not
    if (!this->has_facts(name))
        throw std::underflow_error(this->get_classname() + "::get_facts: No fact(s) found for " + name + ". Might have to increase recursion limits?");
    return this->facts.at(name);
}

void xtypes::XType::remove_fact(const std::string& name, XTypeCPtr xtype)
{
    // Check if a relation definition with that name exists FIRST!
    if (!this->has_relation(name))
        throw std::invalid_argument(this->get_classname() + "::remove_fact: No relation definition found for " + name);
    if (!xtype)
        throw std::invalid_argument(this->get_classname() + "::remove_fact: Given xtype ptr is invalid for fact of " + name);
    if (!this->has_facts(name))
        return;
    // Remove any matching facts
    auto it = std::remove_if(this->facts.at(name).begin(), this->facts.at(name).end(),
            [&](const Fact& f) -> bool {
                return (f.target.lock() == xtype);
            }
            );
    this->facts.at(name).erase(it, this->facts.at(name).end());
}
void xtypes::XType::add_fact(const std::string &name, XTypeCPtr xtype, const nl::json &props)
{
    // FIXME: add_fact() is incomplete:
    // * It has to check if xtype matches the to (or from) domain of the relation definition
    // * It has to check if the properties match the keys of the default properties in the relation definition

    // Check if a relation definition with that name exists FIRST!
    if (!this->has_relation(name))
        throw std::invalid_argument(this->get_classname() + "::add_fact: No relation definition found for " + name);
    if (!xtype)
        throw std::invalid_argument(this->get_classname() + "::add_fact: Given xtype ptr is invalid for fact of " + name);

    // Check if xtype already listed
    // NOTE: This will impicitly add a non-existing fact entry!
    for (const auto &x : this->facts[name])
    {
        if (x.target.lock() == xtype)
            return;
    }
    // Get cardinality
    const Relation &rel(this->get_relation(name));
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
    if (((constraint == Constraint::MANY2ONE) || (constraint == Constraint::ONE2ONE)) && (this->facts[name].size() > 0))
        throw std::length_error(this->get_classname() + "::add_fact: Cardinality constraint on " + name + " of " + this->m_classname + " does not allow adding another " + xtype->m_classname);

    // Add new fact
    this->set_unknown_fact_empty(name);
    this->facts[name].push_back(Fact(xtype, props));

    // Search in xtype for a matching relation (with opposite direction) and also add a fact there
    // NOTE: In older implementations of xtype lib this has been optional but default. Now it is standard.
    const std::map<std::string, Relation> &other_relations(xtype->get_relations());
    for (const auto &[other_name, other_rel] : other_relations)
    {
        // If relation directions are the same, they cannot match (the other has to be in the opposite dir)
        const bool other_forward(xtype->get_relations_dir(other_name));
        if (forward == other_forward)
        {
            //std::cout << "direction match\n";
            continue;
        }
        // Check if relation definitions match
        if (rel != other_rel)
        {
            //std::cout << "no match between " << name << " and " << other_name << "\n";
            continue;
        }
        //std::cout << "*** match between " << name << " and " << other_name << "\n";
        // We found a match, so we auto-fill the other
        xtype->add_fact(other_name, shared_from_this(), props);
    }
}

/// Initialize an UNKNOWN fact to be KNOWN and EMPTY
void xtypes::XType::set_unknown_fact_empty(const std::string& name)
{
    if (this->has_relation(name) && !this->has_facts(name))
    {
      this->facts[name] = std::vector<Fact>{};
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

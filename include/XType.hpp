#pragma once

#include <nlohmann/json.hpp>
#include <functional>
#include <stdexcept>
#include <vector>
#include <memory>
#include <string>
#include <map>
#include <set>
#include <deque>

#include "enums.hpp"
#include "structs.hpp"
#include "XTypeRegistry.hpp"

namespace nl = nlohmann;
namespace xtypes {
    /*Forward declarations*/
    class XType;
    class XTypeRegistry;

    /* Some good aliases */
    using XTypePtr = std::shared_ptr< XType >;
    using XTypeCPtr = const std::shared_ptr< XType >;
    using ConstXTypePtr = std::shared_ptr< const XType >;
    using ConstXTypeCPtr = const std::shared_ptr< const XType >;

    /** Base class for all XType specializations.
     * XTypes are an object oriented type definition which like a normal class hold properties and can have member functions;
     * but further more they can hold relations to other XTypes. */
    class XType : public std::enable_shared_from_this<XType>
    {
    public:
        XType(const std::string& classname = XType::classname);

        /// The constant identifier of this class
        static const std::string classname;

        /** This function returns the classname reported back from derived classes.
         * Useful for lookup from base class to derived class. */
        std::string get_classname() const noexcept;

        /// This method is modified by each derived type (see xtypes_generator)
        virtual std::string uri() const;

        /// This method checks if the uri can be build
        bool is_uri_valid() const;

        /// URI comparison for usage of XTypes
        virtual bool operator==(const XType& other) const { return this->uri() == other.uri(); };
        virtual bool operator!=(const XType& other) const { return !(*this == other); };
        virtual bool operator<(const XType& other) const { return this->uri() < other.uri(); };
        virtual bool operator>(const XType& other) const { return other < *this; };
        virtual bool operator<=(const XType& other) const { return !(*this > other); };
        virtual bool operator>=(const XType& other) const { return !(*this < other); };

        /// This method has to always return an up-to-date hash of the current uri
        std::size_t uuid() const;

        /// Sets the registry of the XType if not already set
        void set_registry_once(XTypeRegistryCPtr reg);

        /// Overwrite the registry
        /// TODO: Make this a protected member function with friend class XTypeRegistry?
        void overwrite_registry(XTypeRegistryCPtr reg);

        /// Returns the registry of this XType if set. Otherwise nullptr
        XTypeRegistryCPtr get_registry() const;

        /* De-/Serialization from/to JSON */

        /**
         * Exports an XType, its dependecies and properties into a JSON object
         * NOTE: This function is part of the basis for a json based XType database
         * @param max_depth Depth limit up to which dependent XTypes are resolved. -1 means no depth limit (full export)
         *        Use max_depth != -1 if you want to do only a partial update. You should choose max_depth that it encompsas all made changes
         * @returns The serialization in JSON
         */
        std::map<std::string, nl::json> export_to(const int max_depth=-1);

        /**
         * Imports an XType, its dependencies and properties from an JSON object
         * NOTE: This function is part of the basis for a json based XType database
         * NOTE: We need a registry to create a new XType
         * @param spec The JSON object to import from
         * @param registry The project registry to use to import an XType
         * @returns The resolved XType
         */
        static XTypeCPtr import_from(const nl::json& spec, XTypeRegistryCPtr reg);

        /* Property Interface */

        // TODO: Add documentation

        void set_property_schema(const nl::json& schema);
        nl::json get_property_schema() const;
        bool has_property(const nl::json& path_to_key) const;

        nl::json::value_t get_property_type(const nl::json& path_to_key) const;
        bool is_type_matching(const nl::json& path_to_key, const nl::json& value) const;

        bool is_allowed_value(const nl::json& path_to_key, const nl::json& value) const;
        std::set<nl::json> get_allowed_property_values(const std::string& name) const;

        nl::json get_property(const nl::json& path_to_key) const;
        void set_property(const nl::json& path_to_key, const nl::json& new_value, const bool shall_throw = true);

        nl::json get_properties() const;
        void set_properties(const nl::json& properties, const bool shall_throw = true);

        /* Relation Interface */

        /** General function to define any relation. See below for predefined relations using this function internally.
         * NOTE: Moved here from Schema.hpp
         * @param The name for the new relation
         * @param The relation type (a name like "has", "is-a" etc.)
         * @param Defines the type(s) of XType on the from side of the relation
         * @param Defines the type(s) of XType on the to side of the relation
         * @param Cardinaltiy Constraint
         * @param DeletePolicy
         * @param The properties of this relation (will be stored in the resulting fact)
         * @param If this relation is a subrelation of another pass here the other's relation type (see relation_type)
         * @param Whether this relation is defined on the to side
         * @param Whether we override an existing relation under this name
         */
        void define_relation(const std::string& name,
                          const RelationType& relation_type,
                          std::set<std::string> from_classnames,
                          std::set<std::string> to_classnames,
                          const Constraint& constraint = Constraint::MANY2MANY,
                          const DeletePolicy& delpolicy = DeletePolicy::DELETENONE,
                          const nl::json& properties = {},
                          const RelationType& super_relation_type = RelationType::NONE,
                          const bool& inverse = false,
                          const bool& override = false);

        /// Checks if a relation has been defined under the given attribute name or not
        bool has_relation(const std::string& name) const;

        /// Returns all the currently defined relations on the XType
        std::map<std::string, Relation> get_relations() const;

        /// Get the relation definition for the given attribute name
        Relation get_relation(const std::string& name) const;

        /// Get the direction in which the attribute gets filled with dependent XType(s)
        /// true if points forward, false otherwise
        bool get_relations_dir(const std::string& name) const;

        /* Facts Interface (Facts are Instances of Relations) */

        /* Check if facts related to name exist or not
         * NOTE: This is NOT to confused with has_relation which checks if a DEFINITION for a relation exists.
         * The facts would be instances of that relation definition (and do not necessarily have to exist because they are UNKNOWN)
         * NOTE: If the facts are empty, this function will return true (since they are not UNKNOWN)*/
        // TODO: A better name would be are_facts_known()
        bool has_facts(const std::string& name) const;

        /// Initialize an UNKNOWN fact to be KNOWN and EMPTY
        void set_unknown_fact_empty(const std::string& name);

        /// Initializes all UNKNOWN facts to be KNOWN and EMPTY
        void set_all_unknown_facts_empty();

        /// Retrieve all the facts currently stored in the given named attribute
        const std::vector< Fact > get_facts(const std::string& name);

        /** Add a fact to the named relation
         *  @param The name of the relation
         *  @param The target XType
         *  @param The fact properties
         */
        void add_fact(const std::string& name, XTypeCPtr other, const nl::json& props={});

        /**
         *  Remove a fact from a named relation
         *  @param The name of the relation
         *  @param The target XType
         */
        void remove_fact(const std::string& name, XTypeCPtr other);

        /* Predefined relations */
        void HAS(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            // HINT: This function creates the correct attribute and defines an 'add_<name>' function to access it with constraints checking
            define_relation(name, RelationType::HAS, {this->get_classname()}, other_classnames, Constraint::ONE2MANY, DeletePolicy::DELETETARGET, properties, RelationType::NONE, inverse, override);
        }
        void PART_OF_COMPOSITION(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::PART_OF_COMPOSITION, {this->get_classname()}, other_classnames, Constraint::MANY2ONE, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }
        // 2022-09-22 MS: Changed constraint to MANY2MANY to allow multiple inheritance
        void SUBCLASS_OF(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::SUBCLASS_OF, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }
        void IMPLEMENTS(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::IMPLEMENTS, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }
        void INSTANCE_OF(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::INSTANCE_OF, {this->get_classname()}, other_classnames, Constraint::MANY2ONE, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void CONNECTED_TO(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::CONNECTED_TO, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void ALIAS_OF(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::ALIAS_OF, {this->get_classname()}, other_classnames, Constraint::ONE2ONE, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void NEEDS(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::NEEDS, {this->get_classname()}, other_classnames, Constraint::ONE2MANY, DeletePolicy::DELETETARGET, properties, RelationType::HAS, inverse, override);
        }

        void PROVIDES(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::PROVIDES, {this->get_classname()}, other_classnames, Constraint::ONE2MANY, DeletePolicy::DELETETARGET, properties, RelationType::HAS, inverse, override);
        }

        void CONTAINED_BY(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::CONTAINED_BY, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void EXISTS_IN(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::EXISTS_IN, {this->get_classname()}, other_classnames, Constraint::MANY2ONE, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        /// Note: GENERATED is a subclass of HAS, maybe change in the future
        void GENERATED(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::GENERATED, {this->get_classname()}, other_classnames, Constraint::ONE2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void DEPENDS_ON(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::DEPENDS_ON, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void CONSTRAINED_BY(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::CONSTRAINED_BY, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void INTERFACE_TO(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::INTERFACE_TO, {this->get_classname()}, other_classnames, Constraint::MANY2ONE, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void SPANS(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::SPANS, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETETARGET, properties, RelationType::NONE, inverse, override);
        }

        void HAS_UNIQUE(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::HAS_UNIQUE, {this->get_classname()}, other_classnames, Constraint::ONE2ONE, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void ANNOTATES(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::ANNOTATES, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETENONE, properties, RelationType::NONE, inverse, override);
        }

        void ATTACHED_TO(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::ATTACHED_TO, {this->get_classname()}, other_classnames, Constraint::MANY2ONE, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void CAN_SAMPLE(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::CAN_SAMPLE, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

        void CONDITIONABLE_ON(const std::string& name, const std::set<std::string>& other_classnames, const nl::json& properties={}, bool inverse = false, bool override = false)
        {
            define_relation(name, RelationType::CONDITIONABLE_ON, {this->get_classname()}, other_classnames, Constraint::MANY2MANY, DeletePolicy::DELETESOURCE, properties, RelationType::NONE, inverse, override);
        }

    protected:
        /// Every XType gets a registry instance which has to be used when instantiating new XType(s) during runtime
        std::weak_ptr< XTypeRegistry > registry;

        std::string m_classname;

        std::map< std::string, Relation > relations;        /* < specifies which relation is attached to the corresponding entry in facts */
        std::map< std::string, bool > relation_dir_forward; /* < specifies whether the target (forward direction) or the source of a relation is filled into the corresponding entry in facts */
        std::map< std::string, std::vector< ExtendedFact > > facts; /* < Holds facts/references to other XTypes (either by URI or by weak pointer) */

        // The schema contains a JSON object with the following structure:
        // key1:
        //   type:
        //   (default:)
        //   (allowed:)
        // key2:
        //   subkey1:
        //     type:
        //     (default:)
        //     (allowed:)
        //   subkey2:
        //     subsubkey1:
        //       ...
        nl::json property_schema;
        nl::json properties;

        /// Checks whether the passed pointer is an instance of the base class
        template<typename Base, typename T>
        inline bool isinstance(const T *ptr)
        {
           return dynamic_cast<const Base*>(ptr) != nullptr;
        }

    };
}

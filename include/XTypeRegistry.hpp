#pragma once

/*
 * XType registry/factory
 *
 * In here all derived classes of an XType are registered and can be instantiated by classname string.
 * This class follows the Functional Factory Pattern.
 *
 * NOTE: The registry returns SHARED pointers and not UNIQUE ones.
 * We do that to allow others to use the registry as a member or argument when they instantiate new XTypes.
 *
 */

#include <functional>
#include <memory>
#include <map>
#include <string>
#include <vector>

/**
 * XType v3
 *
 * * we need to store instances by uri
 * * The registry will be a global entity (not constructed anymore inside the XTypes but passed by reference)
 * * The registry defines and stores a function to load instances from some informations source (probably xdbi) if an XType with a certain uri cannot be found
 */

namespace xtypes
{
    class XType;

    /* Some good aliases */
    using XTypePtr = std::shared_ptr< XType >;
    using XTypeCPtr = const std::shared_ptr< XType >;
    using ConstXTypePtr = std::shared_ptr< const XType >;
    using ConstXTypeCPtr = const std::shared_ptr< const XType >;
    using UniqueXTypePtr = std::unique_ptr< XType >;
    using XTypeWeakPtr = std::weak_ptr< XType >;

    /// Base Class for the Registries of the XTypes and for the Project Registries in the XTypes specializing projects
    struct XTypeRegistry : public std::enable_shared_from_this<XTypeRegistry>
    {
        /// Factory constructor
        XTypeRegistry();

        // Need virtual destructor to become polymorphic (for pybind11)
        virtual ~XTypeRegistry() = default;

        /// *** Classes API ***

        /// Function to register a new XType
        template <typename T> void register_class();

        /// Function to register an alias to an existing class (useful for conversions)
        bool register_alias(const std::string& original, const std::string& alias);

        /// Check if a classname is known to the registry
        bool knows_class(const std::string& with_name) const;

        /// Import factory functions from other registry
        void import_from(const XTypeRegistry& other);

        /// *** Instances API ***
        /// NOTE: These functions DO not register any instance, ownership of the shared pointers is completely given to to the caller

        /// Factory method to create an XType from string
        XTypeCPtr instantiate_from(const std::string& classname);

        /// Factory method to create XType from C++ type
        template <typename T> const std::shared_ptr<T> instantiate();

        /// *** Valid instances API ***
        
        /// This function tells us if an XType with a given URI is known to the registry
        bool knows_uri(const std::string& uri) const;

        /// If an XType has become valid (externally) it can be passed to the registry to be persistently stored
        /// Then others can find this instance by uri
        /// overwrite_if_exists speicifies if the content of the committed entity shall be copied over the existing one
        bool commit(XTypeCPtr& instance, const bool overwrite_if_exists);

        /// This function creates a new temporary object with the content of an valid instance in _valid_instances
        XTypeCPtr get_by_uri(const std::string& uri);

        /// This function will create a new temporary object from an external information source if not found by get_by_uri()
        XTypeCPtr load_by_uri(const std::string& uri);

        /// Defintion of a function to lookup unknown XTypes from some information source
        // When we use lambdas, we can just catch the right registry and the right information source
        using LoadByUriFunc = std::function< XTypePtr(const std::string& uri) >;

        /// Set the load_func to be used by the registry to resolve an XType given the uri
        void set_load_func(const LoadByUriFunc& f);

        /// Removes an valid instance from registry
        /// TODO: This could invalidate other dependent XTypes. Has to be handled!
        void drop(const std::string& uri);

        /// Drop all valid instances
        void clear();

    private:
        // Factory function repository: classname -> factory function
        std::map<std::string, std::function<UniqueXTypePtr()>> _factories;
        // A function to load unknown XTypes from some information source
        LoadByUriFunc _load_func;
        // Every instantiated XType is registered here (might not be valid yet)
        // analogous to GIT UNVERSIONED FILES
        std::vector< XTypePtr > _temporary_instances;
        // When a temporary copy of a valid instance is created it is registered here
        // analogous to GIT MODIFIED FILES
        std::map< std::string, XTypeWeakPtr > _valid_to_temporary;
        // Map of uri to XType instances
        // NOTE: This map GLOBALLY stores all XTypes committed to this registry
        // Only XTypes with a valid uri can be stored in here (either loaded from DB or created and registered)
        // analogous to GIT VERSIONED/COMMITTED FILES
        std::map< std::string, XTypePtr > _valid_instances;
    };

    using XTypeRegistryPtr = std::shared_ptr<XTypeRegistry>;
    using XTypeRegistryCPtr = const std::shared_ptr<XTypeRegistry>;
    using ConstXTypeRegistryPtr = std::shared_ptr< const XTypeRegistry>;
    using ConstXTypeRegistryCPtr = const std::shared_ptr< const XTypeRegistry>;

    /* Template functions */
    // NOTE: We cannot directly access any XType specific stuff 
    // inside these because of circular dependency between XType.hpp and XTypeRegistry.hpp

    template <typename T> void XTypeRegistry::register_class()
    {
        if (knows_class(T::classname))
            return;
        _factories[T::classname] = []{ 
            return std::make_unique<T>();
        };
    }

    // This function is needed to automatically place a ref to the registry in new XTypes
    // Do not remove or deprecate this
    template <typename T> const std::shared_ptr<T> XTypeRegistry::instantiate()
    {
        return std::static_pointer_cast<T>(instantiate_from(T::classname));
    }

}


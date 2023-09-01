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

namespace xtypes
{
    class XType;

    /// Base Class for the Registries of the XTypes and for the Project Registries in the XTypes specializing projects
    struct XTypeRegistry
    {
        /// Factory constructor
        XTypeRegistry()
        {
        }

        /// Function to register a new XType
        template <typename T>
        void register_class()
        {
            if (knows_class(T::classname))
                return;
            _factories[T::classname] = []
            { return std::make_unique<T>(); };
        }

        /// Function to register an alias to an existing class (useful for conversions)
        bool register_alias(const std::string &original, const std::string& alias)
        {
            if (!knows_class(original))
                return false;
            if (knows_class(alias))
                return false;
            _factories[alias] = _factories[original];
            return true;
        }

        /// Check if a classname is known to the registry
        bool knows_class(const std::string &with_name) const
        {
            if (_factories.find(with_name) != _factories.end())
                return true;
            return false;
        }

        /// Factory method to create an XType from string
        std::shared_ptr<XType> instantiate_from(const std::string &classname)
        {
            if (knows_class(classname))
            {
                _instances.push_back(_factories.at(classname)());
                return _instances.back();
            }
            return nullptr;
        }

        /// This function returns the XType with the given URI if it is held by the registry, otherwise returns nullptr
        template <typename T>
        std::shared_ptr<T> get_by_uri(const std::string &uri)
        {
            auto it = std::find_if(_instances.begin(), _instances.end(), [&uri](auto &inst)
                                   { return inst->uri() == uri; });
            if (it != _instances.end())
                return std::static_pointer_cast<T>(*it);
            return nullptr;
        }

        /// Factory method to create XType from C++ type
        template <typename T>
        std::shared_ptr<T> instantiate()
        {
            return std::static_pointer_cast<T>(instantiate_from(T::classname));
        }

        /** Function to insert a shared pointer to an XType instance
         * Useful when the lifetime of the instance would be too short otherwise */
        template <typename T>
        void hold(const std::shared_ptr<T> &instance)
        {
            if (holds(instance))
                return;
            if (!knows_class(instance->get_classname()))
                _factories[instance->get_classname()] = []
                { return std::make_unique<T>(); };
            _instances.push_back(instance);
        }

        /// Checks if an instance is held by the registry
        bool holds(const std::shared_ptr<XType> &instance)
        {
            if (std::find(_instances.begin(), _instances.end(), instance) != _instances.end())
                return true;
            return false;
        }

        /// Removes instance from registry
        void drop(const std::shared_ptr<XType> &instance)
        {
            if (holds(instance))
                _instances.erase(std::remove(_instances.begin(), _instances.end(), instance), _instances.end());
        }

        /// Drop all instances
        void clear()
        {
            _instances.clear();
        }

        /// Import factory functions from other registry
        void import_from(const XTypeRegistry &other)
        {
            for (const auto &[classname, func] : other._factories)
            {
                if (knows_class(classname))
                    continue;
                _factories[classname] = func;
                // TODO: Shall we import instances as well?
            }
        }

    private:
        std::map<std::string, std::function<std::unique_ptr<XType>()>> _factories;
        std::vector<std::shared_ptr<XType>> _instances;
    };

    using XTypeRegistryPtr = std::shared_ptr<XTypeRegistry>;

    /// Specialization for XType
    template <>
    inline void XTypeRegistry::hold(const std::shared_ptr<XType> &instance)
    {
        if (holds(instance))
            return;
        _instances.push_back(instance);
    }
}

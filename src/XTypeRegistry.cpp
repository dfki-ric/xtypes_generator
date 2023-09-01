#include "XTypeRegistry.hpp"
#include "XType.hpp"

namespace xtypes {

XTypeRegistry::XTypeRegistry()
{
    register_class<XType>();
}

bool XTypeRegistry::register_alias(const std::string& original, const std::string& alias)
{
    if (!knows_class(original))
        return false;
    if (knows_class(alias))
        return false;
    _factories[alias] = _factories[original];
    return true;
}

bool XTypeRegistry::knows_class(const std::string& with_name) const
{
    if (_factories.find(with_name) != _factories.end())
        return true;
    return false;
}

void XTypeRegistry::import_from(const XTypeRegistry& other)
{
    for (const auto &[classname, func] : other._factories)
    {
        if (knows_class(classname))
            continue;
        _factories[classname] = func;
        // TODO: Shall we import instances as well?
    }
}

XTypeCPtr XTypeRegistry::instantiate_from(const std::string& classname)
{
    if (knows_class(classname))
    {
        XTypePtr instance(_factories.at(classname)());
        // Set the registry to this registry
        instance->set_registry_once(shared_from_this());
        _temporary_instances.push_back(instance);
        return instance;
    }
    return nullptr;
}

bool XTypeRegistry::knows_uri(const std::string& uri) const
{
    if (_valid_instances.count(uri))
    {
        return true;
    }
    return false;
}

bool XTypeRegistry::commit(XTypeCPtr& instance, const bool overwrite_if_exists)
{
    // Check if the instance is valid
    if (!instance->is_uri_valid())
        return false;
    // Store uri
    const std::string uri(instance->uri());
    // Make sure the instance knows us (only if not yet set!)
    instance->set_registry_once(shared_from_this());
    // Create a new entry in _valid_instances if not found
    if (!knows_uri(uri))
    {
        // NOTE: We have to do this manually because it shall not be in _temporary_instances
        _valid_instances[uri] = _factories.at(instance->get_classname())();
        *(_valid_instances.at(uri)) = *instance;
    }
    else if (overwrite_if_exists)
    {
        // Copy the content of instance into _valid_instances
        *(_valid_instances.at(uri)) = *instance;
    }
    // Make sure the registry of the valid instance is set to us!
    _valid_instances.at(uri)->overwrite_registry(shared_from_this());
    // NOTE: We do not invalidate any temporary instances or copies of valid instances.
    return true;
}

XTypeCPtr XTypeRegistry::get_by_uri(const std::string& uri)
{
    XTypePtr result;

    // Check if that uri is known to point to a temporary instance
    if (_valid_to_temporary.count(uri) && !_valid_to_temporary.at(uri).expired())
    {
        // A valid copy exists. However it could be that it has been changed/altered by the user
        result = _valid_to_temporary.at(uri).lock();
        const std::string current_uri(result->uri());
        // If the uris do not match
        if (uri != current_uri)
        {
            // ... we have to create a new temporary entry, so others can find that thing by old as well as new URI
            _valid_to_temporary[current_uri] = result;
        }
        return result;
    }

    // When we come out here, we do NOT have a temporary copy yet
    if (!knows_uri(uri))
    {
        // We do not know ANY temporary or valid instance which has that uri
        return nullptr;
    }
    // We know that uri, so we create a new temporary copy of it
    XTypePtr valid(_valid_instances.at(uri));
    result = instantiate_from(valid->get_classname());
    *result = *valid;
    _valid_to_temporary[uri] = result;
    return result;
}

XTypeCPtr XTypeRegistry::load_by_uri(const std::string& uri)
{
    XTypePtr instance(get_by_uri(uri));
    if (instance)
    {
        return instance;
    }
    // Does not yet exist, so we ask the load func
    instance = _load_func(uri);
    if (!instance)
    {
        return nullptr;
    }
    if (uri != instance->uri())
    {
        throw std::runtime_error("XTypeRegistry::load_by_uri("+uri+"): URI mismatch!");
    }
    // Make sure that instance is committed (the _load_func could already have done it though)
    if (!knows_uri(uri) && !commit(instance, true))
    {
        return nullptr;
    }
    // We use get_by_uri() to now give us a temporary copy of the valid one
    return get_by_uri(uri);
}

void XTypeRegistry::set_load_func(const LoadByUriFunc& f)
{
    _load_func = f;
}

// TODO: drop() should just erase an XType from _valid_instances and _valid_copies to trigger a reload
void XTypeRegistry::drop(const std::string& uri)
{
    _valid_instances.erase(uri);
    _valid_to_temporary.erase(uri);
}

// TODO: We need a delete(uri) function to drop() and also remove any invalidated other instances
// TODO: When we drop a valid instance others might also become invalid.
// In that case we also have to destroy them or put them (back) to temporary instances and return them

void XTypeRegistry::clear()
{
    _temporary_instances.clear();
    _valid_to_temporary.clear();
    _valid_instances.clear();
}

}

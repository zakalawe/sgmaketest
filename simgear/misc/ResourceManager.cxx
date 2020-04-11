// ResourceManager.cxx -- manage finding resources by names/paths
// Copyright (C) 2010 James Turner
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//
// $Id$

#include <assert.h>
#include <algorithm>

#include <simgear_config.h>

#include <simgear/misc/ResourceManager.hxx>
#include <simgear/debug/logstream.hxx>

namespace simgear
{
    
static ResourceManager* static_manager = nullptr;

ResourceProvider::~ResourceProvider()
{
    // pin to this compilation unit
}

ResourceManager::ResourceManager()
{
    
}

ResourceManager* ResourceManager::instance()
{
    if (!static_manager) {
        static_manager = new ResourceManager();
    }
    
    return static_manager;
}    

ResourceManager::~ResourceManager()
{
    assert(this == static_manager);
    static_manager = nullptr;
    std::for_each(_providers.begin(), _providers.end(), 
        [](ResourceProvider* p) { delete p; });
}
/**
 * trivial provider using a fixed base path
 */
class BasePathProvider : public ResourceProvider
{
public:
    BasePathProvider(const SGPath& aBase, ResourceManager::Priority aPriority) :
        ResourceProvider(aPriority),
        _base(aBase)
    {
        
    }
    
    virtual SGPath resolve(const std::string& aResource, SGPath&) const
    {
        SGPath p(_base, aResource);
        return p.exists() ? p : SGPath();
    }
private:
    SGPath _base;  
};

void ResourceManager::addBasePath(const SGPath& aPath, Priority aPriority)
{
    addProvider(new BasePathProvider(aPath, aPriority));
}

void ResourceManager::addProvider(ResourceProvider* aProvider)
{
    assert(aProvider);

    ProviderVec::iterator it = _providers.begin();
    for (; it != _providers.end(); ++it) {
      if (aProvider->priority() > (*it)->priority()) {
        _providers.insert(it, aProvider);
        return;
      }
    }
    
    // fell out of the iteration, goes to the end of the vec
    _providers.push_back(aProvider);
}

void ResourceManager::removeProvider(ResourceProvider* aProvider)
{
    assert(aProvider);
    auto it = std::find(_providers.begin(), _providers.end(), aProvider);
    if (it == _providers.end()) {
        SG_LOG(SG_GENERAL, SG_DEV_ALERT, "unknown provider doing remove");
        return;
    }
}

SGPath ResourceManager::findPath(const std::string& aResource, SGPath aContext)
{
    if (!aContext.isNull()) {
        SGPath r(aContext, aResource);
        if (r.exists()) {
            return r;
        }
    }
    
    for (auto provider : _providers) {
      SGPath path = provider->resolve(aResource, aContext);
      if (!path.isNull()) {
        return path;
      }
    }
    
    return SGPath();
}

} // of namespace simgear

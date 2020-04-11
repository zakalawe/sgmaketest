// Copyright (C) 2018  Fernando García Liñán <fernandogarcialinan@gmail.com>
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
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA

#include <simgear/props/condition.hxx>
#include <simgear/props/props.hxx>
#include <simgear/scene/tgdb/userdata.hxx>

#include "CompositorUtil.hxx"

namespace simgear {
namespace compositor {

bool
checkConditional(const SGPropertyNode *node)
{
    const SGPropertyNode *p_condition = node->getChild("condition");
    if (!p_condition)
        return true;
    SGSharedPtr<SGCondition> condition =
        sgReadCondition(getPropertyRoot(), p_condition);
    return !condition || condition->test();
}

const SGPropertyNode *
getPropertyNode(const SGPropertyNode *prop)
{
    if (!prop)
        return 0;
    if (prop->nChildren() > 0) {
        const SGPropertyNode *propertyProp = prop->getChild("property");
        if (!propertyProp)
            return prop;
        return getPropertyRoot()->getNode(propertyProp->getStringValue());
    }
    return prop;
}

const SGPropertyNode *
getPropertyChild(const SGPropertyNode *prop,
                 const char *name)
{
    const SGPropertyNode *child = prop->getChild(name);
    if (!child)
        return 0;
    return getPropertyNode(child);
}

} // namespace compositor
} // namespace simgear

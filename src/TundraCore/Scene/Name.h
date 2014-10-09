/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   Name.h
    @brief  Name provides network-synchronizable means of identification for entities in addition to the plain ID number. */

#pragma once

#include "TundraCoreApi.h"
#include "IComponent.h"

namespace Tundra
{

/// Allows identifying the Entity using a name string in addition to the Entity ID number.
/** <table class="header">
    <tr>
    <td>
    <h2>Name</h2>
    Provides network-synchronizable means of identification for entities in addition to the plain ID number.
    This EC is not present by default for entities.

    Registered by TundraLogicModule.

    <b>Attributes</b>:
    <ul>
    <li> String: name
    <div> @copydoc name</div>
    <li> String: description
    <div>@copydoc description </div>
    <li> String: group
    <div>@copydoc group </div>
    </ul>

    Does not emit any actions.

    </table> */
class TUNDRACORE_API Name : public IComponent
{
    COMPONENT_NAME(Name, 26)

public:
    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Name(Urho3D::Context* context, Scene* scene) :
        IComponent(context, scene),
        INIT_ATTRIBUTE_VALUE(name, "Name", ""),
        INIT_ATTRIBUTE_VALUE(description, "Description", ""),
        INIT_ATTRIBUTE_VALUE(group, "Group", "")
    {}
    /// @endcond

    ~Name() {}

    /// Specifies an arbitrary name that can be used for identification.
    /** @note Name is not enforced to be unique in any way, and multiple entities can have the same name.
        @sa Entity::SetName Entity::Name, Scene::EntityByName, Scene::FindEntities, Scene::FindEntitiesContaining */
    Attribute<String> name;

    /// Specifies an arbitrary description.
    /** @sa Entity::SetDescription, Entity::Description */
    Attribute<String> description;

    /// Specifies an arbitrary group identifier that can be used to group entities logically.
    /** @note Grouping does not enforce any kind of behavior; it's up to the client module
        implementations how the group information is used.
        @sa Entity::SetGroup, Entity::Group, Scene::EntitiesOfGroup */
    Attribute<String> group;
};

COMPONENT_TYPEDEFS(Name)

}
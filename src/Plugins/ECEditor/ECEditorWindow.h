/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Tundra
{

class Entity;
typedef SharedPtr<Entity> EntityPtr;

class ECEDITOR_API ECEditorWindow : public Object
{
    URHO3D_OBJECT(ECEditorWindow, Object);

public:
    ECEditorWindow(Framework *framework);
    virtual ~ECEditorWindow();

    void AddEntity(entity_id_t id, bool updateUi = true);
    void RemoveEntity(entity_id_t id, bool updateUi = true);
    void Clear();
    void Refresh();

protected:
    Framework *framework_;
};

}

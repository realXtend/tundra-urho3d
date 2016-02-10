/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ECEditor.h
    @brief  ECEditor core API. */

#pragma once

#include "IModule.h"
#include "SceneFwd.h"
#include "ECEditorApi.h"
#include "CoreStringUtils.h"
#include "UserConnectedResponseData.h"

namespace Tundra
{

class SceneStructureWindow;
typedef SharedPtr<SceneStructureWindow> SceneWindowPtr;

class ECEDITOR_API ECEditor : public IModule
{
    URHO3D_OBJECT(ECEditor, IModule);

public:
    ECEditor(Framework* owner);
    ~ECEditor();

private:
    void Initialize() override;
    void Uninitialize() override;

    SceneWindowPtr sceneEditor;
};

}

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
#include "IAttribute.h"

namespace Tundra
{

class SceneStructureWindow;
class ECEditorWindow;
typedef SharedPtr<SceneStructureWindow> SceneWindowPtr;
typedef SharedPtr<ECEditorWindow> EditorWindowPtr;

class ECEDITOR_API ECEditor : public IModule
{
    URHO3D_OBJECT(ECEditor, IModule);

public:
    ECEditor(Framework* owner);
    ~ECEditor();

    void Initialize() override;

    void OpenSceneEditor();
    void OpenSceneEditor(Scene *scene);
    void OpenEntityEditor(Entity *entity);

private:
    void OnSceneCreated(Scene *scene, AttributeChange::Type type);

    SceneWindowPtr sceneEditor_;
    EditorWindowPtr entityEditor_;
};

}

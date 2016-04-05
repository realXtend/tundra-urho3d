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
class MenuBarItem;
class OpenSceneDialog;
class FileDialog;

typedef SharedPtr<SceneStructureWindow> SceneWindowPtr;
typedef SharedPtr<ECEditorWindow> EditorWindowPtr;
typedef WeakPtr<MenuBarItem> MenuBarItemWeakPtr;
typedef SharedPtr<FileDialog> FileDialogPtr;

class ECEDITOR_API ECEditor : public IModule
{
    URHO3D_OBJECT(ECEditor, IModule);

public:
    ECEditor(Framework* owner);
    ~ECEditor();

    void Initialize() override;
    void Update(float UNUSED_PARAM(frametime)) override;

    /// Open scene editor and use default camera scene.
    void OpenSceneEditor();

    /// Open scene editor
    void OpenSceneEditor(Scene *scene);

    /// Open entity editor for given entity.
    void OpenEntityEditor(Entity *entity);

    /// Open entity editor for given entity and scroll to given compoent.
    void OpenEntityEditor(Entity *entity, IComponent *component);

private:
    void OpenFileDialogWindow(const String &title);

    void OnOpenSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file);
    void OnNewSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file);
    void OnSaveSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file);

    void CreateNewScene(const String &directory, const String &file);
    void OpenScene(const String &directory, const String &file);
    void SaveSceneAs(const String &directory, const String &file);

    void OnBarMenuSelected(MenuBarItem *item);
    void OnSceneCreated(Scene *scene, AttributeChange::Type type);

    SceneWindowPtr sceneEditor_;
    EditorWindowPtr entityEditor_;
    FileDialogPtr fileBrowser_;

    MenuBarItemWeakPtr sceneEditorItem_;
    MenuBarItemWeakPtr openSceneItem_;
    MenuBarItemWeakPtr saveSceneItem_;
    MenuBarItemWeakPtr newSceneItem_;
};

}

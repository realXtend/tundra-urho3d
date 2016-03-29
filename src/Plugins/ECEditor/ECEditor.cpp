// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "ECEditor.h"
#include "Framework.h"
#include "SceneStructureWindow.h"
#include "ECEditorWindow.h"
#include "FileDialog.h"
#include "AssetAPI.h"
#include "UI/UiAPI.h"
#include "UI/MenuBar.h"
#include "UI/MenuBarItem.h"

#include "SceneAPI.h"
#include "Scene.h"

#include "LoggingFunctions.h"

#include <Urho3D/UI/UI.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/FileSelector.h>

namespace Tundra
{
ECEditor::ECEditor(Framework* owner) :
    IModule("LoginScreen", owner),
    entityEditor_(0), sceneEditor_(0),
    sceneEditorItem_(0), openSceneItem_(0),
    newSceneItem_(0), fileBrowser_(0),
    saveSceneItem_(0)
{
    
}

ECEditor::~ECEditor()
{
    entityEditor_.Reset();
    sceneEditor_.Reset();
}

void ECEditor::Initialize()
{
    entityEditor_ = new ECEditorWindow(GetFramework());
    entityEditor_->Hide();

    sceneEditor_ = new SceneStructureWindow(GetFramework(), this);
    sceneEditor_->Hide();

    framework->Scene()->SceneCreated.Connect(this, &ECEditor::OnSceneCreated);

    MenuBar *menu = framework->Ui()->GetMenuBar();
    if (menu != NULL)
    {
        newSceneItem_ = menu->CreateMenuItem("File/New Scene...");
        newSceneItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);

        openSceneItem_ = menu->CreateMenuItem("File/Open Scene...");
        openSceneItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);

        saveSceneItem_ = menu->CreateMenuItem("File/Save Scene...");
        saveSceneItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);

        sceneEditorItem_ = menu->CreateMenuItem("Edit/Scene Editor");
        sceneEditorItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);
    }

    // Enable system clipboard so we can copy & paste entities in editor.
    GetSubsystem<UI>()->SetUseSystemClipboard(true);
}

void ECEditor::Update(float UNUSED_PARAM(frametime))
{
    if (sceneEditor_.Get())
        sceneEditor_->Update();
}

void ECEditor::OpenSceneEditor()
{
    SceneMap scenes = GetFramework()->Scene()->Scenes();
    if (scenes.Values().Size() > 0)
        sceneEditor_->SetShownScene(scenes.Values()[0]);
}

void ECEditor::OpenSceneEditor(Scene *scene)
{
    if (sceneEditor_.Get())
        sceneEditor_->SetShownScene(scene);
}

void ECEditor::OpenEntityEditor(Entity *entity)
{
    if (entityEditor_.Get())
    {
        entityEditor_->AddEntity(entity->Id(), true);
        entityEditor_->Show();
    }
}

void ECEditor::OpenFileDialogWindow(const String &action)
{
    if (fileBrowser_.Get())
    {
        fileBrowser_.Reset();
    }

    StringVector filters;
    filters.Push("*.txml");

    fileBrowser_ = FileDialog::Open(framework);
    fileBrowser_->SetFilters(filters);

    if (action == "OpenScene")
    {
        fileBrowser_->SetTitle("Open Scene...");
        fileBrowser_->OnDialogClosed.Connect(this, &ECEditor::OnOpenSceneDialogClosed);
    }
    else if (action == "NewScene")
    {
        fileBrowser_->SetTitle("Create new Scene...");
        fileBrowser_->OnDialogClosed.Connect(this, &ECEditor::OnNewSceneDialogClosed);
    }
    else if (action == "SaveScene")
    {
        fileBrowser_->SetTitle("Save Scene as...");
        fileBrowser_->OnDialogClosed.Connect(this, &ECEditor::OnSaveSceneDialogClosed);
    }
}

void ECEditor::OnOpenSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file)
{
    if (!confirmed)
        return;

    OpenScene(directory, file);
}

void ECEditor::OnNewSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file)
{
    if (!confirmed)
        return;

    CreateNewScene(directory, file);
}

void ECEditor::OnSaveSceneDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file)
{
    if (!confirmed)
        return;

    SaveSceneAs(directory, file);
}

void ECEditor::CreateNewScene(const String &directory, const String &file)
{
    /// @todo validate file path and confirm the action.

    ScenePtr scene = framework->Scene()->SceneByName("TundraServer");
    if (scene.Get())
        framework->Scene()->RemoveScene("TundraServer");

    ScenePtr newScene = framework->Scene()->CreateScene("TundraServer", true, true);
    if (newScene == NULL)
        return;

    String fullFilePath = directory + file;
    AssetStoragePtr storage = framework->Asset()->DeserializeAssetStorageFromString(fullFilePath, false);
    if (storage == NULL)
    {
       LogError("Failed to create asset storage for file " + fullFilePath);
        return;
    }
    framework->Asset()->SetDefaultAssetStorage(storage);

    AssetStorageVector storages = framework->Asset()->AssetStorages();
    for (uint i = 0; i < storages.Size(); ++i)
    {
        if (storages[i].Get() && storages[i] != storage)
            framework->Asset()->RemoveAssetStorage(storages[i]->Name());
    }

    newScene->SaveSceneXML(fullFilePath, false, true);
}

void ECEditor::OpenScene(const String &directory, const String &file)
{
    /// @todo validate file path and confirm the action.

    String fullFilePath = directory + file;

    ScenePtr scene = framework->Scene()->SceneByName("TundraServer");
    if (!scene.Get())
    {
        scene = framework->Scene()->CreateScene("TundraServer", true, true);
        if (!scene.Get())
            return;
    }

    AssetStoragePtr storage = framework->Asset()->DeserializeAssetStorageFromString(fullFilePath, false);
    if (storage.Get())
        framework->Asset()->SetDefaultAssetStorage(storage);
    else
    {
        LogError("Failed to create asset storage for file " + fullFilePath);
        return;
    }

    AssetStorageVector storages = framework->Asset()->AssetStorages();
    for (uint i = 0; i < storages.Size(); ++i)
    {
        if (storages[i].Get() && storages[i] != storage)
            framework->Asset()->RemoveAssetStorage(storages[i]->Name());
    }

    framework->Asset()->SetDefaultAssetStorage(storage);
    scene->LoadSceneXML(fullFilePath, true, false, AttributeChange::Default);
}

void ECEditor::SaveSceneAs(const String &directory, const String &file)
{
    ScenePtr scene = ScenePtr(framework->Scene()->MainCameraScene());
    if (!scene.Get())
    {
        SceneMap scenes = framework->Scene()->Scenes();
        if (scenes.Size())
            return;

        // Check if any scene exists if main camera scene is null.
        scene = scenes.Values()[0];
        if (!scene.Get())
            return;
    }

    String fullFilePath = directory + file;
    scene->SaveSceneXML(fullFilePath, false, true);
}

void ECEditor::OnBarMenuSelected(MenuBarItem *item)
{
    if (sceneEditorItem_ == item)
        sceneEditor_->Show();
    else if (openSceneItem_ == item)
        OpenFileDialogWindow("OpenScene");
    else if (newSceneItem_ == item)
        OpenFileDialogWindow("NewScene");
    else if (saveSceneItem_ == item)
        OpenFileDialogWindow("SaveScene");;
}

void ECEditor::OnSceneCreated(Scene *scene, AttributeChange::Type /*type*/)
{
    OpenSceneEditor(scene);
}
}

extern "C"
{

    DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
    {
        fw->RegisterModule(new Tundra::ECEditor(fw));
    }

}
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
#include "Sky.h"

#include "SceneAPI.h"
#include "Scene.h"

#include "LoggingFunctions.h"

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/FileSelector.h>

namespace Tundra
{
ECEditor::ECEditor(Framework* owner) :
    IModule("LoginScreen", owner),
    entityEditor_(0), sceneEditor_(0),
    sceneEditorItem_(0), openSceneItem_(0),
    newSceneItem_(0), openSceneDialog_(0),
    newSceneDialog_(0)
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

    StringVector filters;
    filters.Push("*.txml");

    openSceneDialog_ = new FileDialog(GetFramework());
    openSceneDialog_->SetFilters(filters);
    openSceneDialog_->OnDialogClosed.Connect(this, &ECEditor::OnFileDialogClosed);

    newSceneDialog_ = new FileDialog(GetFramework());
    newSceneDialog_->SetFilters(filters);
    newSceneDialog_->OnDialogClosed.Connect(this, &ECEditor::OnFileDialogClosed);

    framework->Scene()->SceneCreated.Connect(this, &ECEditor::OnSceneCreated);

    MenuBar *menu = framework->Ui()->GetMenuBar();
    if (menu != NULL)
    {
        newSceneItem_ = menu->CreateMenuItem("File/New Scene...");
        newSceneItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);

        openSceneItem_ = menu->CreateMenuItem("File/Open Scene...");
        openSceneItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);

        sceneEditorItem_ = menu->CreateMenuItem("Edit/Scene Editor");
        sceneEditorItem_->OnItemPressed.Connect(this, &ECEditor::OnBarMenuSelected);
    }
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

void ECEditor::OpenSceneDialogWindow()
{
    if (openSceneDialog_.Get())
        openSceneDialog_->Open();
}

void ECEditor::NewSceneDialogWindow()
{
    if (newSceneDialog_.Get())
        newSceneDialog_->Open();
}

void ECEditor::OnFileDialogClosed(FileDialog *dialog, bool confirmed, const String &directory, const String &file)
{
    if (!confirmed)
        return;

    if (openSceneDialog_.Get() == dialog)
        OpenScene(directory, file);
    else if (newSceneDialog_.Get() == dialog)
        CreateNewScene(directory, file);
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

void ECEditor::OnBarMenuSelected(MenuBarItem *item)
{
    if (sceneEditorItem_ == item)
        sceneEditor_->Show();
    else if (openSceneItem_ == item)
        OpenSceneDialogWindow();
    else if (newSceneItem_ == item)
        NewSceneDialogWindow();
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
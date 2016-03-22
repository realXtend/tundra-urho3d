// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "FileDialog.h"
#include "Framework.h"
#include "SceneAPI.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/FileSelector.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Window.h>

namespace Tundra
{

FileDialog::FileDialog(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework),
    fileSelector_(0)
{
    
}

FileDialog::~FileDialog()
{
    Release();
}

Urho3D::FileSelector *FileDialog::FileSelector() const
{
    return fileSelector_;
}

void FileDialog::SetFilters(const StringVector &filters)
{
    filters_ = filters;
}

void FileDialog::Open()
{
    if (!fileSelector_.Get())
    {
        Initialize();
        
        // Center dialog
        Urho3D::UIElement *root = GetSubsystem<Urho3D::UI>()->GetRoot();
        Urho3D::IntVector2 size = fileSelector_->GetWindow()->GetSize();
        fileSelector_->GetWindow()->SetPosition(Urho3D::IntVector2(root->GetWidth() * 0.5 - size.x_ * 0.5, root->GetHeight() * 0.5 - size.y_ * 0.5));
    }
}

void FileDialog::Initialize()
{
    Urho3D::XMLFile *style = context_->GetSubsystem<Urho3D::ResourceCache>()->GetResource<Urho3D::XMLFile>("Data/UI/DefaultStyle.xml");

    fileSelector_ = new Urho3D::FileSelector(framework_->GetContext());
    fileSelector_->SetDefaultStyle(style);
    fileSelector_->SetTitle("Open Scene");
    fileSelector_->SetButtonTexts("Ok", "Cancel");
    fileSelector_->SetFilters(filters_, 0);

    SubscribeToEvent(fileSelector_->GetCloseButton(), Urho3D::E_RELEASED, URHO3D_HANDLER(FileDialog, OnButtonPressed));
    SubscribeToEvent(fileSelector_->GetCancelButton(), Urho3D::E_RELEASED, URHO3D_HANDLER(FileDialog, OnButtonPressed));
    SubscribeToEvent(fileSelector_->GetOKButton(), Urho3D::E_RELEASED, URHO3D_HANDLER(FileDialog, OnButtonPressed));
}

void FileDialog::Release()
{
    if (fileSelector_.Get())
        fileSelector_.Reset();
}

void FileDialog::OnButtonPressed(StringHash /*eventType*/, VariantMap &eventData)
{
    Urho3D::UIElement *button = dynamic_cast<Urho3D::UIElement*>(eventData["Element"].GetPtr());
    
    String fileName = fileSelector_->GetFileName();

    /// If no file name is defined ignore selection.
    if (fileName.Length() == 0 || button == fileSelector_->GetCloseButton() || button == fileSelector_->GetCancelButton())
        OnDialogClosed.Emit(this, false, "", "");
    else if (button == fileSelector_->GetOKButton())
        OnDialogClosed.Emit(this, true, fileSelector_->GetPath(), fileName);

    Release();
}

}
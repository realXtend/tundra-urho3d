/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   OpenSceneDialog.h
    @brief  OpenSceneDialog. */

#pragma once

#include "ECEditorApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"

#include <Urho3D/Core/Object.h>
#include <Urho3D/Container/Ptr.h>

namespace Urho3D
{
class FileSelector;
}

namespace Tundra
{

class FileDialog;
typedef SharedPtr<Urho3D::FileSelector> FileSelectorPtr;
typedef SharedPtr<FileDialog> FileDialogPtr;

class ECEDITOR_API FileDialog : public Object
{
    URHO3D_OBJECT(FileDialog, Object);

public:
    explicit FileDialog(Framework *framework);
    virtual ~FileDialog();

    /// Get Urho FileSelector UI object.
    Urho3D::FileSelector *FileSelector() const;

    /// Set fileters for file dialog e.g. "*.txml"
    void SetFilters(const StringVector &filters);
    
    /// Set File Dialog title text
    void SetTitle(const String &title);

    /// Get dialog title text
    String Title() const;

    /// Initialize instance of file dialog window and display it on screen.
    static FileDialogPtr Open(Framework *framework);

    /// Triggered when dialog window is closed
    Signal4<FileDialog *ARG(dialog), bool ARG(confirmed), const String &ARG(path), const String &ARG(file)> OnDialogClosed;

protected:
    void Release();
    void OnButtonPressed(StringHash eventType, VariantMap &eventData);

    Framework *framework_;
    FileSelectorPtr fileSelector_;
    StringVector filters_;
};

}

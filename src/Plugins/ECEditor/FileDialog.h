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

typedef SharedPtr<Urho3D::FileSelector> FileSelectorPtr;

class ECEDITOR_API FileDialog : public Object
{
    URHO3D_OBJECT(FileDialog, Object);

public:
    explicit FileDialog(Framework *framework);
    virtual ~FileDialog();

    Urho3D::FileSelector *FileSelector() const;
    void SetFilters(const StringVector &filters);
    void Open();

    Signal4<FileDialog *ARG(dialog), bool ARG(confirmed), const String &ARG(path), const String &ARG(file)> OnDialogClosed;

protected:
    void Initialize();
    void Release();

    void OnButtonPressed(StringHash eventType, VariantMap &eventData);

    Framework *framework_;
    FileSelectorPtr fileSelector_;
    StringVector filters_;
};

}

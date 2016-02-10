// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"

#include "ECEditorWindow.h"
#include "Framework.h"

#include <Urho3D/Core/Context.h>
#include <Urho3D/Engine/Engine.h>

namespace Tundra
{

ECEditorWindow::ECEditorWindow(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
        
}

ECEditorWindow::~ECEditorWindow()
{
    
}

void ECEditorWindow::AddEntity(entity_id_t id, bool updateUi)
{

}

void ECEditorWindow::RemoveEntity(entity_id_t id, bool updateUi)
{

}

void ECEditorWindow::Clear()
{

}

void ECEditorWindow::Refresh()
{

}

}
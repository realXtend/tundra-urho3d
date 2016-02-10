// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "ECEditor.h"
#include "Framework.h"
#include "SceneStructureWindow.h"

#include "LoggingFunctions.h"

namespace Tundra
{

    ECEditor::ECEditor(Framework* owner) :
        IModule("LoginScreen", owner)
    {
        
    }

    ECEditor::~ECEditor()
    {
        sceneEditor.Reset();
    }

    void ECEditor::Initialize()
    {
        sceneEditor = new SceneStructureWindow(GetFramework());
    }

    void ECEditor::Uninitialize()
    {

    }

}

extern "C"
{

    DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
    {
        fw->RegisterModule(new Tundra::ECEditor(fw));
    }

}
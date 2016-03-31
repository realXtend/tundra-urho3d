// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Win.h"
#include "AvatarApplication.h"
#include "Avatar.h"
#include "GenericAssetFactory.h"
#include "IComponentFactory.h"
#include "Framework.h"
#include "Placeable.h"
#include "AssetAPI.h"
#include "AvatarDescAsset.h"
#include "SceneAPI.h"
#include "LoggingFunctions.h"

#include "JavaScript.h"
#include "JavaScriptInstance.h"
#include "AvatarApplicationBindings/AvatarApplicationBindings.h"

#include <Urho3D/Core/Profiler.h>

using namespace JSBindings;

namespace Tundra
{

AvatarApplication::AvatarApplication(Framework* owner) :
    IModule("AvatarApplication", owner)
{
}

AvatarApplication::~AvatarApplication()
{
}

void AvatarApplication::Load()
{
    SceneAPI* scene = framework->Scene();
    scene->RegisterComponentFactory(ComponentFactoryPtr(new GenericComponentFactory<Avatar>()));

    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new GenericAssetFactory<AvatarDescAsset>("Avatar", ".avatar")));
    framework->Asset()->RegisterAssetTypeFactory(AssetTypeFactoryPtr(new BinaryAssetFactory("AvatarAttachment", ".attachment")));
}

void AvatarApplication::Initialize()
{
    // Connect to JavaScript module instance creation to be able to expose the physics classes to each instance
    JavaScript* javaScript = framework->Module<JavaScript>();
    if (javaScript)
        javaScript->ScriptInstanceCreated.Connect(this, &AvatarApplication::OnScriptInstanceCreated);
}

void AvatarApplication::Uninitialize()
{
}

void AvatarApplication::OnScriptInstanceCreated(JavaScriptInstance* instance)
{
    URHO3D_PROFILE(ExposeAvatarApplicationClasses);

    duk_context* ctx = instance->Context();
    ExposeAvatarApplicationClasses(ctx);
}

}

extern "C"
{

DLLEXPORT void TundraPluginMain(Tundra::Framework *fw)
{
    fw->RegisterModule(new Tundra::AvatarApplication(fw));
}

}

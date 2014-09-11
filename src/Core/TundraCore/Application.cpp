// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "TundraCoreApi.h"

#include <Math/float3.h>
#include <Context.h>
#include <Engine.h>
#include <FileSystem.h>

using namespace math;

int TUNDRACORE_API run(int argc, char **argv)
{
	// Very simple test code to demonstrate successful use of Urho3D and MathGeoLib. To be moved elsewhere/removed.
	float3 test;
	test.x = 0;
	test.Normalize();

	Urho3D::SharedPtr<Urho3D::Context> context(new Urho3D::Context());
	Urho3D::SharedPtr<Urho3D::Engine> engine(new Urho3D::Engine(context));
	Urho3D::VariantMap engineInitMap;
	engineInitMap["ResourcePaths"] = context->GetSubsystem<Urho3D::FileSystem>()->GetProgramDir() + "Data";
	engineInitMap["AutoloadPaths"] = "";
	engineInitMap["FullScreen"] = false;
	engineInitMap["WindowTitle"] = "Tundra";
	engineInitMap["LogName"] = "Tundra.log";
	engine->Initialize(engineInitMap);
	while (!engine->IsExiting())
	{
		engine->RunFrame();
	}

	return 0;
}

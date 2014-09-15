// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "Framework.h"

#include <Context.h>
#include <Engine.h>
#include <FileSystem.h>

namespace Tundra
{

int run(int argc, char** argv)
{
	Framework fw;
	fw.Go();
	return 0;
}

Framework* Framework::instance = 0;

Framework::Framework()
{
	instance = this;

	// Create the Urho3D context and engine, which creates various other subsystems,
	// but does not initialize them yet
	context = new Urho3D::Context();
	engine = new Urho3D::Engine(context);
}

Framework::~Framework()
{
	instance = 0;
}

void Framework::Go()
{
	// Initialize the Urho3D engine now
	Urho3D::VariantMap engineInitMap;
	engineInitMap["ResourcePaths"] = context->GetSubsystem<Urho3D::FileSystem>()->GetProgramDir() + "Data";
	engineInitMap["AutoloadPaths"] = "";
	engineInitMap["FullScreen"] = false;
	engineInitMap["WindowTitle"] = "Tundra";
	engineInitMap["LogName"] = "Tundra.log";
	engine->Initialize(engineInitMap);

	// Run mainloop
	while (!engine->IsExiting())
		engine->RunFrame();
}

Urho3D::Context* Framework::Context() const
{
	return context;
}

Urho3D::Engine* Framework::Engine() const
{
	return engine;
}

}
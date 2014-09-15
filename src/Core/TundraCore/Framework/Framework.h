// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "FrameworkFwd.h"

#include <Ptr.h>

namespace Tundra
{

/// The system root access object.
class TUNDRACORE_API Framework
{
public:
	Framework();
	~Framework();

	/// Run the main loop until exit requested.
	void Go();

	/// Return the Urho3D Context object, which is used to access Urho3D subsystems.
	Urho3D::Context* Context() const;
	/// Return the Urho3D Engine object.
	Urho3D::Engine* Engine() const;
	/// Return the singleton instance of the Framework. Should *only* be used by plugins on initialization!
	static Framework* Instance() { return instance; }

private:
	/// Urho3D context.
	Urho3D::SharedPtr<Urho3D::Context> context;
	/// Urho3D engine
	Urho3D::SharedPtr<Urho3D::Engine> engine;

	/// Singleton instance of the Framework
	static Framework* instance;
};

/// Instantiate the Framework and run until exited.
TUNDRACORE_API int run(int argc, char** argv);

}
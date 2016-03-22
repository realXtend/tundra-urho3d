// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "IModule.h"
#include "JavaScriptApi.h"
#include "JavaScriptFwd.h"
#include "Signals.h"
#include "Scene.h"

namespace Tundra
{

class JavaScriptInstance;
class Script;

/// JavaScript scripting module using the Duktape VM
class JAVASCRIPT_API JavaScript : public IModule
{
    URHO3D_OBJECT(JavaScript, IModule);

public:
    JavaScript(Framework* owner);
    ~JavaScript();

    /// Emitted when a new JavaScript engine instance has been created. Use this to expose more classes to the instance.
    Signal1<JavaScriptInstance*> ScriptInstanceCreated;

    /// Prepare a script engine by registering the API and service objects.
    void PrepareScriptInstance(JavaScriptInstance* instance, Script* scriptComp);

private:
    void Load() override;
    void Initialize() override;
    void Uninitialize() override;

    void OnSceneCreated(Scene *scene, AttributeChange::Type);
    void OnComponentAdded(Entity* entity, IComponent* comp, AttributeChange::Type change);
    void OnComponentRemoved(Entity* entity, IComponent* comp, AttributeChange::Type change);
    void OnScriptAssetsChanged(Script* scriptComp, const Vector<ScriptAssetPtr>& newScripts);
    void OnScriptClassNameChanged(Script* scriptComp, const String& newClassName);
    void OnScriptEvaluated(JavaScriptInstance* instance);
    void OnScriptUnloading(JavaScriptInstance* instance);

    Script* FindScriptApplication(Script* instance, const String& appName);
    void ParseAppAndClassName(Script* instance, String& appName, String& className);
    void CreateScriptObject(Script* app, Script* instance, const String& className);
    void RemoveScriptObject(Script* instance);
    void CreateScriptObjects(Script* app);
    void RemoveScriptObjects(JavaScriptInstance* jsInstance);
};

}

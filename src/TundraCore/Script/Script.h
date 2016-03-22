// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"
#include "IComponent.h"
#include "AssetReference.h"
#include "AssetFwd.h"

namespace Tundra
{

class IScriptInstance;
class ScriptAsset;
typedef SharedPtr<ScriptAsset> ScriptAssetPtr;

/// Provides mechanism for adding scripts to entities.
/** <table class="header">
    <tr>
    <td>
    <h2>Script</h2>
    Provides mechanism for adding scripts to entities.

    When Script has scriptRef(s) set, it will load and run the script assets in an own script engine. Optionally,
    it can define the name of the script application.

    Script's can also create script objects within an existing script application. In this case, the scriptRef
    is left empty, and instead the className attribute tells from which script application to instantiate a specific class.
    The syntax for className attribute is ApplicationName.ClassName . The Script with the matching application will be
    looked up, and the constructor function ClassName will be called, with the entity and component as parameters.
    If the className changes, the previously created script object will be automatically destroyed. Script objects
    may optionally define a destructor called OnScriptObjectDestroyed

    <b>Attributes</b>:
    <ul>
    <li>AssetReferenceList: scriptRef
    <div> @copydoc scriptRef </div>
    <li>bool: runOnLoad
    <div> @copydoc runOnLoad </div>
    <li>enum: runMode
    <div> @copydoc runMode </div>
    <li>QString: applicationName
    <div>@copydoc applicationName </div>
    <li>QString: className
    <div>@copydoc className </div>
    </ul>

    <b>Reacts on the following actions:</b>
    <ul>
    <li> "RunScript": Runs the script. Usage: RunScript [componentName]
    <li> "UnloadScript": Stops and unloads the script. Usage: UnloadScript [componentName]
    </ul>
    </td>
    </tr>

    Does not emit any actions.

    <b>Doesn't depend on any other entity-component</b>.
    </table> */
class TUNDRACORE_API Script: public IComponent
{
    COMPONENT_NAME(Script, 5)

public:
    /// Run mode enumeration
    enum RunMode
    {
        RunOnBoth = 0,
        RunOnClient,
        RunOnServer
    };

    /// @cond PRIVATE
    /// Do not directly allocate new components using operator new, but use the factory-based SceneAPI::CreateComponent functions instead.
    explicit Script(Urho3D::Context* context, Scene* scene);
    /// @endcond
    ~Script();

    /// The script assets that will be loaded. If empty, no script engine will be created
    Attribute<AssetReferenceList> scriptRef;

    /// Is the script engine run as soon as the script asset(s) are set/loaded
    Attribute<bool> runOnLoad;

    /// Whether to run on client, server or both
    Attribute<int> runMode;
    
    /// Name for the script application
    Attribute<String> applicationName;
    
    /// The script class to instantiate from within an existing application. Syntax: applicationName.className
    Attribute<String> className;
    
    /// Sets new script instance.
    /** Unloads and deletes possible already existing script instance.
        @param instance Script instance.
        @note Takes ownership of the script instance. */
    void SetScriptInstance(IScriptInstance *instance);

    /// Returns the current script instance.
    IScriptInstance *ScriptInstance() const { return scriptInstance_; }

    /// Set the application script component this script component has a script object created from
    void SetScriptApplication(Script* app);
    
    /// Return the script application component, if it (still) exists
    Script* ScriptApplication() const;

    /// Set the IsServer and IsClient flags. Called by the parent scripting system, which has this knowledge
    void SetIsClientIsServer(bool isClient, bool isServer);

    /// Runs the script instance.
    /** @param name Name of the script component, optional. The script will be run only if the component name matches.*/
    void Run(const String& name = String());

    /// Stops and unloads the script.
    /** @param name Name(s) of the script(s), optional. The script is unloaded only if the script name matches. */
    void Unload(const String& name = String());

    /// Check whether the script should run
    bool ShouldRun() const;

    /// Emitted when changed script assets are ready to run
    Signal2<Script*, const Vector<ScriptAssetPtr>&> ScriptAssetsChanged;

    /// Emitted when the script application name has changed
    Signal2<Script*, const String&> ApplicationNameChanged;

    /// Emitted when the script class name has changed
    Signal2<Script*, const String&> ClassNameChanged;

private:
    /// Handles logic regarding attribute changes of this EC.
    /** @param attribute Attribute that changed.
        @param change Change type. */
    void HandleAttributeChanged(IAttribute* attribute, AttributeChange::Type change);

    /// Called when a script asset has been loaded.
    void OnScriptAssetLoaded(uint index, AssetPtr asset);

    /// Registers the actions this component provides when parent entity is set.
    void RegisterActions();

    /// Handles the downloading of script assets.
    AssetRefListListenerPtr scriptAssets;

    /// Script instance.
    SharedPtr<IScriptInstance> scriptInstance_;
    
    /// The parent script application, if an object has been instantiated from inside it
    ComponentWeakPtr scriptApplication_;
    
    /// Handle the Run action
    void RunTriggered(const StringVector& params);
    /// Handle the Unload action
    void UnloadTriggered(const StringVector& params);

    /// IsClient flag, for checking run mode
    bool isClient_;
    /// IsServer flag, for checking run mode
    bool isServer_;
};

COMPONENT_TYPEDEFS(Script)

}


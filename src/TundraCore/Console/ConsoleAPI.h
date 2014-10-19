/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ConsoleAPI.h
    @brief  Console core API. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"
#include "FrameworkFwd.h"
#include "Signals.h"

#include <Object.h>
#include <RefCounted.h>

namespace Tundra
{

class TUNDRACORE_API ConsoleCommand : public RefCounted
{
public:
    ConsoleCommand(const String &name, const String &description) :
        name_(name),
        description_(description)
    {
    }

    /// Listen for execution with parameters.
    Signal1<const StringVector&> ExecutedWith;
    /// Listen for executions without parameters.
    Signal0<void> Executed;

    /// Returns the name of this command.
    const String &Name() const { return name_; }

    /// Returns the description of this command.
    const String &Description() const { return description_; }

    void Invoke(const StringVector &parameters)
    {
        Executed.Emit();
        ExecutedWith.Emit(parameters);
    }

private:
    String name_;
    String description_;
};

/// Console core API.
/** Allows printing text to console, executing console commands programmatically 
    and registering new console commands.
    @note Console commands are case-insensitive. */
class TUNDRACORE_API ConsoleAPI : public Object
{
    OBJECT(ConsoleAPI);

public:
    typedef HashMap<String, SharedPtr<ConsoleCommand> > CommandMap;

    /// Returns all command for introspection purposes.
    const CommandMap &Commands() const { return commands_; }

    ConsoleCommand *Command(const String &name) const;

    /// Registers a new console command which invokes a member function on the specified receiver object.
    /** @param name The function name to use for this command.
        @param desc A help description of this command.
        @param receiver The object instance that will be invoked when this command is executed.
        @param memberFunc A function of the @c receiver that is to be called when this command is executed.
        @see UnregisterCommand */
    template<class X, class Y>
    void RegisterCommand(const String &name, const String &desc, Y *receiver, void (X::*memberFunc)())
    {
        ConsoleCommand *cmd = RegisterCommand(name, desc);
        if (cmd)
            cmd->Executed.Connect(receiver, memberFunc);
    }

    /// Registers a new console command which triggers a signal when executed.
    /** param name The function name to use for this command.
        @param desc A help description of this command.
        @return This function returns a pointer to the newly created ConsoleCommand data structure.
                Connect the Invoked() signal of this structure to your script slot/function.
        @note Never store the returned ConsoleCommand pointer to an internal variable. The ownership of this object is retained by ConsoleAPI.
        @see UnregisterCommand */
    ConsoleCommand *RegisterCommand(const String &name, const String &desc);

    /// Listen to command @name.
    //bool RegisterListener(const String &name, ConsoleCommand::CommandListener listener, ConsoleCommand::CommandListenerFunctionPtr handlerFunction);

    /// Unregister console command.
    /** @param name Name of the command. */
    void UnregisterCommand(const String &name);

    /// Executes a console command.
    /** @param command Console command, syntax: "command(param1, param2, param3, ...)". */
    void ExecuteCommand(const String &command);

    /// Lists all console commands and their descriptions to the log.
    /** This command is invoked by typing 'help' to the console. */
    void ListCommands();

    /// Returns names of the available console commands.
    /** @note The names are exactly the strings they were originally registered with.
        Always perform case-insensitive comparison when searching for a specific command. */
    StringVector AvailableCommands() const;

    /// Set UI console visible.
    void SetConsoleVisible(bool visible);

    /// Toggles UI console.
    void ToggleConsole();

    /// Clears UI console.
    void ClearConsole();

private:
    friend class Framework;

    /// Constructor. Framework takes ownership of this object.
    /** @param fw Framework */
    explicit ConsoleAPI(Framework *fw);
    ~ConsoleAPI();

    /// Framework invoked initialize.
    void Initialize();

    /// E_CONSOLECOMMAND handler
    void HandleConsoleCommand(StringHash eventType, Urho3D::VariantMap &eventData);

    /// Frame update handler
    void OnUpdate(float frametime);

    CommandMap::ConstIterator FindCaseInsensitive(const String &name) const;
    CommandMap::Iterator FindCaseInsensitive(const String &name);

    Framework *framework_;
    CommandMap commands_;
};

}

/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ConsoleAPI.cpp
    @brief  Console core API. */

#include "StableHeaders.h"

#include "ConsoleAPI.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "CoreStringUtils.h"

#include <CoreEvents.h>
#include <EngineEvents.h>
#include <ProcessUtils.h>
#include <Console.h>
#include <Log.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace Tundra
{

ConsoleAPI::ConsoleAPI(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework)
{
    SubscribeToEvent(Urho3D::E_CONSOLECOMMAND, HANDLER(ConsoleAPI, HandleConsoleCommand));

    RegisterCommand("help", "Lists all registered commands.")
        ->Executed.Connect(this, &ConsoleAPI::ListCommands);
}

void ConsoleAPI::Initialize()
{
    framework_->Frame()->Updated.Connect(this, &ConsoleAPI::OnUpdate);
}

ConsoleAPI::~ConsoleAPI()
{
    commands_.Clear();
}

ConsoleAPI::CommandMap::ConstIterator ConsoleAPI::FindCaseInsensitive(const String &name) const
{
    for (CommandMap::ConstIterator iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Compare(name, false) == 0)
            return iter;
    return commands_.End();
}

ConsoleAPI::CommandMap::Iterator ConsoleAPI::FindCaseInsensitive(const String &name)
{
    for (CommandMap::Iterator iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Compare(name, false) == 0)
            return iter;
    return commands_.End();
}

StringVector ConsoleAPI::AvailableCommands() const
{
    StringVector ret;
    for(CommandMap::ConstIterator iter = commands_.Begin(); iter != commands_.End(); ++iter)
        ret.Push(iter->first_);
    return ret;
}

ConsoleCommand *ConsoleAPI::Command(const String &name) const
{
    CommandMap::ConstIterator existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LOGWARNING("ConsoleAPI::RegisterCommand: Command '" + name + "' does not exist.");
        return 0;
    }
    return existing->second_.Get();
}

ConsoleCommand *ConsoleAPI::RegisterCommand(const String &name, const String &desc)
{
    if (name.Empty())
    {
        LOGERROR("ConsoleAPI::RegisterCommand: Command name can not be an empty string.");
        return 0;
    }
    CommandMap::Iterator existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LOGWARNING("ConsoleAPI::RegisterCommand: Command '" + name + "' is already registered.");
        return existing->second_.Get();
    }

    SharedPtr<ConsoleCommand> command = SharedPtr<ConsoleCommand>(new ConsoleCommand(name, desc));
    commands_[name] = command;
    return command.Get();
}

void ConsoleAPI::UnregisterCommand(const String &name)
{
    CommandMap::Iterator existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LOGWARNING("ConsoleAPI: Trying to unregister non-existing command '" + name + "'.");
        return;
    }
    commands_.Erase(existing);
}

void ConsoleAPI::ExecuteCommand(const String &command)
{
    String name;
    StringVector parameters;
    ParseCommand(command, name, parameters);
    if (name.Empty())
        return;
    CommandMap::Iterator existing = FindCaseInsensitive(name);
    if (existing == commands_.End())
    {
        LOGERROR("Cannot find a console command '" + name + "'");
        return;
    }
    existing->second_->Invoke(parameters);
}

void ParseCommand(String &command, StringVector &parameters)
{
}

void ConsoleAPI::ListCommands()
{
    LOGINFO("Available Console Commands (case-insensitive)");
    int longestName = 0;
    for(CommandMap::ConstIterator iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Length() > longestName)
            longestName = iter->first_.Length();
    longestName += 2;
    for(CommandMap::ConstIterator iter = commands_.Begin(); iter != commands_.End(); ++iter)
        LOGINFO("  " + PadString(iter->first_, longestName) + iter->second_->Description());
}

void ConsoleAPI::HandleConsoleCommand(StringHash eventType, Urho3D::VariantMap &eventData)
{
    ExecuteCommand(eventData[Urho3D::ConsoleCommand::P_COMMAND].GetString());
}

void ConsoleAPI::OnUpdate(float /*frametime*/)
{
    // Check if there is input from stdin
    String input = Urho3D::GetConsoleInput();
    if (input.Length())
        ExecuteCommand(input);
}

void SetConsoleVisible(bool visible)
{
    /// @todo GUI console
}

void ConsoleAPI::ToggleConsole()
{
    /// @todo GUI console
}

void ConsoleAPI::ClearConsole()
{
    /// @todo Clear GUI consiole when implemented

#ifdef WIN32
    // Check that native console exists. If not and we call cls, we see a console flashing briefly on the screen which is undesirable.
    if (::GetConsoleWindow())
        (void)system("cls");
#else
    (void)system("clear");
#endif
}

}

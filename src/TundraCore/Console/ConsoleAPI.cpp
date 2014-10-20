/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ConsoleAPI.cpp
    @brief  Console core API. */

#include "StableHeaders.h"

#include "ConsoleAPI.h"
#include "Framework.h"
#include "FrameAPI.h"
#include "CoreStringUtils.h"
#include "LoggingFunctions.h"

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
    framework_(framework),
    enabledLogChannels(LogLevelErrorWarnInfo)
{
    SubscribeToEvent(Urho3D::E_CONSOLECOMMAND, HANDLER(ConsoleAPI, HandleConsoleCommand));

    RegisterCommand("help", "Lists all registered commands.", this, &ConsoleAPI::ListCommands);
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
    for (auto iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Compare(name, false) == 0)
            return iter;
    return commands_.End();
}

ConsoleAPI::CommandMap::Iterator ConsoleAPI::FindCaseInsensitive(const String &name)
{
    for (auto iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Compare(name, false) == 0)
            return iter;
    return commands_.End();
}

StringVector ConsoleAPI::AvailableCommands() const
{
    StringVector ret;
    for(auto iter = commands_.Begin(); iter != commands_.End(); ++iter)
        ret.Push(iter->first_);
    return ret;
}

ConsoleCommand *ConsoleAPI::Command(const String &name) const
{
    auto existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LogWarning("ConsoleAPI::RegisterCommand: Command '" + name + "' does not exist.");
        return 0;
    }
    return existing->second_.Get();
}

ConsoleCommand *ConsoleAPI::RegisterCommand(const String &name, const String &desc)
{
    if (name.Empty())
    {
        LogError("ConsoleAPI::RegisterCommand: Command name can not be an empty string.");
        return 0;
    }
    auto existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LogWarning("ConsoleAPI::RegisterCommand: Command '" + name + "' is already registered.");
        return existing->second_.Get();
    }

    SharedPtr<ConsoleCommand> command = SharedPtr<ConsoleCommand>(new ConsoleCommand(name, desc));
    commands_[name] = command;
    return command.Get();
}

void ConsoleAPI::UnregisterCommand(const String &name)
{
    auto existing = FindCaseInsensitive(name);
    if (existing != commands_.End())
    {
        LogWarning("ConsoleAPI: Trying to unregister non-existing command '" + name + "'.");
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
    auto existing = FindCaseInsensitive(name);
    if (existing == commands_.End())
    {
        LogError("Cannot find a console command '" + name + "'");
        return;
    }
    existing->second_->Invoke(parameters);
}

void ConsoleAPI::ListCommands()
{
    LogInfo("Available Console Commands (case-insensitive)");
    uint longestName = 0;
    for(auto iter = commands_.Begin(); iter != commands_.End(); ++iter)
        if (iter->first_.Length() > longestName)
            longestName = iter->first_.Length();
    longestName += 2;
    for(auto iter = commands_.Begin(); iter != commands_.End(); ++iter)
        LogInfo("  " + PadString(iter->first_, longestName) + iter->second_->Description());
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

void ConsoleAPI::SetLogLevel(const String &level)
{
    if (level.Compare("error", false))
        SetEnabledLogChannels(LogLevelErrorsOnly);
    else if (level.Compare("warning", false))
        SetEnabledLogChannels(LogLevelErrorWarning);
    else if (level.Compare("info", false))
        SetEnabledLogChannels(LogLevelErrorWarnInfo);
    else if (level.Compare("debug", false))
        SetEnabledLogChannels(LogLevelErrorWarnInfoDebug);
    else
        LogError("Unknown parameter \"" + level + "\" specified to ConsoleAPI::SetLogLevel!");
}

void ConsoleAPI::SetEnabledLogChannels(u32 newChannels)
{
    enabledLogChannels = newChannels;
}

bool ConsoleAPI::IsLogChannelEnabled(u32 logChannel) const
{
    return (enabledLogChannels & logChannel) != 0;
}

u32 ConsoleAPI::EnabledLogChannels() const
{
    return enabledLogChannels;
}

void ConsoleAPI::Print(const String &message)
{
    LOGRAW(message);
}

}

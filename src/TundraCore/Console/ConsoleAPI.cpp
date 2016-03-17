/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   ConsoleAPI.cpp
    @brief  Console core API. */

#include "StableHeaders.h"
#include "Win.h"
#include "ConsoleAPI.h"
#include "Framework.h"
#include "FrameAPI.h"

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/EngineEvents.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Engine/Console.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/IO/Log.h>

namespace Tundra
{

ConsoleAPI::ConsoleAPI(Framework *framework) :
    Object(framework->GetContext()),
    framework_(framework),
    logLevel_(LogLevelInfo),
    pollInput_(1.f/30.f)
{
    SubscribeToEvent(Urho3D::E_CONSOLECOMMAND, URHO3D_HANDLER(ConsoleAPI, HandleConsoleCommand));

    RegisterCommand("help", "Lists all registered commands.", this, &ConsoleAPI::ListCommands);
}

void ConsoleAPI::Initialize()
{
    framework_->Frame()->Updated.Connect(this, &ConsoleAPI::OnUpdate);
}

ConsoleAPI::~ConsoleAPI()
{
    commands_.clear();
}

StringVector ConsoleAPI::AvailableCommands() const
{
    StringVector ret;
    for(auto iter = commands_.begin(); iter != commands_.end(); ++iter)
        ret.Push(iter->first);
    return ret;
}

ConsoleCommand *ConsoleAPI::Command(const String &name) const
{
    auto existing = commands_.find(name);
    if (existing != commands_.end())
    {
        LogWarning("ConsoleAPI::RegisterCommand: Command '" + name + "' does not exist.");
        return 0;
    }
    return existing->second.Get();
}

ConsoleCommand *ConsoleAPI::RegisterCommand(const String &name, const String &desc)
{
    if (name.Empty())
    {
        LogError("ConsoleAPI::RegisterCommand: Command name can not be an empty string.");
        return 0;
    }
    auto existing = commands_.find(name);
    if (existing != commands_.end())
    {
        LogWarning("ConsoleAPI::RegisterCommand: Command '" + name + "' is already registered.");
        return existing->second.Get();
    }

    SharedPtr<ConsoleCommand> command = SharedPtr<ConsoleCommand>(new ConsoleCommand(name, desc));
    commands_[name] = command;
    return command.Get();
}

void ConsoleAPI::UnregisterCommand(const String &name)
{
    auto existing = commands_.find(name);
    if (existing != commands_.end())
    {
        LogWarning("ConsoleAPI: Trying to unregister non-existing command '" + name + "'.");
        return;
    }
    commands_.erase(existing);
}

void ConsoleAPI::ExecuteCommand(const String &command)
{
    String name;
    StringVector parameters;
    ParseCommand(command, name, parameters);
    if (name.Empty())
        return;
    auto existing = commands_.find(name);
    if (existing == commands_.end())
    {
        LogError("Cannot find a console command '" + name + "'");
        return;
    }
    existing->second->Invoke(parameters);
}

void ConsoleAPI::ListCommands() const
{
    LogInfo("Available Console Commands (case-insensitive)");
    uint longestName = 0;
    for(auto iter = commands_.begin(); iter != commands_.end(); ++iter)
        if (iter->first.Length() > longestName)
            longestName = iter->first.Length();
    longestName += 2;
    for(auto iter = commands_.begin(); iter != commands_.end(); ++iter)
        LogInfo("  " + PadString(iter->first, longestName) + iter->second->Description());
}

void ConsoleAPI::HandleConsoleCommand(StringHash /*eventType*/, VariantMap &eventData)
{
    ExecuteCommand(eventData[Urho3D::ConsoleCommand::P_COMMAND].GetString());
}

void ConsoleAPI::OnUpdate(float frametime)
{
    if (pollInput_.ShouldUpdate(frametime))
    {
        // Check if there is input from stdin
        String input = Urho3D::GetConsoleInput();
        if (input.Length())
            ExecuteCommand(input);
    }
}

void ConsoleAPI::SetConsoleVisible(bool /*visible*/)
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

LogLevel ConsoleAPI::LogLevelFromString(const String &level)
{
    if (level.Compare("none", false) == 0 || level.Compare("disabled", false) == 0)
        return LogLevelNone;
    else if (level.Compare("error", false) == 0)
        return LogLevelError;
    else if (level.Compare("warn", false) == 0 || level.Compare("warning", false) == 0)
        return LogLevelWarning;
    else if (level.Compare("info", false) == 0)
        return LogLevelInfo;
    else if (level.Compare("debug", false) == 0 || level.Compare("verbose", false) == 0)
        return LogLevelDebug;
    else
        LogError("ConsoleAPI::LogLevelFromString: Unsupported log level '" + level + "'. Returning LogLevelInfo.");
    return LogLevelInfo;
}

void ConsoleAPI::SetLogLevel(const String &level)
{
    logLevel_ = LogLevelFromString(level);
}

bool ConsoleAPI::IsLogLevelEnabled(LogLevel level) const
{
    return (level >= logLevel_);
}

LogLevel ConsoleAPI::CurrentLogLevel() const
{
    return logLevel_;
}

void ConsoleAPI::Print(const String &message)
{
    URHO3D_LOGRAW(message);
}

}

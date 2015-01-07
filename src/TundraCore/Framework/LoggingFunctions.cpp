/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   LoggingFunctions.cpp
    @brief  Tundra logging utility functions. */

#include "StableHeaders.h"
#include "Win.h"
#include "LoggingFunctions.h"
#include "Framework.h"
#include "Console/ConsoleAPI.h"

#include <Urho3D/Core/ProcessUtils.h>

namespace Tundra
{

void PrintLogMessage(LogLevel level, const String &str)
{
    if (!IsLogLevelEnabled(level))
        return;

    Framework *instance = Framework::Instance();
    ConsoleAPI *console = (instance ? instance->Console() : 0);

#ifdef WIN32
    // On Windows, highlight errors and warnings.
    HANDLE stdoutHandle = (level == LogLevelError || level == LogLevelWarning ? GetStdHandle(STD_OUTPUT_HANDLE) : INVALID_HANDLE_VALUE);
    if (stdoutHandle != INVALID_HANDLE_VALUE)
    {
        if (level == LogLevelError)
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_INTENSITY);
        else if (level == LogLevelWarning)
            SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
    }
#endif

    // The console and stdout prints are equivalent.
    if (console)
        console->Print(str);
    else // The Console API is already dead for some reason, print directly to stdout to guarantee we don't lose any logging messages.
        PrintRaw(str);

#ifdef WIN32
    // Restore the text color to normal if was changed above.
    if (stdoutHandle)
        SetConsoleTextAttribute(stdoutHandle, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#endif
}

bool IsLogLevelEnabled(LogLevel level)
{
    Framework *instance = Framework::Instance();
    ConsoleAPI *console = (instance ? instance->Console() : 0);
    if (console)
        return console->IsLogLevelEnabled(level);
    else
        return true; // We've already killed Framework and Console! Print out everything so that we can't accidentally lose any important messages.
}

void PrintRaw(const String &str)
{
    Urho3D::PrintUnicode(str);
}

}

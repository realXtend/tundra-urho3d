/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   LoggingFunctions.h
    @brief  Tundra logging utility functions. */

#pragma once

#include "TundraCoreApi.h"
#include "CoreTypes.h"

#include <Str.h>
#include <string>
#include <Log.h>

namespace Tundra
{

/// Available log levels. Each log level includes all the output from the levels above it.
enum LogLevel
{
    LogLevelDebug    = 0, // Urho3D::LOG_DEBUG
    LogLevelInfo     = 1, // Urho3D::LOG_INFO
    LogLevelWarning  = 2, // Urho3D::LOG_WARNING
    LogLevelError    = 3, // Urho3D::LOG_ERROR
    LogLevelNone     = 4  // Urho3D::LOG_NONE
};

namespace Detail // Hide from Tundra namespace
{
    const String Newline = "\n";
}

/// Outputs a message to the log to the given channel (if @c level is enabled) to both stdout and ConsoleAPI.
/** On Windows, yellow and red text colors are used for warning and error prints. */
void TUNDRACORE_API PrintLogMessage(LogLevel level, const String &str);

/// Returns true if the given log level is enabled.
bool TUNDRACORE_API IsLogLevelEnabled(LogLevel level);

/// Outputs a string to the stdout.
void TUNDRACORE_API PrintRaw(const String &str);

/// Outputs an error message to the log (if LogLevelError is enabled), both stdout and ConsoleAPI, with "Error: " prefix and with newline appended.
/** On Windows, red text color is used for the stdout print. */
static inline void LogError(const String &msg)                          { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + msg + Detail::Newline);}

/// Outputs a warning message to the log (if LogLevelWarning is enabled), both stdout and ConsoleAPI, with "Warning: " prefix and with newline appended.
/** On Windows, yellow text color is used for the stdout print. */
static inline void LogWarning(const String &msg)                        { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + msg + Detail::Newline);}

/// Outputs an information message to the log (if LogLevelInfo is enabled), both stdout and ConsoleAPI, with newline appended.
static inline void LogInfo(const String &msg)                           { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, msg + Detail::Newline);}

/// Outputs a debug message to the log (if LogLevelDebug is enabled), both stdout and ConsoleAPI, with "Debug: " prefix and with newline appended.
static inline void LogDebug(const String &msg)                          { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: " + msg + Detail::Newline);}

static inline void LogError(const std::string &msg)   /**< @overload */ { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + String(msg.c_str()) + Detail::Newline); }
static inline void LogWarning(const std::string &msg) /**< @overload */ { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + String(msg.c_str()) + Detail::Newline); }
static inline void LogInfo(const std::string &msg)    /**< @overload */ { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, String(msg.c_str()) + Detail::Newline); }
static inline void LogDebug(const std::string &msg)   /**< @overload */ { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: "+ String(msg.c_str()) + Detail::Newline); }

static inline void LogError(const char *msg)          /**< @overload */ { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + String(msg) + Detail::Newline); }
static inline void LogWarning(const char *msg)        /**< @overload */ { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + String(msg) + Detail::Newline); }
static inline void LogInfo(const char *msg)           /**< @overload */ { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, String(msg) + Detail::Newline); }
static inline void LogDebug(const char *msg)          /**< @overload */ { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: " + String(msg) + Detail::Newline); }

namespace Detail // Hide from Tundra namespace
{
    static inline void LogFormatted(LogLevel level, const String &prefix, const char* formatString, va_list args)
    {
        String msg;
        if (!prefix.Empty())
            msg.Append(prefix);
        msg.AppendWithFormatArgs(formatString, args);
        va_end(args);
        msg.Append(Newline);
        PrintLogMessage(level, msg);
    }
}

/// Log formatted error. @see http://www.cplusplus.com/reference/cstdio/printf/.
static inline void LogErrorF(const char* formatString, ...)             { if (IsLogLevelEnabled(LogLevelError))   { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelError, "", formatString, a); } }
/// Log formatted warning. @see http://www.cplusplus.com/reference/cstdio/printf/.
static inline void LogWarningF(const char* formatString, ...)           { if (IsLogLevelEnabled(LogLevelWarning)) { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelWarning, "", formatString, a); } }
/// Log formatted info line. @see http://www.cplusplus.com/reference/cstdio/printf/.
static inline void LogInfoF(const char* formatString, ...)              { if (IsLogLevelEnabled(LogLevelInfo))    { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelInfo, "", formatString, a); } }
/// Log formatted debug line. @see http://www.cplusplus.com/reference/cstdio/printf/.
static inline void LogDebugF(const char* formatString, ...)             { if (IsLogLevelEnabled(LogLevelDebug))   { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelDebug, "", formatString, a); } }

/// Simple logger that provide a prefix 'name' to each line.
struct Logger
{
    /// Creates new Logger with name.
    Logger(const String &name) : prefix("[" + name + "] ") {}

    void Error(const String &msg) const                 { if (IsLogLevelEnabled(LogLevelError))     LogError(prefix + msg); }
    void Warning(const String &msg) const               { if (IsLogLevelEnabled(LogLevelWarning))   LogWarning(prefix + msg); }
    void Info(const String &msg) const                  { if (IsLogLevelEnabled(LogLevelInfo))      LogInfo(prefix + msg); }
    void Debug(const String &msg) const                 { if (IsLogLevelEnabled(LogLevelDebug))     LogDebug(prefix + msg); }

    void ErrorF(const char* formatString, ...) const    { if (IsLogLevelEnabled(LogLevelError))   { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelError, prefix, formatString, a); } }
    void WarningF(const char* formatString, ...) const  { if (IsLogLevelEnabled(LogLevelWarning)) { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelWarning, prefix, formatString, a); } }
    void InfoF(const char* formatString, ...) const     { if (IsLogLevelEnabled(LogLevelInfo))    { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelInfo, prefix, formatString, a); } }
    void DebugF(const char* formatString, ...) const    { if (IsLogLevelEnabled(LogLevelDebug))   { va_list a; va_start(a, formatString); Detail::LogFormatted(LogLevelDebug, prefix, formatString, a); } }

    String prefix;
};

}

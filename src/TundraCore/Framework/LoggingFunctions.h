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

enum LogLevel
{
    LogLevelDebug    = 0, // Urho3D::LOG_DEBUG
    LogLevelInfo     = 1, // Urho3D::LOG_INFO
    LogLevelWarning  = 2, // Urho3D::LOG_WARNING
    LogLevelError    = 3, // Urho3D::LOG_ERROR
    LogLevelNone     = 4  // Urho3D::LOG_NONE
};

const String Newline = "\n";

/// Outputs a message to the log to the given channel (if the level is enabled) to both stdout and ConsoleAPI.
/** On Windows, yellow and red text colors are used for warning and error prints. */
void TUNDRACORE_API PrintLogMessage(LogLevel level, const String &str);

/// Returns true if the given log level is enabled.
bool TUNDRACORE_API IsLogLevelEnabled(LogLevel level);

/// Outputs a string to the stdout.
void TUNDRACORE_API PrintRaw(const String &str);

/// Outputs an error message to the log (if the channel is enabled), both stdout and ConsoleAPI, with "Error: " prefix and with newline appended.
/** On Windows, red text color is used for the stdout print. */
static inline void LogError(const String &msg)                          { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + msg + Newline);}

/// Outputs a warning message to the log (if the channel is enabled), both stdout and ConsoleAPI, with "Warning: " prefix and with newline appended.
/** On Windows, yellow text color is used for the stdout print. */
static inline void LogWarning(const String &msg)                        { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + msg + Newline);}

/// Outputs an information message to the log (if the channel is enabled), both stdout and ConsoleAPI, with newline appended.
static inline void LogInfo(const String &msg)                           { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, msg + Newline);}

/// Outputs a debug message to the log (if the channel is enabled), both stdout and ConsoleAPI, with "Debug: " prefix and with newline appended.
static inline void LogDebug(const String &msg)                          { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: " + msg + Newline);}

static inline void LogError(const std::string &msg)   /**< @overload */ { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + String(msg.c_str()) + Newline); }
static inline void LogWarning(const std::string &msg) /**< @overload */ { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + String(msg.c_str()) + Newline); }
static inline void LogInfo(const std::string &msg)    /**< @overload */ { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, String(msg.c_str()) + Newline); }
static inline void LogDebug(const std::string &msg)   /**< @overload */ { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: "+ String(msg.c_str()) + Newline); }

static inline void LogError(const char *msg)          /**< @overload */ { if (IsLogLevelEnabled(LogLevelError))   PrintLogMessage(LogLevelError, "Error: " + String(msg) + Newline); }
static inline void LogWarning(const char *msg)        /**< @overload */ { if (IsLogLevelEnabled(LogLevelWarning)) PrintLogMessage(LogLevelWarning, "Warning: " + String(msg) + Newline); }
static inline void LogInfo(const char *msg)           /**< @overload */ { if (IsLogLevelEnabled(LogLevelInfo))    PrintLogMessage(LogLevelInfo, String(msg) + Newline); }
static inline void LogDebug(const char *msg)          /**< @overload */ { if (IsLogLevelEnabled(LogLevelDebug))   PrintLogMessage(LogLevelDebug, "Debug: " + String(msg) + Newline); }

// Simple logger that provide a prefix 'name' to each line.
struct Logger
{
    /// Creates new Logger with name.
    Logger(const String &name) : prefix("[" + name + "] ") {}

    void Error(const String &msg) const   { LogError(prefix + msg); }
    void Warning(const String &msg) const { LogWarning(prefix + msg); }
    void Info(const String &msg) const    { LogInfo(prefix + msg); }
    void Debug(const String &msg) const   { LogDebug(prefix + msg); }

    String prefix;
};

}

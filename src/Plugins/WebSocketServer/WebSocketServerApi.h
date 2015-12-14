// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(WEBSOCKETSERVER_EXPORTS)
#define WEBSOCKETSERVER_API __declspec(dllexport)
#else
#define WEBSOCKETSERVER_API __declspec(dllimport)
#endif
#else
#define WEBSOCKETSERVER_API
#endif

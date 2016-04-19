// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#if defined (_WINDOWS)
#if defined(HTTPSERVER_EXPORTS)
#define HTTPSERVER_API __declspec(dllexport)
#else
#define HTTPSERVER_API __declspec(dllimport)
#endif
#else
#define HTTPSERVER_API
#endif

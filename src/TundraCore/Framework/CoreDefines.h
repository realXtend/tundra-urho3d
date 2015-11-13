/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   CoreDefines.h
    @brief  Preprocessor macros. */

#pragma once

#include <cassert>

// Disable DLL interface related warnings resulting from Urho3D template classes
#pragma warning(disable:4251)
#pragma warning(disable:4275)

/** @def DLLEXPORT
    __declspec(dllexport) for Windows, empty otherwise. */
#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

/// Uses operator delete to delete p and sets p to nullptr after that.
#define SAFE_DELETE(p) { delete p; p = nullptr; }
/// Uses operator delete[] to delete p and sets p to nullptr after that.
#define SAFE_DELETE_ARRAY(p) { delete [] p; p = nullptr; }

/// Specifies the number of elements in a C array.
#define NUMELEMS(x) (sizeof(x)/sizeof(x[0]))

/// Use this template to downcast from a base class to a derived class when you know by static code analysis what the derived 
/// type has to be and don't want to pay the runtime performance incurred by dynamic_casting. In debug mode, the proper
/// derived type will be assert()ed, but in release mode this be just the same as using static_cast.
/// Repeating to make a note: In RELEASE mode, checked_static_cast == static_cast. It is *NOT* a substitute to use in places
/// where you really need a dynamic_cast.
template<typename Dst, typename Src>
inline Dst checked_static_cast(Src src)
{
    assert(src == 0 || dynamic_cast<Dst>(src) != 0);
    return static_cast<Dst>(src);
}

/// foreach macro for STL containers (and any other containers providing begin() and end() member functions) and C arrays.
/** @bug break withing foreach is buggy at the moment (only breaks one iteration, does not break for good). Urho's implementation
    was used as a base for this, issue for Urho's foreach can be found from here https://github.com/urho3d/Urho3D/issues/561 */
#define foreach_std(val, container) \
    for(auto it = std::begin(container); it != std::end(container); ++it) \
        if (bool _foreach_flag = false) {} \
        else for(val = *it; !_foreach_flag; _foreach_flag = true)

/** @def UNUSED_PARAM(x)
    Preprocessor macro for suppressing unused formal parameter warnings while still showing the variable name in Doxygen documentation.  */
#if defined(DOXYGEN) // DOXYGEN is a special define used when Doxygen is run.
#define UNUSED_PARAM(x) x
#else
#define UNUSED_PARAM(x)
#endif

/// ARG is meant simply for documenting arguments of Signals.
#define ARG(x)

/// Use to suppress warning C4101 (unreferenced local variable)
#define UNREFERENCED_PARAM(P) (void)(P);

/** @def DEPRECATED(func)
    Raises compiler warning for a deprecated function. */
#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#define DEPRECATED(func) func
#endif

#ifdef __GNUC__
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#if GCC_VERSION < 40700
#define override // 'overrride' available at GCC >= 4.7.0
#endif
#endif // ~__GNUC__

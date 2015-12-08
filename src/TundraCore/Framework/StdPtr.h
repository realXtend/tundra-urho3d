// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <memory>

#if defined(_MSC_VER) && (_MSC_VER == 1500) || (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 2 && !defined(__APPLE__))
#define STDTR1_NAMESPACE std::tr1
#elif defined(_MSC_VER) && (_MSC_VER >= 1600) || (defined(__APPLE__) && defined(__clang__) && __clang_major__ == 4 && __clang_minor__ == 2) || (defined(__GNUC__) && __GNUC__ == 4 && __GNUC_MINOR__ >= 8 && !defined(__APPLE__))
#define STDTR1_NAMESPACE std
#endif

using STDTR1_NAMESPACE::shared_ptr;
using STDTR1_NAMESPACE::weak_ptr;
using STDTR1_NAMESPACE::dynamic_pointer_cast;
using STDTR1_NAMESPACE::static_pointer_cast;
using STDTR1_NAMESPACE::enable_shared_from_this;

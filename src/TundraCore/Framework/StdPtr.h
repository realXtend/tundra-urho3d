// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include <memory>

#if defined(_MSC_VER) && (_MSC_VER == 1500) 
#define STDTR1_NAMESPACE std::tr1
#else
#define STDTR1_NAMESPACE std
#endif

using STDTR1_NAMESPACE::shared_ptr;
using STDTR1_NAMESPACE::weak_ptr;
using STDTR1_NAMESPACE::dynamic_pointer_cast;
using STDTR1_NAMESPACE::static_pointer_cast;
using STDTR1_NAMESPACE::enable_shared_from_this;

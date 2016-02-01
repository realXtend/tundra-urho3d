#include "duktape.h"

namespace Bindings
{

template<class T> T* GetObject(duk_context* ctx, duk_idx_t stackIndex, const char* typeName)
{
    if (!duk_is_object(ctx, stackIndex))
        return 0;

    duk_get_prop_string(ctx, stackIndex, "\xff""obj");
    T* obj = static_cast<T*>(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    // No type safety check
    if (!typeName)
        return obj;

    duk_get_prop_string(ctx, stackIndex, "\xff""type");
    const char* objTypeName = (const char*)duk_to_pointer(ctx, -1);
    duk_pop(ctx);
    if (objTypeName == typeName)
        return obj;
    else
        return 0;
}

template<class T> T* GetThisObject(duk_context* ctx, const char* typeName)
{
    duk_push_this(ctx);
    T* obj = getobject<T>(ctx, -1, typeName);
    duk_pop(ctx);
    return obj;
}

void SetObject(duk_context* ctx, duk_idx_t stackIndex, void* obj, const char* typeName);

void DefineProperty(duk_context* ctx, const char* propertyName, duk_c_function getFunc, duk_c_function setFunc);

}
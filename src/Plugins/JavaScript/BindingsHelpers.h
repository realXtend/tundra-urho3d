#include "duktape.h"

namespace JSBindings
{

void SetObject(duk_context* ctx, duk_idx_t stackIndex, void* obj, const char* typeName);

void DefineProperty(duk_context* ctx, const char* propertyName, duk_c_function getFunc, duk_c_function setFunc);

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

template<class T> T* GetCheckedObject(duk_context* ctx, duk_idx_t stackIndex, const char* typeName)
{
    T* obj = GetObject<T>(ctx, stackIndex, typeName);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null or invalid object argument");
    return obj;
}

template<class T> T* GetThisObject(duk_context* ctx, const char* typeName)
{
    duk_push_this(ctx);
    T* obj = GetObject<T>(ctx, -1, typeName);
    duk_pop(ctx);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null this pointer");
    return obj;
}

template<class T> void PushValueObjectCopy(duk_context* ctx, const T& source, const char* typeName, duk_c_function destructor)
{
    duk_push_object(ctx);
    SetObject(ctx, -1, new T(source), typeName);
    duk_push_c_function(ctx, destructor, 1);
    duk_set_finalizer(ctx, -2);
    // When pushing an object without going through the constructor, have to set prototype manually
    duk_get_global_string(ctx, typeName);
    duk_get_prop_string(ctx, -1, "prototype");
    duk_set_prototype(ctx, -3);
    duk_pop(ctx);
}

template<class T> void PushConstructorResult(duk_context* ctx, T* source, const char* typeName, duk_c_function destructor)
{
   duk_push_this(ctx);
   SetObject(ctx, -1, source, typeName);
   duk_push_c_function(ctx, destructor, 1);
   duk_set_finalizer(ctx, -2);
}

}
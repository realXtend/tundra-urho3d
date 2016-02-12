// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "BindingsHelpers.h"

namespace JSBindings
{

void SetValueObject(duk_context* ctx, duk_idx_t stackIndex, void* obj, const char* typeName)
{
    if (stackIndex < 0)
        --stackIndex;
    duk_push_pointer(ctx, obj);
    duk_put_prop_string(ctx, stackIndex, "\xff""obj");
    duk_push_pointer(ctx, (void*)typeName);
    duk_put_prop_string(ctx, stackIndex, "\xff""type");
}

void DefineProperty(duk_context* ctx, const char* propertyName, duk_c_function getFunc, duk_c_function setFunc)
{
    duk_push_string(ctx, propertyName);
    if (setFunc)
    {
        duk_push_c_function(ctx, getFunc, 0);
        duk_push_c_function(ctx, setFunc, 1);
        duk_def_prop(ctx, -4, DUK_DEFPROP_HAVE_GETTER | DUK_DEFPROP_HAVE_SETTER);
    }
    else
    {
        duk_push_c_function(ctx, getFunc, 0);
        duk_def_prop(ctx, -3, DUK_DEFPROP_HAVE_GETTER);
    }
}

Urho3D::WeakPtr<Urho3D::RefCounted>* GetWeakPtr(duk_context* ctx, duk_idx_t stackIndex)
{
    if (!duk_is_object(ctx, stackIndex))
        return nullptr;

    Urho3D::WeakPtr<Urho3D::RefCounted>* ptr = nullptr;
    duk_get_prop_string(ctx, stackIndex, "\xff""weak");
    if (duk_is_pointer(ctx, -1))
        ptr = static_cast<Urho3D::WeakPtr<Urho3D::RefCounted>* >(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    return ptr;
}

duk_ret_t RefCounted_Finalizer(duk_context* ctx)
{
    Urho3D::WeakPtr<Urho3D::RefCounted>* ptr = GetWeakPtr(ctx, 0);
    if (ptr)
    {
        delete ptr;
        duk_push_pointer(ctx, nullptr);
        duk_put_prop_string(ctx, 0, "\xff""weak");
    }
    return 0;
}

}
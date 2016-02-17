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

Urho3D::WeakPtr<Urho3D::Object>* GetWeakPtr(duk_context* ctx, duk_idx_t stackIndex)
{
    if (!duk_is_object(ctx, stackIndex))
        return nullptr;

    Urho3D::WeakPtr<Urho3D::Object>* ptr = nullptr;
    duk_get_prop_string(ctx, stackIndex, "\xff""weak");
    if (duk_is_pointer(ctx, -1))
        ptr = static_cast<Urho3D::WeakPtr<Urho3D::Object>* >(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    return ptr;
}

void PushWeakObject(duk_context* ctx, Urho3D::Object* object)
{
    duk_push_object(ctx);
    Urho3D::WeakPtr<Urho3D::Object>* ptr = new Urho3D::WeakPtr<Urho3D::Object>(object);
    duk_push_pointer(ctx, ptr);
    duk_put_prop_string(ctx, -2, "\xff""weak");
    duk_push_c_function(ctx, WeakPtr_Finalizer, 1);
    duk_set_finalizer(ctx, -2);

    // Set prototype. If not found, use base class prototype (e.g. IComponent)
    duk_get_global_string(ctx, object->GetTypeName().CString());
    if (!duk_is_object(ctx, -1))
    {
        duk_pop(ctx);
        duk_get_global_string(ctx, object->GetTypeInfo()->GetBaseTypeInfo()->GetTypeName().CString());
    }
    duk_get_prop_string(ctx, -1, "prototype");
    duk_set_prototype(ctx, -3);
    duk_pop(ctx);
}

duk_ret_t WeakPtr_Finalizer(duk_context* ctx)
{
    Urho3D::WeakPtr<Urho3D::Object>* ptr = GetWeakPtr(ctx, 0);
    if (ptr)
    {
        delete ptr;
        duk_push_pointer(ctx, nullptr);
        duk_put_prop_string(ctx, 0, "\xff""weak");
    }
    return 0;
}

Urho3D::Vector<Urho3D::String> GetStringVector(duk_context* ctx, duk_idx_t stackIndex)
{
    Urho3D::Vector<Urho3D::String> ret;

    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            if (duk_is_string(ctx, -1))
                ret.Push(Urho3D::String(duk_get_string(ctx, -1)));
            duk_pop(ctx);
        }
    }

    return ret;
}

void PushStringVector(duk_context* ctx, const Urho3D::Vector<Urho3D::String> vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        duk_push_string(ctx, vector[i].CString());
        duk_put_prop_index(ctx, -2, i);
    }
}

}
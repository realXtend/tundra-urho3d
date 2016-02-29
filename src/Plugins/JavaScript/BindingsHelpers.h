#pragma once

#include "duktape.h"
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Variant.h>
#include <Urho3D/Container/Str.h>
#include <Urho3D/Container/Vector.h>

namespace JSBindings
{

/// Non-template functions
/// Set a C++ object to a JS object at stack index, using the "obj" (pointer) and "type" (string) internal properties.
void SetValueObject(duk_context* ctx, duk_idx_t stackIndex, void* obj, const char* typeName);

/// Define a property to an object prototype at top of stack.
void DefineProperty(duk_context* ctx, const char* propertyName, duk_c_function getFunc, duk_c_function setFunc);

/// Get a WeakPtr<Object> from a JS object.
Urho3D::WeakPtr<Urho3D::Object>* GetWeakPtr(duk_context* ctx, duk_idx_t stackIndex);

/// Common WeakPtr<Object> finalizer.
duk_ret_t WeakPtr_Finalizer(duk_context* ctx);

/// Push a weak-refcounted object that must derive from Urho3D::Object. Uses an internal "weak" property to store the object inside a heap-allocated weak ptr.
void PushWeakObject(duk_context* ctx, Urho3D::Object* object);

/// Get a string vector from a JS array.
Urho3D::Vector<Urho3D::String> GetStringVector(duk_context* ctx, duk_idx_t stackIndex);

/// Push a string vector to JS array.
void PushStringVector(duk_context* ctx, const Urho3D::Vector<Urho3D::String>& vector);

/// Convert and push a variant.
void PushVariant(duk_context* ctx, const Urho3D::Variant& variant);

/// Get a variant from JS stack.
Urho3D::Variant GetVariant(duk_context* ctx, duk_idx_t stackIndex);

/// Value object template functions

/// Get a value object of specified type from JS object at stack index. Uses the "obj" (pointer) and "type" (string) internal properties.
template<class T> T* GetValueObject(duk_context* ctx, duk_idx_t stackIndex, const char* typeName)
{
    if (!duk_is_object(ctx, stackIndex))
        return nullptr;

    duk_get_prop_string(ctx, stackIndex, "\xff""obj");
    T* obj = nullptr;
    if (duk_is_pointer(ctx, -1))
        obj = static_cast<T*>(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    // No type safety check
    if (!typeName)
        return obj;

    duk_get_prop_string(ctx, stackIndex, "\xff""type");
    const char* objTypeName = nullptr;
    if (duk_is_pointer(ctx, -1))
        objTypeName = (const char*)duk_to_pointer(ctx, -1);
    duk_pop(ctx);

    return (objTypeName == typeName) ? obj : nullptr;
}

/// Get an object of type and raise a JS error if null or invalid.
template<class T> T* GetCheckedValueObject(duk_context* ctx, duk_idx_t stackIndex, const char* typeName)
{
    T* obj = GetValueObject<T>(ctx, stackIndex, typeName);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null or invalid object reference, should be %s", typeName);
    return obj;
}

/// Get this value object of specified type. Raise JS error if null or invalid.
template<class T> T* GetThisValueObject(duk_context* ctx, const char* typeName)
{
    duk_push_this(ctx);
    T* obj = GetValueObject<T>(ctx, -1, typeName);
    duk_pop(ctx);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null this object reference for type %s", typeName);
    return obj;
}

/// Push a copy of a value (non-refcounted) object on the stack. Requires the object to have a copy constructor. Finalizer function for the object needs to be specified.
template<class T> void PushValueObjectCopy(duk_context* ctx, const T& source, const char* typeName, duk_c_function finalizer)
{
    duk_push_object(ctx);
    SetValueObject(ctx, -1, new T(source), typeName);
    duk_push_c_function(ctx, finalizer, 1);
    duk_set_finalizer(ctx, -2);
    // When pushing an object without going through the constructor, have to set prototype manually
    duk_get_global_string(ctx, typeName);
    duk_get_prop_string(ctx, -1, "prototype");
    duk_set_prototype(ctx, -3);
    duk_pop(ctx);
}

/// Push a value object on the stack. Finalizer function for the object needs to be specified. Optionally set prototype.
template<class T> void PushValueObject(duk_context* ctx, T* source, const char* typeName, duk_c_function finalizer, bool setPrototype)
{
    duk_push_object(ctx);
    SetValueObject(ctx, -1, source, typeName);
    duk_push_c_function(ctx, finalizer, 1);
    duk_set_finalizer(ctx, -2);
    // When pushing an object without going through the constructor, have to set prototype manually
    if (setPrototype)
    {
        duk_get_global_string(ctx, typeName);
        duk_get_prop_string(ctx, -1, "prototype");
        duk_set_prototype(ctx, -3);
        duk_pop(ctx);
    }
}

/// Push the result of a value object constructor. Finalizer function for the object needs to be specified.
template<class T> void PushConstructorResult(duk_context* ctx, T* source, const char* typeName, duk_c_function finalizer)
{
   duk_push_this(ctx);
   SetValueObject(ctx, -1, source, typeName);
   duk_push_c_function(ctx, finalizer, 1);
   duk_set_finalizer(ctx, -2);
}

/// Push a vector of value objects as an array.
template<class T> void PushValueObjectVector(duk_context* ctx, const Urho3D::Vector<T>& vector, const char* typeName, duk_c_function finalizer)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        PushValueObjectCopy(ctx, vector[i], typeName, finalizer);
        duk_put_prop_index(ctx, -2, i);
    }
}

/// Get a weak-refcounted object of type from JS object at stack index. Objects are stored internally using WeakPtr<Object>, and cast accordingly.
template<class T> T* GetWeakObject(duk_context* ctx, duk_idx_t stackIndex)
{
    T* obj = nullptr;
    Urho3D::WeakPtr<Urho3D::Object>* ptr = GetWeakPtr(ctx, stackIndex);
    if (ptr)
        obj = dynamic_cast<T*>(ptr->Get());
    return obj;
}

/// Get a weak-refcounted object of type and raise a JS error if null or invalid.
template<class T> T* GetCheckedWeakObject(duk_context* ctx, duk_idx_t stackIndex)
{
    T* obj = GetWeakObject<T>(ctx, stackIndex);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null or invalid refcounted object reference");
    return obj;
}

/// Get weak refcounted this object of specified type. Raise JS error if null or invalid.
template<class T> T* GetThisWeakObject(duk_context* ctx)
{
    duk_push_this(ctx);
    T* obj = GetWeakObject<T>(ctx, -1);
    duk_pop(ctx);
    if (!obj)
        duk_error(ctx, DUK_ERR_REFERENCE_ERROR, "Null this refcounted object reference");
    return obj;
}

/// Get a vector of value objects from a Javascript array. Null indices are skipped.
template<class T> Urho3D::Vector<T> GetValueObjectVector(duk_context* ctx, duk_idx_t stackIndex, const char* typeName)
{
    Urho3D::Vector<T> ret;
    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            T* obj = GetValueObject<T>(ctx, -1, typeName);
            if (obj)
                ret.Push(T(*obj));
            duk_pop(ctx);
        }
    }

    return ret;
}

/// Get a vector of refcounted objects from a Javascript array. Null indices are skipped.
template<class T> Urho3D::Vector<T*> GetWeakObjectVector(duk_context* ctx, duk_idx_t stackIndex)
{
    Urho3D::Vector<T> ret;
    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            T* obj = GetWeakObject<T>(ctx, -1);
            if (obj)
                ret.Push(obj);
            duk_pop(ctx);
        }
    }

    return ret;
}

/// Get a shared ptr vector of refcounted objects from a Javascript array. Null indices are skipped.
template<class T> Urho3D::Vector<Urho3D::SharedPtr<T> > GetWeakObjectSharedPtrVector(duk_context* ctx, duk_idx_t stackIndex)
{
    Urho3D::Vector<Urho3D::SharedPtr<T> > ret;
    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            T* obj = GetWeakObject<T>(ctx, -1);
            if (obj)
                ret.Push(Urho3D::SharedPtr<T>(obj));
            duk_pop(ctx);
        }
    }

    return ret;
}

/// Get a weak ptr vector of refcounted objects from a Javascript array. Null indices are skipped.
template<class T> Urho3D::Vector<Urho3D::WeakPtr<T> > GetWeakObjectWeakPtrVector(duk_context* ctx, duk_idx_t stackIndex)
{
    Urho3D::Vector<Urho3D::WeakPtr<T> > ret;
    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            T* obj = GetWeakObject<T>(ctx, -1);
            if (obj)
                ret.Push(Urho3D::WeakPtr<T>(obj));
            duk_pop(ctx);
        }
    }

    return ret;
}

/// Push a vector of weak-refcounted objects as an array. Null indices are included as null objects.
template<class T> void PushWeakObjectVector(duk_context* ctx, const Urho3D::Vector<T*>& vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        PushWeakObject(ctx, vector[i]);
        duk_put_prop_index(ctx, -2, i);
    }
}

/// Push a vector of weak-refcounted objects as an array. Null indices are included as null objects.
template<class T> void PushWeakObjectVector(duk_context* ctx, const Urho3D::Vector<Urho3D::SharedPtr<T> >& vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        PushWeakObject(ctx, vector[i].Get());
        duk_put_prop_index(ctx, -2, i);
    }
}

/// Push a vector of weak-refcounted objects as an array. Null indices are included as null objects.
template<class T> void PushWeakObjectVector(duk_context* ctx, const Urho3D::Vector<Urho3D::WeakPtr<T> >& vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        PushWeakObject(ctx, vector[i].Get());
        duk_put_prop_index(ctx, -2, i);
    }
}

/// Push a map of weak-refcounted objects.
template<class T, class U> void PushWeakObjectMap(duk_context* ctx, const Urho3D::HashMap<T, Urho3D::SharedPtr<U> >& map)
{
    duk_push_object(ctx);

    for (typename Urho3D::HashMap<T, Urho3D::SharedPtr<U> >::ConstIterator i = map.Begin(); i != map.End(); ++i)
    {
        PushWeakObject(ctx, i->second_.Get());
        duk_put_prop_string(ctx, -2, Urho3D::String(i->first_).CString());
    }
}

/// Push a map of weak-refcounted objects.
template<class T, class U> void PushWeakObjectMap(duk_context* ctx, const Urho3D::HashMap<T, Urho3D::WeakPtr<U> >& map)
{
    duk_push_object(ctx);

    for (typename Urho3D::HashMap<T, Urho3D::WeakPtr<U> >::ConstIterator i = map.Begin(); i != map.End(); ++i)
    {
        PushWeakObject(ctx, i->second_.Get());
        duk_put_prop_string(ctx, -2, Urho3D::String(i->first_).CString());
    }
}

/// Base class for signal receivers.
class SignalReceiver : public Urho3D::RefCounted
{
public:
    /// Duktape context pointer.
    duk_context* ctx_;
    /// Key (signal pointer) which is used to lookup the receiver on the JS side.
    void* key_;
};

}

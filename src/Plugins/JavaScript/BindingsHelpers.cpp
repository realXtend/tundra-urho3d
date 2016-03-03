// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "BindingsHelpers.h"
#include "Scene/Entity.h"
#include "Scene/IComponent.h"
#include <Urho3D/Core/StringUtils.h>

using namespace Tundra;

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

static duk_ret_t Entity_GetProperty(duk_context* ctx)
{
    /* 'this' binding: handler
     * [0]: target
     * [1]: key
     * [2]: receiver (proxy)
     */
    const char* compTypeName = duk_to_string(ctx, 1);
    // Component properties must be lowercase, to distinguish e.g. between name component, and entity's Name() function
    // (also speeds up entity function calls)
    if (compTypeName && compTypeName[0] >= 'a' && compTypeName[0] <= 'z')
    {
        Entity* entity = GetWeakObject<Entity>(ctx, 0);
        if (entity)
        {
            // Now convert to uppercase so that the type comparision will work
            String compTypeStr(compTypeName);
            compTypeStr[0] = (char)Urho3D::ToUpper(compTypeStr[0]);
            IComponent* comp = entity->Component(compTypeStr);
            if (comp)
            {
                PushWeakObject(ctx, comp);
                return 1;
            }
        }
    }

    // Fallthrough to ordinary properties
    duk_dup(ctx, 1);
    duk_get_prop(ctx, 0);
    return 1;
}

static const duk_function_list_entry EntityProxyFunctions[] = {
    { "get", Entity_GetProperty, 3 },
    { NULL, NULL, 0 }
};

/*
static const duk_function_list_entry ComponentProxyFunctions[] = {
    { "get", Component_GetProperty, 3 },
    { "set", Component_SetProperty, 4 },
    { NULL, NULL, 0 }
};
*/

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

    // Proxied property access handling for entity & component
    if (object->GetType() == Entity::GetTypeStatic())
        SetupProxy(ctx, EntityProxyFunctions);
    /*
    else if (dynamic_cast<IComponent*>(object))
        SetupProxy(ctx, ComponentProxyFunctions);
    */
}

void SetupProxy(duk_context* ctx, const duk_function_list_entry* funcs)
{
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "Proxy");
    duk_dup(ctx, -3); // Duplicate target at stack top
    duk_remove(ctx, -4); // Remove original target
    duk_push_object(ctx); // Handler
    duk_put_function_list(ctx, -1, funcs);
    duk_new(ctx, 2); // Create proxy
    duk_remove(ctx, -2); // Remove global object
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

void PushStringVector(duk_context* ctx, const Urho3D::Vector<Urho3D::String>& vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        duk_push_string(ctx, vector[i].CString());
        duk_put_prop_index(ctx, -2, i);
    }
}

void PushVariant(duk_context* ctx, const Urho3D::Variant& variant)
{
    switch (variant.GetType())
    {
    case Urho3D::VAR_BOOL:
        duk_push_boolean(ctx, variant.GetBool() ? 1 : 0);
        break;
    case Urho3D::VAR_INT:
        duk_push_number(ctx, (duk_double_t)variant.GetInt());
        break;
    case Urho3D::VAR_FLOAT:
        duk_push_number(ctx, variant.GetFloat());
        break;
    case Urho3D::VAR_DOUBLE:
        duk_push_number(ctx, variant.GetFloat());
        break;
    case Urho3D::VAR_STRING:
        duk_push_string(ctx, variant.GetString().CString());
        break;
    default:
        /// \todo More types
        duk_push_null(ctx);
        break;
    }
}

Urho3D::Variant GetVariant(duk_context* ctx, duk_idx_t stackIndex)
{
    if (duk_is_boolean(ctx, stackIndex))
        return Urho3D::Variant(duk_get_boolean(ctx, stackIndex) ? true : false);
    if (duk_is_number(ctx, stackIndex))
        return Urho3D::Variant(duk_get_number(ctx, stackIndex));
    else if (duk_is_string(ctx, stackIndex))
        return Urho3D::Variant(Urho3D::String(duk_get_string(ctx, stackIndex)));
    else
        /// \todo More types
        return Urho3D::Variant();
}

}
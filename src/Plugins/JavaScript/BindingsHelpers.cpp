// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "JavaScriptInstance.h"
#include "Scene/Entity.h"
#include "Scene/IComponent.h"
#include "Scene/Scene.h"
#include "LoggingFunctions.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float4.h"
#include "Math/Quat.h"
#include "Math/Transform.h"
#include "Asset/AssetReference.h"

#include <Urho3D/Core/StringUtils.h>
#include <cstring>

using namespace Tundra;

namespace JSBindings
{

static const char* float2_ID = "float2";
static const char* float3_ID = "float3";
static const char* float4_ID = "float4";
static const char* Quat_ID = "Quat";
static const char* Transform_ID = "Transform";
static const char* AssetReference_ID = "AssetReference";
static const char* AssetReferenceList_ID = "AssetReferenceList";

static duk_ret_t float2_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<float2>(ctx, float2_ID);
    return 0;
}

static duk_ret_t float3_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<float3>(ctx, float3_ID);
    return 0;
}

static duk_ret_t float4_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<float4>(ctx, float4_ID);
    return 0;
}

static duk_ret_t Quat_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<Quat>(ctx, Quat_ID);
    return 0;
}

static duk_ret_t Transform_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<Transform>(ctx, Transform_ID);
    return 0;
}

static duk_ret_t AssetReference_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<AssetReference>(ctx, AssetReference_ID);
    return 0;
}

static duk_ret_t AssetReferenceList_Finalizer(duk_context* ctx)
{
    FinalizeValueObject<AssetReferenceList>(ctx, AssetReferenceList_ID);
    return 0;
}

const char* GetValueObjectType(duk_context* ctx, duk_idx_t stackIndex)
{
    duk_get_prop_string(ctx, stackIndex, "\xff""type");
    const char* objTypeName = nullptr;
    if (duk_is_pointer(ctx, -1))
        objTypeName = (const char*)duk_to_pointer(ctx, -1);
    duk_pop(ctx);
    return objTypeName;
}

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

WeakPtr<Object>* GetWeakPtr(duk_context* ctx, duk_idx_t stackIndex)
{
    if (!duk_is_object(ctx, stackIndex))
        return nullptr;

    WeakPtr<Object>* ptr = nullptr;
    duk_get_prop_string(ctx, stackIndex, "\xff""weak");
    if (duk_is_pointer(ctx, -1))
        ptr = static_cast<WeakPtr<Object>*>(duk_to_pointer(ctx, -1));
    duk_pop(ctx);

    return ptr;
}

static duk_ret_t Scene_GetProperty(duk_context* ctx)
{
    /* 'this' binding: handler
     * [0]: target
     * [1]: key
     * [2]: receiver (proxy)
     */
    const char* subsystemTypeName = duk_to_string(ctx, 1);
    // Subsystem properties must be lowercase to optimize speed, as this is called for every property access.
    if (subsystemTypeName && subsystemTypeName[0] >= 'a' && subsystemTypeName[0] <= 'z')
    {
        Scene* scene = GetWeakObject<Scene>(ctx, 0);
        if (scene)
        {
            SharedPtr<Object> ptr = scene->Subsystem(String(subsystemTypeName));
            if (ptr)
            {
                PushWeakObject(ctx, ptr.Get());
                return 1;
            }
        }
    }

    // Fallthrough to ordinary properties
    duk_dup(ctx, 1);
    duk_get_prop(ctx, 0);
    return 1;
}

static duk_ret_t Entity_GetProperty(duk_context* ctx)
{
    /* 'this' binding: handler
     * [0]: target
     * [1]: key
     * [2]: receiver (proxy)
     */
    const char* compTypeName = duk_to_string(ctx, 1);
    // Component properties must be lowercase to optimize speed, as this is called for every property access.
    // Exception: do not look up Name component, as Entity also has its own "name" property
    if (compTypeName && compTypeName[0] >= 'a' && compTypeName[0] <= 'z' && strcmp(compTypeName, "name"))
    {
        Entity* entity = GetWeakObject<Entity>(ctx, 0);
        if (entity)
        {
            // Now convert to uppercase so that the type comparison will work
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

static duk_ret_t Component_GetProperty(duk_context* ctx)
{
    /* 'this' binding: handler
     * [0]: target
     * [1]: key
     * [2]: receiver (proxy)
     */
    const char* attrName = duk_to_string(ctx, 1);
    if (attrName && attrName[0] >= 'a' && attrName[0] <= 'z')
    {
        IComponent* comp = GetWeakObject<IComponent>(ctx, 0);
        if (comp)
        {
            IAttribute* attr = comp->AttributeById(String(attrName));
            PushAttributeValue(ctx, attr);
            return 1;
        }
    }

    // Fallthrough to ordinary properties
    duk_dup(ctx, 1);
    duk_get_prop(ctx, 0);
    return 1;
}

static duk_ret_t Component_SetProperty(duk_context* ctx)
{
    /* 'this' binding: handler
     * [0]: target
     * [1]: key
     * [2]: val
     * [3]: receiver (proxy)
     */
    const char* attrName = duk_to_string(ctx, 1);
    if (attrName && attrName[0] >= 'a' && attrName[0] <= 'z')
    {
        IComponent* comp = GetWeakObject<IComponent>(ctx, 0);
        if (comp)
        {
            IAttribute* attr = comp->AttributeById(String(attrName));
            AssignAttributeValue(ctx, 2, attr, AttributeChange::Default);
            return 1;
        }
    }

    // Fallthrough to ordinary properties
    duk_dup(ctx, 1);
    duk_dup(ctx, 2);
    duk_put_prop(ctx, 0);
    duk_push_true(ctx);
    return 1;
}

static const duk_function_list_entry SceneProxyFunctions[] = {
    { "get", Scene_GetProperty, 3 },
    { NULL, NULL, 0 }
};

static const duk_function_list_entry EntityProxyFunctions[] = {
    { "get", Entity_GetProperty, 3 },
    { NULL, NULL, 0 }
};

static const duk_function_list_entry ComponentProxyFunctions[] = {
    { "get", Component_GetProperty, 3 },
    { "set", Component_SetProperty, 4 },
    { NULL, NULL, 0 }
};

void PushWeakObject(duk_context* ctx, Object* object)
{
    if (!object)
    {
        duk_push_null(ctx);
        return;
    }

    duk_push_heap_stash(ctx);

    // Check if the wrapper for the object already exists in stash
    // This is required so that comparisons of object references (e.g. against the me property) work properly
    if (duk_has_prop_index(ctx, -1, (size_t)object))
    {
        duk_get_prop_index(ctx, -1, (size_t)object);
        WeakPtr<Object>* oldPtr = GetWeakPtr(ctx, -1);
        if (oldPtr && oldPtr->Get() == object)
        {
            duk_remove(ctx, -2); // Remove stash
            return;
        }
        else
            duk_pop(ctx); // Valid existing wrapper not found
    }

    duk_push_object(ctx);
    WeakPtr<Object>* ptr = new WeakPtr<Object>(object);
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

    // Proxied property access handling for scene, entity & component
    if (object->GetType() == Scene::GetTypeStatic())
        SetupProxy(ctx, SceneProxyFunctions);
    if (object->GetType() == Entity::GetTypeStatic())
        SetupProxy(ctx, EntityProxyFunctions);
    else if (dynamic_cast<IComponent*>(object))
        SetupProxy(ctx, ComponentProxyFunctions);

    // Store to stash
    duk_dup(ctx, -1);
    duk_put_prop_index(ctx, -3, (size_t)object);
    duk_remove(ctx, -2); // Remove stash
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
    WeakPtr<Object>* ptr = GetWeakPtr(ctx, 0);
    if (ptr)
    {
        delete ptr;
        duk_push_pointer(ctx, nullptr);
        duk_put_prop_string(ctx, 0, "\xff""weak");
    }
    return 0;
}

Vector<String> GetStringVector(duk_context* ctx, duk_idx_t stackIndex)
{
    Vector<String> ret;

    if (duk_is_object(ctx, stackIndex))
    {
        duk_size_t len = duk_get_length(ctx, stackIndex);
        for (duk_size_t i = 0; i < len; ++i)
        {
            duk_get_prop_index(ctx, stackIndex, i);
            if (duk_is_string(ctx, -1))
                ret.Push(String(duk_get_string(ctx, -1)));
            duk_pop(ctx);
        }
    }

    return ret;
}

void PushStringVector(duk_context* ctx, const Vector<String>& vector)
{
    duk_push_array(ctx);

    for (unsigned i = 0; i < vector.Size(); ++i)
    {
        duk_push_string(ctx, vector[i].CString());
        duk_put_prop_index(ctx, -2, i);
    }
}

void PushVariant(duk_context* ctx, const Variant& variant)
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
    
    case Urho3D::VAR_VECTOR2:
        PushValueObjectCopy<float2>(ctx, float2(variant.GetVector2()), float2_ID, float2_Finalizer);
        break;
    
    case Urho3D::VAR_VECTOR3:
        PushValueObjectCopy<float3>(ctx, float3(variant.GetVector3()), float3_ID, float3_Finalizer);
        break;
    
    case Urho3D::VAR_VECTOR4:
        PushValueObjectCopy<float4>(ctx, float4(variant.GetVector4()), float4_ID, float4_Finalizer);
        break;
    
    case Urho3D::VAR_QUATERNION:
        PushValueObjectCopy<Quat>(ctx, Quat(variant.GetQuaternion()), Quat_ID, Quat_Finalizer);
        break;
    
    default:
        /// \todo More types
        duk_push_null(ctx);
        break;
    }
}

Variant GetVariant(duk_context* ctx, duk_idx_t stackIndex)
{
    if (duk_is_boolean(ctx, stackIndex))
        return Variant(duk_get_boolean(ctx, stackIndex) ? true : false);
    if (duk_is_number(ctx, stackIndex))
        return Variant(duk_get_number(ctx, stackIndex));
    else if (duk_is_string(ctx, stackIndex))
        return Variant(String(duk_get_string(ctx, stackIndex)));
    else if (duk_is_object(ctx, stackIndex))
    {
        const char* objTypeName = GetValueObjectType(ctx, stackIndex);
        if (objTypeName == float2_ID)
            return Variant(Urho3D::Vector2(*GetValueObject<float2>(ctx, stackIndex, nullptr)));
        else if (objTypeName == float3_ID)
            return Variant(Urho3D::Vector3(*GetValueObject<float3>(ctx, stackIndex, nullptr)));
        else if (objTypeName == float4_ID)
            return Variant(Urho3D::Vector4(*GetValueObject<float4>(ctx, stackIndex, nullptr)));
        else if (objTypeName == Quat_ID)
            return Variant(Urho3D::Quaternion(*GetValueObject<Quat>(ctx, stackIndex, nullptr)));
    }

    /// \todo More types
    return Variant();
}

void AssignAttributeValue(duk_context* ctx, duk_idx_t stackIndex, IAttribute* destAttr, AttributeChange::Type change)
{
    if (!destAttr)
        return;

    switch (destAttr->TypeId())
    {
    case IAttribute::BoolId:
        static_cast<Attribute<bool>*>(destAttr)->Set(duk_get_boolean(ctx, stackIndex) ? true : false, change);
        break;
    
    case IAttribute::IntId:
        static_cast<Attribute<int>*>(destAttr)->Set((int)duk_get_number(ctx, stackIndex), change);
        break;
    
    case IAttribute::UIntId:
        static_cast<Attribute<uint>*>(destAttr)->Set((uint)duk_get_number(ctx, stackIndex), change);
        break;
    
    case IAttribute::RealId:
        static_cast<Attribute<float>*>(destAttr)->Set((float)duk_get_number(ctx, stackIndex), change);
        break;
    
    case IAttribute::StringId:
        static_cast<Attribute<String>*>(destAttr)->Set(String(duk_get_string(ctx, stackIndex)), change);
        break;
    
    case IAttribute::Float2Id:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), float2_ID) == 0)
            static_cast<Attribute<float2>*>(destAttr)->Set(*GetValueObject<float2>(ctx, stackIndex, nullptr), change);
        break;
    
    case IAttribute::Float3Id:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), float3_ID) == 0)
            static_cast<Attribute<float3>*>(destAttr)->Set(*GetValueObject<float3>(ctx, stackIndex, nullptr), change);
        break;
    
    case IAttribute::Float4Id:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), float4_ID) == 0)
            static_cast<Attribute<float4>*>(destAttr)->Set(*GetValueObject<float4>(ctx, stackIndex, nullptr), change);
        break;
    
    case IAttribute::QuatId:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), Quat_ID) == 0)
            static_cast<Attribute<Quat>*>(destAttr)->Set(*GetValueObject<Quat>(ctx, stackIndex, nullptr), change);
        break;

    case IAttribute::TransformId:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), Transform_ID) == 0)
            static_cast<Attribute<Transform>*>(destAttr)->Set(*GetValueObject<Transform>(ctx, stackIndex, nullptr), change);
        break;

    case IAttribute::AssetReferenceId:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), AssetReference_ID) == 0)
            static_cast<Attribute<AssetReference>*>(destAttr)->Set(*GetValueObject<AssetReference>(ctx, stackIndex, nullptr), change);
        break;

    case IAttribute::AssetReferenceListId:
        if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), AssetReferenceList_ID) == 0)
            static_cast<Attribute<AssetReferenceList>*>(destAttr)->Set(*GetValueObject<AssetReferenceList>(ctx, stackIndex, nullptr), change);
        // Also allow assigning a single AssetReference to an AssetReferenceList
        else if (duk_is_object(ctx, stackIndex) && strcmp(GetValueObjectType(ctx, stackIndex), AssetReference_ID) == 0)
        {
            const AssetReference& ref = *GetValueObject<AssetReference>(ctx, stackIndex, nullptr);
            AssetReferenceList list;
            list.type = ref.type;
            list.Append(ref);
            static_cast<Attribute<AssetReferenceList>*>(destAttr)->Set(list);
        }
        break;
    }
}

void PushAttributeValue(duk_context* ctx, IAttribute* attr)
{
    if (attr)
    {
        switch (attr->TypeId())
        {
        case IAttribute::BoolId:
            duk_push_boolean(ctx, static_cast<Attribute<bool>*>(attr)->Get() ? 1 : 0);
            return;
        
        case IAttribute::IntId:
            duk_push_number(ctx, static_cast<Attribute<int>*>(attr)->Get());
            return;
        
        case IAttribute::UIntId:
            duk_push_number(ctx, static_cast<Attribute<uint>*>(attr)->Get());
            return;
        
        case IAttribute::RealId:
            duk_push_number(ctx, static_cast<Attribute<float>*>(attr)->Get());
            return;
        
        case IAttribute::StringId:
            duk_push_string(ctx, static_cast<Attribute<String>*>(attr)->Get().CString());
            return;
        
        case IAttribute::Float2Id:
            PushValueObjectCopy<float2>(ctx, static_cast<Attribute<float2>*>(attr)->Get(), float2_ID, float2_Finalizer);
            return;
        
        case IAttribute::Float3Id:
            PushValueObjectCopy<float3>(ctx, static_cast<Attribute<float3>*>(attr)->Get(), float3_ID, float3_Finalizer);
            return;
        
        case IAttribute::Float4Id:
            PushValueObjectCopy<float4>(ctx, static_cast<Attribute<float4>*>(attr)->Get(), float4_ID, float4_Finalizer);
            return;
        
        case IAttribute::QuatId:
            PushValueObjectCopy<Quat>(ctx, static_cast<Attribute<Quat>*>(attr)->Get(), Quat_ID, Quat_Finalizer);
            return;

        case IAttribute::TransformId:
            PushValueObjectCopy<Transform>(ctx, static_cast<Attribute<Transform>*>(attr)->Get(), Transform_ID, Transform_Finalizer);
            return;

        case IAttribute::AssetReferenceId:
            PushValueObjectCopy<AssetReference>(ctx, static_cast<Attribute<AssetReference>*>(attr)->Get(), AssetReference_ID, AssetReference_Finalizer);
            return;

        case IAttribute::AssetReferenceListId:
            PushValueObjectCopy<AssetReferenceList>(ctx, static_cast<Attribute<AssetReferenceList>*>(attr)->Get(), AssetReferenceList_ID, AssetReferenceList_Finalizer);
            return;

            /// \todo More types
        }
    }

    // Push null if attr was null or type not yet supported
    duk_push_null(ctx);
}

void CallConnectSignal(duk_context* ctx, void* signal)
{
    int numArgs = duk_get_top(ctx);
    duk_push_number(ctx, (size_t)signal);
    duk_insert(ctx, 0);
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "_ConnectSignal");
    duk_remove(ctx, -2); // Global object
    duk_insert(ctx, 0);
    duk_pcall(ctx, numArgs + 1);
    duk_pop(ctx);
}

void CallDisconnectSignal(duk_context* ctx, void* signal)
{
    int numArgs = duk_get_top(ctx);
    duk_push_number(ctx, (size_t)signal);
    duk_insert(ctx, 0);
    duk_push_global_object(ctx);
    duk_get_prop_string(ctx, -1, "_DisconnectSignal");
    duk_remove(ctx, -2); // Global object
    duk_insert(ctx, 0);
    duk_pcall(ctx, numArgs + 1);
    if (duk_get_boolean(ctx, -1)) // Last receiver disconnected
    {
        HashMap<void*, SharedPtr<SignalReceiver> >& signalReceivers = JavaScriptInstance::InstanceFromContext(ctx)->SignalReceivers();
        signalReceivers.Erase(signal);
    }
    duk_pop(ctx); // Result
}

static int GetStackRaw(duk_context *ctx)
{
    if (!duk_is_object(ctx, -1) || !duk_has_prop_string(ctx, -1, "stack") || !duk_is_error(ctx, -1))
        return 1;

    duk_get_prop_string(ctx, -1, "stack");
    duk_remove(ctx, -2);
    return 1;
}

Tundra::String GetErrorString(duk_context* ctx)
{
    duk_safe_call(ctx, GetStackRaw, 1, 1);
    const char* str = duk_safe_to_string(ctx, -1);
    return String(str);
}

}
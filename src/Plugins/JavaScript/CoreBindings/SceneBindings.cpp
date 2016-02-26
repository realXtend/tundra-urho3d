// For conditions of distribution and use, see copyright notice in LICENSE
// This file has been autogenerated with BindingsGenerator

#include "StableHeaders.h"
#include "CoreTypes.h"
#include "BindingsHelpers.h"
#include "Scene/Scene.h"

#ifdef _MSC_VER
#pragma warning(disable: 4800)
#endif

#include "Scene/Entity.h"
#include "Framework/Framework.h"
#include "Scene/IComponent.h"


using namespace Tundra;
using namespace std;

namespace JSBindings
{

extern const char* float3_ID;

duk_ret_t float3_Finalizer(duk_context* ctx);

const char* Scene_ID = "Scene";

const char* SignalWrapper_Scene_ComponentAdded_ID = "SignalWrapper_Scene_ComponentAdded";

class SignalWrapper_Scene_ComponentAdded
{
public:
    SignalWrapper_Scene_ComponentAdded(Urho3D::Object* owner, Signal3< Entity *, IComponent *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal3< Entity *, IComponent *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_ComponentAdded_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentAdded* obj = GetValueObject<SignalWrapper_Scene_ComponentAdded>(ctx, 0, SignalWrapper_Scene_ComponentAdded_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_ComponentAdded_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_ComponentAdded_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentAdded* wrapper = GetThisValueObject<SignalWrapper_Scene_ComponentAdded>(ctx, SignalWrapper_Scene_ComponentAdded_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    IComponent* param1 = GetWeakObject<IComponent>(ctx, 1);
    AttributeChange::Type param2 = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    wrapper->signal_->Emit(param0, param1, param2);
    return 0;
}

static duk_ret_t Scene_Get_ComponentAdded(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_ComponentAdded* wrapper = new SignalWrapper_Scene_ComponentAdded(thisObj, &thisObj->ComponentAdded);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_ComponentAdded_ID, SignalWrapper_Scene_ComponentAdded_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_ComponentAdded_Emit, 3);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_ComponentRemoved_ID = "SignalWrapper_Scene_ComponentRemoved";

class SignalWrapper_Scene_ComponentRemoved
{
public:
    SignalWrapper_Scene_ComponentRemoved(Urho3D::Object* owner, Signal3< Entity *, IComponent *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal3< Entity *, IComponent *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_ComponentRemoved_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentRemoved* obj = GetValueObject<SignalWrapper_Scene_ComponentRemoved>(ctx, 0, SignalWrapper_Scene_ComponentRemoved_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_ComponentRemoved_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_ComponentRemoved_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentRemoved* wrapper = GetThisValueObject<SignalWrapper_Scene_ComponentRemoved>(ctx, SignalWrapper_Scene_ComponentRemoved_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    IComponent* param1 = GetWeakObject<IComponent>(ctx, 1);
    AttributeChange::Type param2 = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    wrapper->signal_->Emit(param0, param1, param2);
    return 0;
}

static duk_ret_t Scene_Get_ComponentRemoved(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_ComponentRemoved* wrapper = new SignalWrapper_Scene_ComponentRemoved(thisObj, &thisObj->ComponentRemoved);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_ComponentRemoved_ID, SignalWrapper_Scene_ComponentRemoved_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_ComponentRemoved_Emit, 3);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_EntityCreated_ID = "SignalWrapper_Scene_EntityCreated";

class SignalWrapper_Scene_EntityCreated
{
public:
    SignalWrapper_Scene_EntityCreated(Urho3D::Object* owner, Signal2< Entity *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal2< Entity *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_EntityCreated_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_EntityCreated* obj = GetValueObject<SignalWrapper_Scene_EntityCreated>(ctx, 0, SignalWrapper_Scene_EntityCreated_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_EntityCreated_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_EntityCreated_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_EntityCreated* wrapper = GetThisValueObject<SignalWrapper_Scene_EntityCreated>(ctx, SignalWrapper_Scene_EntityCreated_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    AttributeChange::Type param1 = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    wrapper->signal_->Emit(param0, param1);
    return 0;
}

static duk_ret_t Scene_Get_EntityCreated(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_EntityCreated* wrapper = new SignalWrapper_Scene_EntityCreated(thisObj, &thisObj->EntityCreated);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_EntityCreated_ID, SignalWrapper_Scene_EntityCreated_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_EntityCreated_Emit, 2);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_EntityRemoved_ID = "SignalWrapper_Scene_EntityRemoved";

class SignalWrapper_Scene_EntityRemoved
{
public:
    SignalWrapper_Scene_EntityRemoved(Urho3D::Object* owner, Signal2< Entity *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal2< Entity *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_EntityRemoved_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_EntityRemoved* obj = GetValueObject<SignalWrapper_Scene_EntityRemoved>(ctx, 0, SignalWrapper_Scene_EntityRemoved_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_EntityRemoved_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_EntityRemoved_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_EntityRemoved* wrapper = GetThisValueObject<SignalWrapper_Scene_EntityRemoved>(ctx, SignalWrapper_Scene_EntityRemoved_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    AttributeChange::Type param1 = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    wrapper->signal_->Emit(param0, param1);
    return 0;
}

static duk_ret_t Scene_Get_EntityRemoved(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_EntityRemoved* wrapper = new SignalWrapper_Scene_EntityRemoved(thisObj, &thisObj->EntityRemoved);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_EntityRemoved_ID, SignalWrapper_Scene_EntityRemoved_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_EntityRemoved_Emit, 2);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_EntityAcked_ID = "SignalWrapper_Scene_EntityAcked";

class SignalWrapper_Scene_EntityAcked
{
public:
    SignalWrapper_Scene_EntityAcked(Urho3D::Object* owner, Signal2< Entity *, entity_id_t >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal2< Entity *, entity_id_t >* signal_;
};

duk_ret_t SignalWrapper_Scene_EntityAcked_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_EntityAcked* obj = GetValueObject<SignalWrapper_Scene_EntityAcked>(ctx, 0, SignalWrapper_Scene_EntityAcked_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_EntityAcked_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_EntityAcked_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_EntityAcked* wrapper = GetThisValueObject<SignalWrapper_Scene_EntityAcked>(ctx, SignalWrapper_Scene_EntityAcked_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    entity_id_t param1 = (entity_id_t)duk_require_number(ctx, 1);
    wrapper->signal_->Emit(param0, param1);
    return 0;
}

static duk_ret_t Scene_Get_EntityAcked(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_EntityAcked* wrapper = new SignalWrapper_Scene_EntityAcked(thisObj, &thisObj->EntityAcked);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_EntityAcked_ID, SignalWrapper_Scene_EntityAcked_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_EntityAcked_Emit, 2);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_EntityTemporaryStateToggled_ID = "SignalWrapper_Scene_EntityTemporaryStateToggled";

class SignalWrapper_Scene_EntityTemporaryStateToggled
{
public:
    SignalWrapper_Scene_EntityTemporaryStateToggled(Urho3D::Object* owner, Signal2< Entity *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal2< Entity *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_EntityTemporaryStateToggled_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_EntityTemporaryStateToggled* obj = GetValueObject<SignalWrapper_Scene_EntityTemporaryStateToggled>(ctx, 0, SignalWrapper_Scene_EntityTemporaryStateToggled_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_EntityTemporaryStateToggled_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_EntityTemporaryStateToggled_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_EntityTemporaryStateToggled* wrapper = GetThisValueObject<SignalWrapper_Scene_EntityTemporaryStateToggled>(ctx, SignalWrapper_Scene_EntityTemporaryStateToggled_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    AttributeChange::Type param1 = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    wrapper->signal_->Emit(param0, param1);
    return 0;
}

static duk_ret_t Scene_Get_EntityTemporaryStateToggled(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_EntityTemporaryStateToggled* wrapper = new SignalWrapper_Scene_EntityTemporaryStateToggled(thisObj, &thisObj->EntityTemporaryStateToggled);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_EntityTemporaryStateToggled_ID, SignalWrapper_Scene_EntityTemporaryStateToggled_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_EntityTemporaryStateToggled_Emit, 2);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_ComponentAcked_ID = "SignalWrapper_Scene_ComponentAcked";

class SignalWrapper_Scene_ComponentAcked
{
public:
    SignalWrapper_Scene_ComponentAcked(Urho3D::Object* owner, Signal2< IComponent *, component_id_t >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal2< IComponent *, component_id_t >* signal_;
};

duk_ret_t SignalWrapper_Scene_ComponentAcked_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentAcked* obj = GetValueObject<SignalWrapper_Scene_ComponentAcked>(ctx, 0, SignalWrapper_Scene_ComponentAcked_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_ComponentAcked_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_ComponentAcked_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_ComponentAcked* wrapper = GetThisValueObject<SignalWrapper_Scene_ComponentAcked>(ctx, SignalWrapper_Scene_ComponentAcked_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    IComponent* param0 = GetWeakObject<IComponent>(ctx, 0);
    component_id_t param1 = (component_id_t)duk_require_number(ctx, 1);
    wrapper->signal_->Emit(param0, param1);
    return 0;
}

static duk_ret_t Scene_Get_ComponentAcked(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_ComponentAcked* wrapper = new SignalWrapper_Scene_ComponentAcked(thisObj, &thisObj->ComponentAcked);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_ComponentAcked_ID, SignalWrapper_Scene_ComponentAcked_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_ComponentAcked_Emit, 2);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_Removed_ID = "SignalWrapper_Scene_Removed";

class SignalWrapper_Scene_Removed
{
public:
    SignalWrapper_Scene_Removed(Urho3D::Object* owner, Signal1< Scene * >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal1< Scene * >* signal_;
};

duk_ret_t SignalWrapper_Scene_Removed_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_Removed* obj = GetValueObject<SignalWrapper_Scene_Removed>(ctx, 0, SignalWrapper_Scene_Removed_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_Removed_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_Removed_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_Removed* wrapper = GetThisValueObject<SignalWrapper_Scene_Removed>(ctx, SignalWrapper_Scene_Removed_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Scene* param0 = GetWeakObject<Scene>(ctx, 0);
    wrapper->signal_->Emit(param0);
    return 0;
}

static duk_ret_t Scene_Get_Removed(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_Removed* wrapper = new SignalWrapper_Scene_Removed(thisObj, &thisObj->Removed);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_Removed_ID, SignalWrapper_Scene_Removed_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_Removed_Emit, 1);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_SceneCleared_ID = "SignalWrapper_Scene_SceneCleared";

class SignalWrapper_Scene_SceneCleared
{
public:
    SignalWrapper_Scene_SceneCleared(Urho3D::Object* owner, Signal1< Scene * >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal1< Scene * >* signal_;
};

duk_ret_t SignalWrapper_Scene_SceneCleared_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_SceneCleared* obj = GetValueObject<SignalWrapper_Scene_SceneCleared>(ctx, 0, SignalWrapper_Scene_SceneCleared_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_SceneCleared_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_SceneCleared_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_SceneCleared* wrapper = GetThisValueObject<SignalWrapper_Scene_SceneCleared>(ctx, SignalWrapper_Scene_SceneCleared_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Scene* param0 = GetWeakObject<Scene>(ctx, 0);
    wrapper->signal_->Emit(param0);
    return 0;
}

static duk_ret_t Scene_Get_SceneCleared(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_SceneCleared* wrapper = new SignalWrapper_Scene_SceneCleared(thisObj, &thisObj->SceneCleared);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_SceneCleared_ID, SignalWrapper_Scene_SceneCleared_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_SceneCleared_Emit, 1);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

const char* SignalWrapper_Scene_EntityParentChanged_ID = "SignalWrapper_Scene_EntityParentChanged";

class SignalWrapper_Scene_EntityParentChanged
{
public:
    SignalWrapper_Scene_EntityParentChanged(Urho3D::Object* owner, Signal3< Entity *, Entity *, AttributeChange::Type >* signal) :
        owner_(owner),
        signal_(signal)
    {
    }

    Urho3D::WeakPtr<Urho3D::Object> owner_;
    Signal3< Entity *, Entity *, AttributeChange::Type >* signal_;
};

duk_ret_t SignalWrapper_Scene_EntityParentChanged_Finalizer(duk_context* ctx)
{
    SignalWrapper_Scene_EntityParentChanged* obj = GetValueObject<SignalWrapper_Scene_EntityParentChanged>(ctx, 0, SignalWrapper_Scene_EntityParentChanged_ID);
    if (obj)
    {
        delete obj;
        SetValueObject(ctx, 0, 0, SignalWrapper_Scene_EntityParentChanged_ID);
    }
    return 0;
}

static duk_ret_t SignalWrapper_Scene_EntityParentChanged_Emit(duk_context* ctx)
{
    SignalWrapper_Scene_EntityParentChanged* wrapper = GetThisValueObject<SignalWrapper_Scene_EntityParentChanged>(ctx, SignalWrapper_Scene_EntityParentChanged_ID);
    if (!wrapper->owner_) return 0; // Check signal owner expiration
    Entity* param0 = GetWeakObject<Entity>(ctx, 0);
    Entity* param1 = GetWeakObject<Entity>(ctx, 1);
    AttributeChange::Type param2 = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    wrapper->signal_->Emit(param0, param1, param2);
    return 0;
}

static duk_ret_t Scene_Get_EntityParentChanged(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    SignalWrapper_Scene_EntityParentChanged* wrapper = new SignalWrapper_Scene_EntityParentChanged(thisObj, &thisObj->EntityParentChanged);
    PushValueObject(ctx, wrapper, SignalWrapper_Scene_EntityParentChanged_ID, SignalWrapper_Scene_EntityParentChanged_Finalizer, false);
    duk_push_c_function(ctx, SignalWrapper_Scene_EntityParentChanged_Emit, 3);
    duk_put_prop_string(ctx, -2, "Emit");
    return 1;
}

static duk_ret_t Scene_Name(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    const String & ret = thisObj->Name();
    duk_push_string(ctx, ret.CString());
    return 1;
}

static duk_ret_t Scene_Entities(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    const Scene::EntityMap & ret = thisObj->Entities();
    PushWeakObjectMap(ctx, ret);
    return 1;
}

static duk_ret_t Scene_ChangeEntityId_entity_id_t_entity_id_t(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t old_id = (entity_id_t)duk_require_number(ctx, 0);
    entity_id_t new_id = (entity_id_t)duk_require_number(ctx, 1);
    thisObj->ChangeEntityId(old_id, new_id);
    return 0;
}

static duk_ret_t Scene_EndAllAttributeInterpolations(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    thisObj->EndAllAttributeInterpolations();
    return 0;
}

static duk_ret_t Scene_UpdateAttributeInterpolations_float(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    float frametime = (float)duk_require_number(ctx, 0);
    thisObj->UpdateAttributeInterpolations(frametime);
    return 0;
}

static duk_ret_t Scene_IsInterpolating(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    bool ret = thisObj->IsInterpolating();
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_GetFramework(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Framework * ret = thisObj->GetFramework();
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EmitComponentAdded_Entity_IComponent_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    IComponent* comp = GetWeakObject<IComponent>(ctx, 1);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    thisObj->EmitComponentAdded(entity, comp, change);
    return 0;
}

static duk_ret_t Scene_EmitComponentRemoved_Entity_IComponent_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    IComponent* comp = GetWeakObject<IComponent>(ctx, 1);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    thisObj->EmitComponentRemoved(entity, comp, change);
    return 0;
}

static duk_ret_t Scene_EmitEntityRemoved_Entity_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    thisObj->EmitEntityRemoved(entity, change);
    return 0;
}

static duk_ret_t Scene_EmitEntityAcked_Entity_entity_id_t(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    entity_id_t oldId = (entity_id_t)duk_require_number(ctx, 1);
    thisObj->EmitEntityAcked(entity, oldId);
    return 0;
}

static duk_ret_t Scene_EmitComponentAcked_IComponent_component_id_t(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    IComponent* component = GetWeakObject<IComponent>(ctx, 0);
    component_id_t oldId = (component_id_t)duk_require_number(ctx, 1);
    thisObj->EmitComponentAcked(component, oldId);
    return 0;
}

static duk_ret_t Scene_EntitiesWithComponent_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String name(duk_require_string(ctx, 0));
    EntityVector ret = thisObj->EntitiesWithComponent(name);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_CreateEntity_entity_id_t_StringVector_AttributeChange__Type_bool_bool_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t id = (entity_id_t)duk_require_number(ctx, 0);
    StringVector components = GetStringVector(ctx, 1);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    bool replicated = duk_require_boolean(ctx, 3);
    bool componentsReplicated = duk_require_boolean(ctx, 4);
    bool temporary = duk_require_boolean(ctx, 5);
    EntityPtr ret = thisObj->CreateEntity(id, components, change, replicated, componentsReplicated, temporary);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_CreateLocalEntity_StringVector_AttributeChange__Type_bool_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    StringVector components = GetStringVector(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    bool componentsReplicated = duk_require_boolean(ctx, 2);
    bool temporary = duk_require_boolean(ctx, 3);
    EntityPtr ret = thisObj->CreateLocalEntity(components, change, componentsReplicated, temporary);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_CreateTemporaryEntity_StringVector_AttributeChange__Type_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    StringVector components = GetStringVector(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    bool componentsReplicated = duk_require_boolean(ctx, 2);
    EntityPtr ret = thisObj->CreateTemporaryEntity(components, change, componentsReplicated);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_CreateLocalTemporaryEntity_StringVector_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    StringVector components = GetStringVector(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    EntityPtr ret = thisObj->CreateLocalTemporaryEntity(components, change);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_UpVector(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    float3 ret = thisObj->UpVector();
    PushValueObjectCopy<float3>(ctx, ret, float3_ID, float3_Finalizer);
    return 1;
}

static duk_ret_t Scene_RightVector(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    float3 ret = thisObj->RightVector();
    PushValueObjectCopy<float3>(ctx, ret, float3_ID, float3_Finalizer);
    return 1;
}

static duk_ret_t Scene_ForwardVector(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    float3 ret = thisObj->ForwardVector();
    PushValueObjectCopy<float3>(ctx, ret, float3_ID, float3_Finalizer);
    return 1;
}

static duk_ret_t Scene_ViewEnabled(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    bool ret = thisObj->ViewEnabled();
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_IsAuthority(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    bool ret = thisObj->IsAuthority();
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntityById_entity_id_t(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t id = (entity_id_t)duk_require_number(ctx, 0);
    EntityPtr ret = thisObj->EntityById(id);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntityByName_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String name(duk_require_string(ctx, 0));
    EntityPtr ret = thisObj->EntityByName(name);
    PushWeakObject(ctx, ret);
    return 1;
}

static duk_ret_t Scene_IsUniqueName_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String name(duk_require_string(ctx, 0));
    bool ret = thisObj->IsUniqueName(name);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_HasEntity_entity_id_t(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t id = (entity_id_t)duk_require_number(ctx, 0);
    bool ret = thisObj->HasEntity(id);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_RemoveEntity_entity_id_t_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t id = (entity_id_t)duk_require_number(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    bool ret = thisObj->RemoveEntity(id, change);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_RemoveAllEntities_bool_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    bool signal = duk_require_boolean(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    thisObj->RemoveAllEntities(signal, change);
    return 0;
}

static duk_ret_t Scene_NextFreeId(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t ret = thisObj->NextFreeId();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Scene_NextFreeIdLocal(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    entity_id_t ret = thisObj->NextFreeIdLocal();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntitiesWithComponent_u32_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    u32 typeId = (u32)duk_require_number(ctx, 0);
    String name(duk_require_string(ctx, 1));
    EntityVector ret = thisObj->EntitiesWithComponent(typeId, name);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntitiesWithComponent_String_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String typeName(duk_require_string(ctx, 0));
    String name(duk_require_string(ctx, 1));
    EntityVector ret = thisObj->EntitiesWithComponent(typeName, name);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntitiesOfGroup_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String groupName(duk_require_string(ctx, 0));
    EntityVector ret = thisObj->EntitiesOfGroup(groupName);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_Components_u32_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    u32 typeId = (u32)duk_require_number(ctx, 0);
    String name(duk_require_string(ctx, 1));
    Entity::ComponentVector ret = thisObj->Components(typeId, name);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_Components_String_String(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String typeName(duk_require_string(ctx, 0));
    String name(duk_require_string(ctx, 1));
    Entity::ComponentVector ret = thisObj->Components(typeName, name);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_FindEntitiesContaining_String_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String substring(duk_require_string(ctx, 0));
    bool caseSensitive = duk_require_boolean(ctx, 1);
    EntityVector ret = thisObj->FindEntitiesContaining(substring, caseSensitive);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_FindEntitiesByName_String_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String name(duk_require_string(ctx, 0));
    bool caseSensitive = duk_require_boolean(ctx, 1);
    EntityVector ret = thisObj->FindEntitiesByName(name, caseSensitive);
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_RootLevelEntities(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    EntityVector ret = thisObj->RootLevelEntities();
    PushWeakObjectVector(ctx, ret);
    return 1;
}

static duk_ret_t Scene_SerializeToXmlString_bool_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    bool serializeTemporary = duk_require_boolean(ctx, 0);
    bool serializeLocal = duk_require_boolean(ctx, 1);
    String ret = thisObj->SerializeToXmlString(serializeTemporary, serializeLocal);
    duk_push_string(ctx, ret.CString());
    return 1;
}

static duk_ret_t Scene_SaveSceneXML_String_bool_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String filename(duk_require_string(ctx, 0));
    bool saveTemporary = duk_require_boolean(ctx, 1);
    bool saveLocal = duk_require_boolean(ctx, 2);
    bool ret = thisObj->SaveSceneXML(filename, saveTemporary, saveLocal);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_SaveSceneBinary_String_bool_bool(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    String filename(duk_require_string(ctx, 0));
    bool saveTemporary = duk_require_boolean(ctx, 1);
    bool saveLocal = duk_require_boolean(ctx, 2);
    bool ret = thisObj->SaveSceneBinary(filename, saveTemporary, saveLocal);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EntityParentId_Entity(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* ent = GetWeakObject<Entity>(ctx, 0);
    entity_id_t ret = thisObj->EntityParentId(ent);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Scene_EmitEntityCreated_Entity_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 1);
    thisObj->EmitEntityCreated(entity, change);
    return 0;
}

static duk_ret_t Scene_EmitEntityParentChanged_Entity_Entity_AttributeChange__Type(duk_context* ctx)
{
    Scene* thisObj = GetThisWeakObject<Scene>(ctx);
    Entity* entity = GetWeakObject<Entity>(ctx, 0);
    Entity* newParent = GetWeakObject<Entity>(ctx, 1);
    AttributeChange::Type change = (AttributeChange::Type)(int)duk_require_number(ctx, 2);
    thisObj->EmitEntityParentChanged(entity, newParent, change);
    return 0;
}

static duk_ret_t Scene_EntitiesWithComponent_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 1 && duk_is_string(ctx, 0))
        return Scene_EntitiesWithComponent_String(ctx);
    if (numArgs == 2 && duk_is_number(ctx, 0) && duk_is_string(ctx, 1))
        return Scene_EntitiesWithComponent_u32_String(ctx);
    if (numArgs == 2 && duk_is_string(ctx, 0) && duk_is_string(ctx, 1))
        return Scene_EntitiesWithComponent_String_String(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Scene_Components_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 2 && duk_is_number(ctx, 0) && duk_is_string(ctx, 1))
        return Scene_Components_u32_String(ctx);
    if (numArgs == 2 && duk_is_string(ctx, 0) && duk_is_string(ctx, 1))
        return Scene_Components_String_String(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static const duk_function_list_entry Scene_Functions[] = {
    {"Name", Scene_Name, 0}
    ,{"Entities", Scene_Entities, 0}
    ,{"ChangeEntityId", Scene_ChangeEntityId_entity_id_t_entity_id_t, 2}
    ,{"EndAllAttributeInterpolations", Scene_EndAllAttributeInterpolations, 0}
    ,{"UpdateAttributeInterpolations", Scene_UpdateAttributeInterpolations_float, 1}
    ,{"IsInterpolating", Scene_IsInterpolating, 0}
    ,{"GetFramework", Scene_GetFramework, 0}
    ,{"EmitComponentAdded", Scene_EmitComponentAdded_Entity_IComponent_AttributeChange__Type, 3}
    ,{"EmitComponentRemoved", Scene_EmitComponentRemoved_Entity_IComponent_AttributeChange__Type, 3}
    ,{"EmitEntityRemoved", Scene_EmitEntityRemoved_Entity_AttributeChange__Type, 2}
    ,{"EmitEntityAcked", Scene_EmitEntityAcked_Entity_entity_id_t, 2}
    ,{"EmitComponentAcked", Scene_EmitComponentAcked_IComponent_component_id_t, 2}
    ,{"EntitiesWithComponent", Scene_EntitiesWithComponent_Selector, DUK_VARARGS}
    ,{"CreateEntity", Scene_CreateEntity_entity_id_t_StringVector_AttributeChange__Type_bool_bool_bool, 6}
    ,{"CreateLocalEntity", Scene_CreateLocalEntity_StringVector_AttributeChange__Type_bool_bool, 4}
    ,{"CreateTemporaryEntity", Scene_CreateTemporaryEntity_StringVector_AttributeChange__Type_bool, 3}
    ,{"CreateLocalTemporaryEntity", Scene_CreateLocalTemporaryEntity_StringVector_AttributeChange__Type, 2}
    ,{"UpVector", Scene_UpVector, 0}
    ,{"RightVector", Scene_RightVector, 0}
    ,{"ForwardVector", Scene_ForwardVector, 0}
    ,{"ViewEnabled", Scene_ViewEnabled, 0}
    ,{"IsAuthority", Scene_IsAuthority, 0}
    ,{"EntityById", Scene_EntityById_entity_id_t, 1}
    ,{"EntityByName", Scene_EntityByName_String, 1}
    ,{"IsUniqueName", Scene_IsUniqueName_String, 1}
    ,{"HasEntity", Scene_HasEntity_entity_id_t, 1}
    ,{"RemoveEntity", Scene_RemoveEntity_entity_id_t_AttributeChange__Type, 2}
    ,{"RemoveAllEntities", Scene_RemoveAllEntities_bool_AttributeChange__Type, 2}
    ,{"NextFreeId", Scene_NextFreeId, 0}
    ,{"NextFreeIdLocal", Scene_NextFreeIdLocal, 0}
    ,{"EntitiesOfGroup", Scene_EntitiesOfGroup_String, 1}
    ,{"Components", Scene_Components_Selector, DUK_VARARGS}
    ,{"FindEntitiesContaining", Scene_FindEntitiesContaining_String_bool, 2}
    ,{"FindEntitiesByName", Scene_FindEntitiesByName_String_bool, 2}
    ,{"RootLevelEntities", Scene_RootLevelEntities, 0}
    ,{"SerializeToXmlString", Scene_SerializeToXmlString_bool_bool, 2}
    ,{"SaveSceneXML", Scene_SaveSceneXML_String_bool_bool, 3}
    ,{"SaveSceneBinary", Scene_SaveSceneBinary_String_bool_bool, 3}
    ,{"EntityParentId", Scene_EntityParentId_Entity, 1}
    ,{"EmitEntityCreated", Scene_EmitEntityCreated_Entity_AttributeChange__Type, 2}
    ,{"EmitEntityParentChanged", Scene_EmitEntityParentChanged_Entity_Entity_AttributeChange__Type, 3}
    ,{nullptr, nullptr, 0}
};

void Expose_Scene(duk_context* ctx)
{
    duk_push_object(ctx);
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, Scene_Functions);
    DefineProperty(ctx, "componentAdded", Scene_Get_ComponentAdded, nullptr);
    DefineProperty(ctx, "componentRemoved", Scene_Get_ComponentRemoved, nullptr);
    DefineProperty(ctx, "entityCreated", Scene_Get_EntityCreated, nullptr);
    DefineProperty(ctx, "entityRemoved", Scene_Get_EntityRemoved, nullptr);
    DefineProperty(ctx, "entityAcked", Scene_Get_EntityAcked, nullptr);
    DefineProperty(ctx, "entityTemporaryStateToggled", Scene_Get_EntityTemporaryStateToggled, nullptr);
    DefineProperty(ctx, "componentAcked", Scene_Get_ComponentAcked, nullptr);
    DefineProperty(ctx, "removed", Scene_Get_Removed, nullptr);
    DefineProperty(ctx, "sceneCleared", Scene_Get_SceneCleared, nullptr);
    DefineProperty(ctx, "entityParentChanged", Scene_Get_EntityParentChanged, nullptr);
    duk_put_prop_string(ctx, -2, "prototype");
    duk_put_global_string(ctx, Scene_ID);
}

}

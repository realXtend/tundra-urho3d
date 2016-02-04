// For conditions of distribution and use, see copyright notice in LICENSE
// This file has been autogenerated with BindingsGenerator

#include "StableHeaders.h"
#include "BindingsHelpers.h"
#include "Math/Quat.h"
#include "Math/float3x3.h"
#include "Math/float3x4.h"
#include "Math/float4x4.h"
#include "Math/float3.h"
#include "Math/float4.h"
#include "Algorithm/Random/LCG.h"

namespace JSBindings
{

extern const char* float3x3_Id;
extern const char* float3x4_Id;
extern const char* float4x4_Id;
extern const char* float3_Id;
extern const char* float4_Id;
extern const char* LCG_Id;

duk_ret_t float3x3_Dtor(duk_context* ctx);
duk_ret_t float3x4_Dtor(duk_context* ctx);
duk_ret_t float4x4_Dtor(duk_context* ctx);
duk_ret_t float3_Dtor(duk_context* ctx);
duk_ret_t float4_Dtor(duk_context* ctx);
duk_ret_t LCG_Dtor(duk_context* ctx);

const char* Quat_Id = "Quat";

duk_ret_t Quat_Dtor(duk_context* ctx)
{
    Quat* obj = GetObject<Quat>(ctx, 0, Quat_Id);
    if (obj)
    {
        delete obj;
        SetObject(ctx, 0, 0, Quat_Id);
    }
    return 0;
}

static duk_ret_t Quat_Set_x(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float x = (float)duk_require_number(ctx, 0);
    thisObj->x = x;
    return 0;
}

static duk_ret_t Quat_Get_x(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    duk_push_number(ctx, thisObj->x);
    return 1;
}

static duk_ret_t Quat_Set_y(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float y = (float)duk_require_number(ctx, 0);
    thisObj->y = y;
    return 0;
}

static duk_ret_t Quat_Get_y(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    duk_push_number(ctx, thisObj->y);
    return 1;
}

static duk_ret_t Quat_Set_z(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float z = (float)duk_require_number(ctx, 0);
    thisObj->z = z;
    return 0;
}

static duk_ret_t Quat_Get_z(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    duk_push_number(ctx, thisObj->z);
    return 1;
}

static duk_ret_t Quat_Set_w(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float w = (float)duk_require_number(ctx, 0);
    thisObj->w = w;
    return 0;
}

static duk_ret_t Quat_Get_w(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    duk_push_number(ctx, thisObj->w);
    return 1;
}

static duk_ret_t Quat_Ctor(duk_context* ctx)
{
    Quat* newObj = new Quat();
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_Quat(duk_context* ctx)
{
    Quat* rhs = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    Quat* newObj = new Quat(*rhs);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float3x3(duk_context* ctx)
{
    float3x3* rotationMatrix = GetCheckedObject<float3x3>(ctx, 0, float3x3_Id);
    Quat* newObj = new Quat(*rotationMatrix);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float3x4(duk_context* ctx)
{
    float3x4* rotationMatrix = GetCheckedObject<float3x4>(ctx, 0, float3x4_Id);
    Quat* newObj = new Quat(*rotationMatrix);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float4x4(duk_context* ctx)
{
    float4x4* rotationMatrix = GetCheckedObject<float4x4>(ctx, 0, float4x4_Id);
    Quat* newObj = new Quat(*rotationMatrix);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float_float_float_float(duk_context* ctx)
{
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    float w = (float)duk_require_number(ctx, 3);
    Quat* newObj = new Quat(x, y, z, w);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float3_float(duk_context* ctx)
{
    float3* rotationAxis = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    Quat* newObj = new Quat(*rotationAxis, rotationAngleRadians);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Ctor_float4_float(duk_context* ctx)
{
    float4* rotationAxis = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    Quat* newObj = new Quat(*rotationAxis, rotationAngleRadians);
    PushConstructorResult<Quat>(ctx, newObj, Quat_Id, Quat_Dtor);
    return 0;
}

static duk_ret_t Quat_Angle(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float ret = thisObj->Angle();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Dot_Quat(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* rhs = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    float ret = thisObj->Dot(*rhs);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_LengthSq(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float ret = thisObj->LengthSq();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Length(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float ret = thisObj->Length();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Normalize(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float ret = thisObj->Normalize();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Normalized(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat ret = thisObj->Normalized();
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_IsNormalized_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float epsilon = (float)duk_require_number(ctx, 0);
    bool ret = thisObj->IsNormalized(epsilon);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Quat_IsInvertible_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float epsilon = (float)duk_require_number(ctx, 0);
    bool ret = thisObj->IsInvertible(epsilon);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Quat_IsFinite(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    bool ret = thisObj->IsFinite();
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Equals_Quat_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* rhs = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    float epsilon = (float)duk_require_number(ctx, 1);
    bool ret = thisObj->Equals(*rhs, epsilon);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Quat_BitEquals_Quat(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* other = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    bool ret = thisObj->BitEquals(*other);
    duk_push_boolean(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Inverse(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    thisObj->Inverse();
    return 0;
}

static duk_ret_t Quat_Inverted(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat ret = thisObj->Inverted();
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_InverseAndNormalize(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float ret = thisObj->InverseAndNormalize();
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_Conjugate(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    thisObj->Conjugate();
    return 0;
}

static duk_ret_t Quat_Conjugated(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat ret = thisObj->Conjugated();
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_Transform_float_float_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    float3 ret = thisObj->Transform(x, y, z);
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_Transform_float3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3* vec = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3 ret = thisObj->Transform(*vec);
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_Transform_float4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4* vec = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float4 ret = thisObj->Transform(*vec);
    PushValueObjectCopy<float4>(ctx, ret, float4_Id, float4_Dtor);
    return 1;
}

static duk_ret_t Quat_Lerp_Quat_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* target = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    float t = (float)duk_require_number(ctx, 1);
    Quat ret = thisObj->Lerp(*target, t);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_Slerp_Quat_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* target = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    float t = (float)duk_require_number(ctx, 1);
    Quat ret = thisObj->Slerp(*target, t);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_AngleBetween_Quat(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* target = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
     float ret = thisObj->AngleBetween(*target);
    duk_push_number(ctx, ret);
    return 1;
}

static duk_ret_t Quat_ToAxisAngle_float3_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3* rotationAxis = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    thisObj->ToAxisAngle(*rotationAxis, rotationAngleRadians);
    return 0;
}

static duk_ret_t Quat_ToAxisAngle_float4_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4* rotationAxis = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    thisObj->ToAxisAngle(*rotationAxis, rotationAngleRadians);
    return 0;
}

static duk_ret_t Quat_SetFromAxisAngle_float3_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3* rotationAxis = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    thisObj->SetFromAxisAngle(*rotationAxis, rotationAngleRadians);
    return 0;
}

static duk_ret_t Quat_SetFromAxisAngle_float4_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4* rotationAxis = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float rotationAngleRadians = (float)duk_require_number(ctx, 1);
    thisObj->SetFromAxisAngle(*rotationAxis, rotationAngleRadians);
    return 0;
}

static duk_ret_t Quat_Set_float3x3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3x3* matrix = GetCheckedObject<float3x3>(ctx, 0, float3x3_Id);
    thisObj->Set(*matrix);
    return 0;
}

static duk_ret_t Quat_Set_float3x4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3x4* matrix = GetCheckedObject<float3x4>(ctx, 0, float3x4_Id);
    thisObj->Set(*matrix);
    return 0;
}

static duk_ret_t Quat_Set_float4x4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4x4* matrix = GetCheckedObject<float4x4>(ctx, 0, float4x4_Id);
    thisObj->Set(*matrix);
    return 0;
}

static duk_ret_t Quat_Set_float_float_float_float(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    float w = (float)duk_require_number(ctx, 3);
    thisObj->Set(x, y, z, w);
    return 0;
}

static duk_ret_t Quat_ToEulerXYX(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerXYX();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerXZX(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerXZX();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerYXY(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerYXY();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerYZY(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerYZY();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerZXZ(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerZXZ();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerZYZ(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerZYZ();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerXYZ(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerXYZ();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerXZY(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerXZY();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerYXZ(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerYXZ();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerYZX(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerYZX();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerZXY(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerZXY();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToEulerZYX(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3 ret = thisObj->ToEulerZYX();
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToFloat3x3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3x3 ret = thisObj->ToFloat3x3();
    PushValueObjectCopy<float3x3>(ctx, ret, float3x3_Id, float3x3_Dtor);
    return 1;
}

static duk_ret_t Quat_ToFloat3x4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3x4 ret = thisObj->ToFloat3x4();
    PushValueObjectCopy<float3x4>(ctx, ret, float3x4_Id, float3x4_Dtor);
    return 1;
}

static duk_ret_t Quat_ToFloat4x4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4x4 ret = thisObj->ToFloat4x4();
    PushValueObjectCopy<float4x4>(ctx, ret, float4x4_Id, float4x4_Dtor);
    return 1;
}

static duk_ret_t Quat_ToFloat4x4_float3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3* translation = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float4x4 ret = thisObj->ToFloat4x4(*translation);
    PushValueObjectCopy<float4x4>(ctx, ret, float4x4_Id, float4x4_Dtor);
    return 1;
}

static duk_ret_t Quat_ToFloat4x4_float4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4* translation = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float4x4 ret = thisObj->ToFloat4x4(*translation);
    PushValueObjectCopy<float4x4>(ctx, ret, float4x4_Id, float4x4_Dtor);
    return 1;
}

static duk_ret_t Quat_CastToFloat4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4 ret = thisObj->CastToFloat4();
    PushValueObjectCopy<float4>(ctx, ret, float4_Id, float4_Dtor);
    return 1;
}

static duk_ret_t Quat_ToString(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    std::string  ret = thisObj->ToString();
    duk_push_string(ctx, ret.c_str());
    return 1;
}

static duk_ret_t Quat_ToString2(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    std::string  ret = thisObj->ToString2();
    duk_push_string(ctx, ret.c_str());
    return 1;
}

static duk_ret_t Quat_SerializeToString(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    std::string  ret = thisObj->SerializeToString();
    duk_push_string(ctx, ret.c_str());
    return 1;
}

static duk_ret_t Quat_SerializeToCodeString(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    std::string ret = thisObj->SerializeToCodeString();
    duk_push_string(ctx, ret.c_str());
    return 1;
}

static duk_ret_t Quat_Mul_Quat(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat* rhs = GetCheckedObject<Quat>(ctx, 0, Quat_Id);
    Quat ret = thisObj->Mul(*rhs);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_Mul_float3x3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3x3* rhs = GetCheckedObject<float3x3>(ctx, 0, float3x3_Id);
    Quat ret = thisObj->Mul(*rhs);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_Mul_float3(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float3* vector = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3 ret = thisObj->Mul(*vector);
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_Mul_float4(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    float4* vector = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float4 ret = thisObj->Mul(*vector);
    PushValueObjectCopy<float4>(ctx, ret, float4_Id, float4_Dtor);
    return 1;
}

static duk_ret_t Quat_Neg(duk_context* ctx)
{
    Quat* thisObj = GetThisObject<Quat>(ctx, Quat_Id);
    Quat ret = thisObj->Neg();
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_Ctor_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 0)
        return Quat_Ctor(ctx);
    if (numArgs == 1 && GetObject<Quat>(ctx, 0, Quat_Id))
        return Quat_Ctor_Quat(ctx);
    if (numArgs == 1 && GetObject<float3x3>(ctx, 0, float3x3_Id))
        return Quat_Ctor_float3x3(ctx);
    if (numArgs == 1 && GetObject<float3x4>(ctx, 0, float3x4_Id))
        return Quat_Ctor_float3x4(ctx);
    if (numArgs == 1 && GetObject<float4x4>(ctx, 0, float4x4_Id))
        return Quat_Ctor_float4x4(ctx);
    if (numArgs == 4 && duk_is_number(ctx, 0) && duk_is_number(ctx, 1) && duk_is_number(ctx, 2) && duk_is_number(ctx, 3))
        return Quat_Ctor_float_float_float_float(ctx);
    if (numArgs == 2 && GetObject<float3>(ctx, 0, float3_Id) && duk_is_number(ctx, 1))
        return Quat_Ctor_float3_float(ctx);
    if (numArgs == 2 && GetObject<float4>(ctx, 0, float4_Id) && duk_is_number(ctx, 1))
        return Quat_Ctor_float4_float(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_Transform_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 3 && duk_is_number(ctx, 0) && duk_is_number(ctx, 1) && duk_is_number(ctx, 2))
        return Quat_Transform_float_float_float(ctx);
    if (numArgs == 1 && GetObject<float3>(ctx, 0, float3_Id))
        return Quat_Transform_float3(ctx);
    if (numArgs == 1 && GetObject<float4>(ctx, 0, float4_Id))
        return Quat_Transform_float4(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_ToAxisAngle_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 2 && GetObject<float3>(ctx, 0, float3_Id) && duk_is_number(ctx, 1))
        return Quat_ToAxisAngle_float3_float(ctx);
    if (numArgs == 2 && GetObject<float4>(ctx, 0, float4_Id) && duk_is_number(ctx, 1))
        return Quat_ToAxisAngle_float4_float(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_SetFromAxisAngle_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 2 && GetObject<float3>(ctx, 0, float3_Id) && duk_is_number(ctx, 1))
        return Quat_SetFromAxisAngle_float3_float(ctx);
    if (numArgs == 2 && GetObject<float4>(ctx, 0, float4_Id) && duk_is_number(ctx, 1))
        return Quat_SetFromAxisAngle_float4_float(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_Set_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 1 && GetObject<float3x3>(ctx, 0, float3x3_Id))
        return Quat_Set_float3x3(ctx);
    if (numArgs == 1 && GetObject<float3x4>(ctx, 0, float3x4_Id))
        return Quat_Set_float3x4(ctx);
    if (numArgs == 1 && GetObject<float4x4>(ctx, 0, float4x4_Id))
        return Quat_Set_float4x4(ctx);
    if (numArgs == 4 && duk_is_number(ctx, 0) && duk_is_number(ctx, 1) && duk_is_number(ctx, 2) && duk_is_number(ctx, 3))
        return Quat_Set_float_float_float_float(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_ToFloat4x4_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 0)
        return Quat_ToFloat4x4(ctx);
    if (numArgs == 1 && GetObject<float3>(ctx, 0, float3_Id))
        return Quat_ToFloat4x4_float3(ctx);
    if (numArgs == 1 && GetObject<float4>(ctx, 0, float4_Id))
        return Quat_ToFloat4x4_float4(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_Mul_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 1 && GetObject<Quat>(ctx, 0, Quat_Id))
        return Quat_Mul_Quat(ctx);
    if (numArgs == 1 && GetObject<float3x3>(ctx, 0, float3x3_Id))
        return Quat_Mul_float3x3(ctx);
    if (numArgs == 1 && GetObject<float3>(ctx, 0, float3_Id))
        return Quat_Mul_float3(ctx);
    if (numArgs == 1 && GetObject<float4>(ctx, 0, float4_Id))
        return Quat_Mul_float4(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static duk_ret_t Quat_SlerpVector_Static_float3_float3_float(duk_context* ctx)
{
    float3* from = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3* to = GetCheckedObject<float3>(ctx, 1, float3_Id);
    float t = (float)duk_require_number(ctx, 2);
    float3 ret = Quat::SlerpVector(*from, *to, t);
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_SlerpVectorAbs_Static_float3_float3_float(duk_context* ctx)
{
    float3* from = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3* to = GetCheckedObject<float3>(ctx, 1, float3_Id);
    float angleRadians = (float)duk_require_number(ctx, 2);
    float3 ret = Quat::SlerpVectorAbs(*from, *to, angleRadians);
    PushValueObjectCopy<float3>(ctx, ret, float3_Id, float3_Dtor);
    return 1;
}

static duk_ret_t Quat_LookAt_Static_float3_float3_float3_float3(duk_context* ctx)
{
    float3* localForward = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3* targetDirection = GetCheckedObject<float3>(ctx, 1, float3_Id);
    float3* localUp = GetCheckedObject<float3>(ctx, 2, float3_Id);
    float3* worldUp = GetCheckedObject<float3>(ctx, 3, float3_Id);
    Quat ret = Quat::LookAt(*localForward, *targetDirection, *localUp, *worldUp);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateX_Static_float(duk_context* ctx)
{
    float angleRadians = (float)duk_require_number(ctx, 0);
    Quat ret = Quat::RotateX(angleRadians);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateY_Static_float(duk_context* ctx)
{
    float angleRadians = (float)duk_require_number(ctx, 0);
    Quat ret = Quat::RotateY(angleRadians);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateZ_Static_float(duk_context* ctx)
{
    float angleRadians = (float)duk_require_number(ctx, 0);
    Quat ret = Quat::RotateZ(angleRadians);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateAxisAngle_Static_float3_float(duk_context* ctx)
{
    float3* axisDirection = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float angleRadians = (float)duk_require_number(ctx, 1);
    Quat ret = Quat::RotateAxisAngle(*axisDirection, angleRadians);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateFromTo_Static_float3_float3(duk_context* ctx)
{
    float3* sourceDirection = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3* targetDirection = GetCheckedObject<float3>(ctx, 1, float3_Id);
    Quat ret = Quat::RotateFromTo(*sourceDirection, *targetDirection);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateFromTo_Static_float4_float4(duk_context* ctx)
{
    float4* sourceDirection = GetCheckedObject<float4>(ctx, 0, float4_Id);
    float4* targetDirection = GetCheckedObject<float4>(ctx, 1, float4_Id);
    Quat ret = Quat::RotateFromTo(*sourceDirection, *targetDirection);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateFromTo_Static_float3_float3_float3_float3(duk_context* ctx)
{
    float3* sourceDirection = GetCheckedObject<float3>(ctx, 0, float3_Id);
    float3* targetDirection = GetCheckedObject<float3>(ctx, 1, float3_Id);
    float3* sourceDirection2 = GetCheckedObject<float3>(ctx, 2, float3_Id);
    float3* targetDirection2 = GetCheckedObject<float3>(ctx, 3, float3_Id);
    Quat ret = Quat::RotateFromTo(*sourceDirection, *targetDirection, *sourceDirection2, *targetDirection2);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerXYX_Static_float_float_float(duk_context* ctx)
{
    float x2 = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float x = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerXYX(x2, y, x);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerXZX_Static_float_float_float(duk_context* ctx)
{
    float x2 = (float)duk_require_number(ctx, 0);
    float z = (float)duk_require_number(ctx, 1);
    float x = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerXZX(x2, z, x);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerYXY_Static_float_float_float(duk_context* ctx)
{
    float y2 = (float)duk_require_number(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerYXY(y2, x, y);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerYZY_Static_float_float_float(duk_context* ctx)
{
    float y2 = (float)duk_require_number(ctx, 0);
    float z = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerYZY(y2, z, y);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerZXZ_Static_float_float_float(duk_context* ctx)
{
    float z2 = (float)duk_require_number(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerZXZ(z2, x, z);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerZYZ_Static_float_float_float(duk_context* ctx)
{
    float z2 = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerZYZ(z2, y, z);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerXYZ_Static_float_float_float(duk_context* ctx)
{
    float x = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerXYZ(x, y, z);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerXZY_Static_float_float_float(duk_context* ctx)
{
    float x = (float)duk_require_number(ctx, 0);
    float z = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerXZY(x, z, y);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerYXZ_Static_float_float_float(duk_context* ctx)
{
    float y = (float)duk_require_number(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float z = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerYXZ(y, x, z);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerYZX_Static_float_float_float(duk_context* ctx)
{
    float y = (float)duk_require_number(ctx, 0);
    float z = (float)duk_require_number(ctx, 1);
    float x = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerYZX(y, z, x);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerZXY_Static_float_float_float(duk_context* ctx)
{
    float z = (float)duk_require_number(ctx, 0);
    float x = (float)duk_require_number(ctx, 1);
    float y = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerZXY(z, x, y);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromEulerZYX_Static_float_float_float(duk_context* ctx)
{
    float z = (float)duk_require_number(ctx, 0);
    float y = (float)duk_require_number(ctx, 1);
    float x = (float)duk_require_number(ctx, 2);
    Quat ret = Quat::FromEulerZYX(z, y, x);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RandomRotation_Static_LCG(duk_context* ctx)
{
    LCG* lcg = GetCheckedObject<LCG>(ctx, 0, LCG_Id);
    Quat ret = Quat::RandomRotation(*lcg);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_FromString_Static_std__string(duk_context* ctx)
{
    std::string str = std::string(duk_require_string(ctx, 0));
    Quat ret = Quat::FromString(str);
    PushValueObjectCopy<Quat>(ctx, ret, Quat_Id, Quat_Dtor);
    return 1;
}

static duk_ret_t Quat_RotateFromTo_Static_Selector(duk_context* ctx)
{
    int numArgs = duk_get_top(ctx);
    if (numArgs == 2 && GetObject<float3>(ctx, 0, float3_Id) && GetObject<float3>(ctx, 1, float3_Id))
        return Quat_RotateFromTo_Static_float3_float3(ctx);
    if (numArgs == 2 && GetObject<float4>(ctx, 0, float4_Id) && GetObject<float4>(ctx, 1, float4_Id))
        return Quat_RotateFromTo_Static_float4_float4(ctx);
    if (numArgs == 4 && GetObject<float3>(ctx, 0, float3_Id) && GetObject<float3>(ctx, 1, float3_Id) && GetObject<float3>(ctx, 2, float3_Id) && GetObject<float3>(ctx, 3, float3_Id))
        return Quat_RotateFromTo_Static_float3_float3_float3_float3(ctx);
    duk_error(ctx, DUK_ERR_ERROR, "Could not select function overload");
}

static const duk_function_list_entry Quat_Functions[] = {
    {"Angle", Quat_Angle, 0}
    ,{"Dot", Quat_Dot_Quat, 1}
    ,{"LengthSq", Quat_LengthSq, 0}
    ,{"Length", Quat_Length, 0}
    ,{"Normalize", Quat_Normalize, 0}
    ,{"Normalized", Quat_Normalized, 0}
    ,{"IsNormalized", Quat_IsNormalized_float, 1}
    ,{"IsInvertible", Quat_IsInvertible_float, 1}
    ,{"IsFinite", Quat_IsFinite, 0}
    ,{"Equals", Quat_Equals_Quat_float, 2}
    ,{"BitEquals", Quat_BitEquals_Quat, 1}
    ,{"Inverse", Quat_Inverse, 0}
    ,{"Inverted", Quat_Inverted, 0}
    ,{"InverseAndNormalize", Quat_InverseAndNormalize, 0}
    ,{"Conjugate", Quat_Conjugate, 0}
    ,{"Conjugated", Quat_Conjugated, 0}
    ,{"Transform", Quat_Transform_Selector, DUK_VARARGS}
    ,{"Lerp", Quat_Lerp_Quat_float, 2}
    ,{"Slerp", Quat_Slerp_Quat_float, 2}
    ,{"AngleBetween", Quat_AngleBetween_Quat, 1}
    ,{"ToAxisAngle", Quat_ToAxisAngle_Selector, DUK_VARARGS}
    ,{"SetFromAxisAngle", Quat_SetFromAxisAngle_Selector, DUK_VARARGS}
    ,{"Set", Quat_Set_Selector, DUK_VARARGS}
    ,{"ToEulerXYX", Quat_ToEulerXYX, 0}
    ,{"ToEulerXZX", Quat_ToEulerXZX, 0}
    ,{"ToEulerYXY", Quat_ToEulerYXY, 0}
    ,{"ToEulerYZY", Quat_ToEulerYZY, 0}
    ,{"ToEulerZXZ", Quat_ToEulerZXZ, 0}
    ,{"ToEulerZYZ", Quat_ToEulerZYZ, 0}
    ,{"ToEulerXYZ", Quat_ToEulerXYZ, 0}
    ,{"ToEulerXZY", Quat_ToEulerXZY, 0}
    ,{"ToEulerYXZ", Quat_ToEulerYXZ, 0}
    ,{"ToEulerYZX", Quat_ToEulerYZX, 0}
    ,{"ToEulerZXY", Quat_ToEulerZXY, 0}
    ,{"ToEulerZYX", Quat_ToEulerZYX, 0}
    ,{"ToFloat3x3", Quat_ToFloat3x3, 0}
    ,{"ToFloat3x4", Quat_ToFloat3x4, 0}
    ,{"ToFloat4x4", Quat_ToFloat4x4_Selector, DUK_VARARGS}
    ,{"CastToFloat4", Quat_CastToFloat4, 0}
    ,{"ToString", Quat_ToString, 0}
    ,{"ToString2", Quat_ToString2, 0}
    ,{"SerializeToString", Quat_SerializeToString, 0}
    ,{"SerializeToCodeString", Quat_SerializeToCodeString, 0}
    ,{"Mul", Quat_Mul_Selector, DUK_VARARGS}
    ,{"Neg", Quat_Neg, 0}
    ,{nullptr, nullptr, 0}
};

static const duk_function_list_entry Quat_StaticFunctions[] = {
    {"SlerpVector", Quat_SlerpVector_Static_float3_float3_float, 3}
    ,{"SlerpVectorAbs", Quat_SlerpVectorAbs_Static_float3_float3_float, 3}
    ,{"LookAt", Quat_LookAt_Static_float3_float3_float3_float3, 4}
    ,{"RotateX", Quat_RotateX_Static_float, 1}
    ,{"RotateY", Quat_RotateY_Static_float, 1}
    ,{"RotateZ", Quat_RotateZ_Static_float, 1}
    ,{"RotateAxisAngle", Quat_RotateAxisAngle_Static_float3_float, 2}
    ,{"RotateFromTo", Quat_RotateFromTo_Static_Selector, DUK_VARARGS}
    ,{"FromEulerXYX", Quat_FromEulerXYX_Static_float_float_float, 3}
    ,{"FromEulerXZX", Quat_FromEulerXZX_Static_float_float_float, 3}
    ,{"FromEulerYXY", Quat_FromEulerYXY_Static_float_float_float, 3}
    ,{"FromEulerYZY", Quat_FromEulerYZY_Static_float_float_float, 3}
    ,{"FromEulerZXZ", Quat_FromEulerZXZ_Static_float_float_float, 3}
    ,{"FromEulerZYZ", Quat_FromEulerZYZ_Static_float_float_float, 3}
    ,{"FromEulerXYZ", Quat_FromEulerXYZ_Static_float_float_float, 3}
    ,{"FromEulerXZY", Quat_FromEulerXZY_Static_float_float_float, 3}
    ,{"FromEulerYXZ", Quat_FromEulerYXZ_Static_float_float_float, 3}
    ,{"FromEulerYZX", Quat_FromEulerYZX_Static_float_float_float, 3}
    ,{"FromEulerZXY", Quat_FromEulerZXY_Static_float_float_float, 3}
    ,{"FromEulerZYX", Quat_FromEulerZYX_Static_float_float_float, 3}
    ,{"RandomRotation", Quat_RandomRotation_Static_LCG, 1}
    ,{"FromString", Quat_FromString_Static_std__string, 1}
    ,{nullptr, nullptr, 0}
};

void Expose_Quat(duk_context* ctx)
{
    duk_push_c_function(ctx, Quat_Ctor_Selector, DUK_VARARGS);
    duk_put_function_list(ctx, -1, Quat_StaticFunctions);
    duk_push_object(ctx);
    duk_put_function_list(ctx, -1, Quat_Functions);
    DefineProperty(ctx, "x", Quat_Get_x, Quat_Set_x);
    DefineProperty(ctx, "y", Quat_Get_y, Quat_Set_y);
    DefineProperty(ctx, "z", Quat_Get_z, Quat_Set_z);
    DefineProperty(ctx, "w", Quat_Get_w, Quat_Set_w);
    duk_put_prop_string(ctx, -2, "prototype");
    duk_put_global_string(ctx, Quat_Id);
}

}

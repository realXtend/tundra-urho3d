// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "MathBindings.h"

namespace JSBindings
{

void Expose_AABB(duk_context* ctx);
void Expose_Capsule(duk_context* ctx);
void Expose_Circle(duk_context* ctx);
void Expose_float2(duk_context* ctx);
void Expose_float3(duk_context* ctx);
void Expose_float3x3(duk_context* ctx);
void Expose_float3x4(duk_context* ctx);
void Expose_float4(duk_context* ctx);
void Expose_float4x4(duk_context* ctx);
void Expose_Frustum(duk_context* ctx);
void Expose_LCG(duk_context* ctx);
void Expose_Line(duk_context* ctx);
void Expose_LineSegment(duk_context* ctx);
void Expose_OBB(duk_context* ctx);
void Expose_Plane(duk_context* ctx);
void Expose_Quat(duk_context* ctx);
void Expose_Ray(duk_context* ctx);
void Expose_Sphere(duk_context* ctx);

void ExposeMathClasses(duk_context* ctx)
{
    Expose_AABB(ctx);
    Expose_Capsule(ctx);
    Expose_Circle(ctx);
    Expose_float2(ctx);
    Expose_float3(ctx);
    Expose_float3x3(ctx);
    Expose_float3x4(ctx);
    Expose_float4(ctx);
    Expose_float4x4(ctx);
    Expose_Frustum(ctx);
    Expose_LCG(ctx);
    Expose_Line(ctx);
    Expose_LineSegment(ctx);
    Expose_OBB(ctx);
    Expose_Plane(ctx);
    Expose_Quat(ctx);
    Expose_Ray(ctx);
    Expose_Sphere(ctx);
}

}
// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "UrhoRendererBindings.h"

namespace JSBindings
{

void Expose_AnimationController(duk_context* ctx);
void Expose_Camera(duk_context* ctx);
void Expose_EnvironmentLight(duk_context* ctx);
void Expose_Fog(duk_context* ctx);
void Expose_Light(duk_context* ctx);
void Expose_Mesh(duk_context* ctx);
void Expose_ParticleSystem(duk_context* ctx);
void Expose_Placeable(duk_context* ctx);
void Expose_Sky(duk_context* ctx);
void Expose_Terrain(duk_context* ctx);
void Expose_WaterPlane(duk_context* ctx);
void Expose_GraphicsWorld(duk_context* ctx);
void Expose_UrhoRenderer(duk_context* ctx);

void ExposeUrhoRendererClasses(duk_context* ctx)
{
    Expose_AnimationController(ctx);
    Expose_Camera(ctx);
    Expose_EnvironmentLight(ctx);
    Expose_Fog(ctx);
    Expose_Light(ctx);
    Expose_Mesh(ctx);
    Expose_ParticleSystem(ctx);
    Expose_Placeable(ctx);
    Expose_Sky(ctx);
    Expose_Terrain(ctx);
    Expose_WaterPlane(ctx);
    Expose_GraphicsWorld(ctx);
    Expose_UrhoRenderer(ctx);
}

}
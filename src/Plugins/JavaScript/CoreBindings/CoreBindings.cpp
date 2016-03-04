// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "CoreBindings.h"

namespace JSBindings
{

void Expose_Entity(duk_context* ctx);
void Expose_IComponent(duk_context* ctx);
void Expose_Scene(duk_context* ctx);
void Expose_Transform(duk_context* ctx);
void Expose_Framework(duk_context* ctx);
void Expose_FrameAPI(duk_context* ctx);
void Expose_SceneAPI(duk_context* ctx);
void Expose_ConfigAPI(duk_context* ctx);
void Expose_AssetAPI(duk_context* ctx);
void Expose_IAsset(duk_context* ctx);
void Expose_AssetReference(duk_context* ctx);
void Expose_AssetReferenceList(duk_context* ctx);

void ExposeCoreClasses(duk_context* ctx)
{
    Expose_Entity(ctx);
    Expose_IComponent(ctx);
    Expose_Scene(ctx);
    Expose_Transform(ctx);
    Expose_Framework(ctx);
    Expose_FrameAPI(ctx);
    Expose_SceneAPI(ctx);
    Expose_ConfigAPI(ctx);
    Expose_AssetAPI(ctx);
    Expose_IAsset(ctx);
    Expose_AssetReference(ctx);
    Expose_AssetReferenceList(ctx);
}

}
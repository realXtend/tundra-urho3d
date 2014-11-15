/// Class copied from the Urho3D project. Modified for Tundra use.

#pragma once

#include "FrameworkFwd.h"
#include "DebugHudPanel.h"
#include "CoreTimeUtils.h"

/// @cond PRIVATE

namespace Tundra
{

class ProfilerHudPanel : public DebugHudPanel
{
public:
    ProfilerHudPanel(Framework *framework);

    /// DebugHudPanel override.
    void UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget) override;

    /// Profiler max block depth.
    unsigned profilerMaxDepth;
    /// Profiler accumulation interval.
    unsigned profilerInterval;

protected:
    /// DebugHudPanel override.
    SharedPtr<Urho3D::UIElement> CreateImpl() override;

private:
    /// Profiler timer.
    Urho3D::Timer profilerTimer_;
};

class SceneHudPanel : public DebugHudPanel
{
public:
    SceneHudPanel(Framework *framework);

    /// DebugHudPanel override.
    void UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget) override;

protected:
    /// DebugHudPanel override.
    SharedPtr<Urho3D::UIElement> CreateImpl() override;

private:
    FrameLimiter limiter_;
};

class AssetHudPanel : public DebugHudPanel
{
public:
    AssetHudPanel(Framework *framework);

    /// DebugHudPanel override.
    void UpdatePanel(float frametime, const SharedPtr<Urho3D::UIElement> &widget) override;

protected:
    /// DebugHudPanel override.
    SharedPtr<Urho3D::UIElement> CreateImpl() override;

private:
    FrameLimiter limiter_;
};

}

/// @endcond

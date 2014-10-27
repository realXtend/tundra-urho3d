// For conditions of distribution and use, see copyright notice in LICENSE

#include "TestRunner.h"
#include "TestBenchmark.h"

#include <Algorithm/Random/LCG.h>

#include <Math/float2.h>
#include <Math/float3.h>
#include <Math/float4.h>
#include <Math/float3x3.h>
#include <Math/float3x4.h>
#include <Math/float4x4.h>
#include <Math/Quat.h>
#include <Geometry/AABB.h>
#include <Geometry/Ray.h>
#include <Geometry/Plane.h>
#include <Geometry/Sphere.h>
#include "Math/Color.h" // Tundra's own class

#include <Engine/Math/Vector2.h>
#include <Engine/Math/Vector3.h>
#include <Engine/Math/Vector4.h>
#include <Engine/Math/Matrix3.h>
#include <Engine/Math/Matrix3x4.h>
#include <Engine/Math/Matrix4.h>
#include <Engine/Math/Quaternion.h>
#include <Engine/Math/BoundingBox.h>
#include <Engine/Math/Ray.h>
#include <Engine/Math/Plane.h>
#include <Engine/Math/Sphere.h>
#include <Engine/Math/Color.h>

#include <Engine/Core/ProcessUtils.h>

using namespace Tundra;
using namespace Tundra::Test;

TEST_F(Runner, UrhoInterop)
{
    math::LCG lcg;
    math::float2 mglFloat2 = float2::RandomDir(lcg);
    math::float3 mglFloat3 = float3::RandomDir(lcg);
    math::float4 mglFloat4 = float4::RandomDir(lcg);
    math::float3x3 mglFloat3x3 = float3x3::RandomGeneral(lcg, -lcg.Float(), lcg.Float());
    math::float3x4 mglFloat3x4 = float3x4::RandomGeneral(lcg, -lcg.Float(), lcg.Float());
    math::float4x4 mglFloat4x4 = float4x4::RandomGeneral(lcg, -lcg.Float(), lcg.Float());
    math::Quat mglQuat = Quat::RandomRotation(lcg);
    math::AABB mglAabb(float3::RandomDir(lcg), float3::RandomDir(lcg));
    math::Ray mglRay(float3::RandomDir(lcg), float3::RandomDir(lcg));
    math::Plane mglPlane = math::Plane(float3::RandomDir(lcg), float3::RandomDir(lcg));
    math::Sphere mglSphere(float3::RandomDir(lcg), lcg.Float());
    Tundra::Color tundraColor = float4::RandomDir(lcg);

    Log("Tundra Color");

    Log(PadString("Tundra::Color",16) + "Urho3D::Color", 2);
    Urho3D::Color urhoColor = tundraColor;
    ASSERT_TRUE(urhoColor.Equals(tundraColor));

    Log(PadString("Tundra::Color",16) + "math::float4", 2);
    math::float4 tundraColorFloat4 = tundraColor;
    ASSERT_TRUE(tundraColorFloat4.Equals(tundraColor));

    Log("MGL > Urho");

    Log(PadString("math::float2",16) + "Urho3D::Vector2", 2);
    Urho3D::Vector2 urhoFloat2 = mglFloat2;
    ASSERT_TRUE(urhoFloat2.Equals(mglFloat2));

    Log(PadString("math::float3",16) + "Urho3D::Vector3", 2);
    Urho3D::Vector3 urhoFloat3 = mglFloat3;
    ASSERT_TRUE(urhoFloat3.Equals(mglFloat3));

    Log(PadString("math::float4",16) + "Urho3D::Vector4", 2);
    Urho3D::Vector4 urhoFloat4 = mglFloat4;
    ASSERT_TRUE(urhoFloat4.Equals(mglFloat4));

    Log(PadString("math::float3x3",16) + "Urho3D::Matrix3", 2);
    Urho3D::Matrix3 urhoFloat3x3 = mglFloat3x3;
    ASSERT_TRUE(urhoFloat3x3.Equals(mglFloat3x3));

    Log(PadString("math::float3x4",16) + "Urho3D::Matrix3x4", 2);
    Urho3D::Matrix3x4 urhoFloat3x4 = mglFloat3x4;
    ASSERT_TRUE(urhoFloat3x4.Equals(mglFloat3x4));

    Log(PadString("math::float4x4",16) + "Urho3D::Matrix4", 2);
    Urho3D::Matrix4 urhoFloat4x4 = mglFloat4x4;
    ASSERT_TRUE(urhoFloat4x4.Equals(mglFloat4x4));

    Log(PadString("math::Quat",16) + "Urho3D::Quaternion", 2);
    Urho3D::Quaternion urhoQuat = mglQuat;
    ASSERT_TRUE(urhoQuat.Equals(mglQuat));

    Log(PadString("math::AABB",16) + "Urho3D::BoundingBox", 2);
    Urho3D::BoundingBox urhoAabb = mglAabb;
    ASSERT_TRUE(urhoAabb.min_.Equals(mglAabb.minPoint)); ASSERT_TRUE(urhoAabb.max_.Equals(mglAabb.maxPoint));

    Log(PadString("math::Ray",16) + "Urho3D::Ray", 2);
    Urho3D::Ray urhoRay = mglRay;
    ASSERT_TRUE(urhoRay.origin_.Equals(mglRay.pos)); ASSERT_TRUE(urhoRay.direction_.Equals(mglRay.dir));

    Log(PadString("math::Plane",16) + "Urho3D::Plane", 2);
    Urho3D::Plane urhoPlane = mglPlane;
    ASSERT_TRUE(urhoPlane.normal_.Equals(mglPlane.normal)); ASSERT_TRUE(math::Equal(urhoPlane.d_, mglPlane.d));

    Log(PadString("math::Sphere",16) + "Urho3D::Sphere", 2);
    Urho3D::Sphere urhoSphere = mglSphere;
    ASSERT_TRUE(urhoSphere.center_.Equals(mglSphere.pos)); ASSERT_TRUE(math::Equal(urhoSphere.radius_, mglSphere.r));

    Log("Urho > MGL");
    mglFloat2 = urhoFloat2;
    ASSERT_TRUE(mglFloat2.Equals(urhoFloat2));
    mglFloat3 = urhoFloat3;
    ASSERT_TRUE(mglFloat3.Equals(urhoFloat3));
    mglFloat4 = urhoFloat4;
    ASSERT_TRUE(mglFloat4.Equals(urhoFloat4));
    mglFloat3x3 = urhoFloat3x3;
    ASSERT_TRUE(mglFloat3x3.Equals(urhoFloat3x3));
    mglFloat3x4 = urhoFloat3x4;
    ASSERT_TRUE(mglFloat3x4.Equals(urhoFloat3x4));
    mglFloat4x4 = urhoFloat4x4;
    ASSERT_TRUE(mglFloat4x4.Equals(urhoFloat4x4));
    mglQuat = urhoQuat;
    ASSERT_TRUE(mglQuat.Equals(urhoQuat));
    mglAabb = urhoAabb;
    ASSERT_TRUE(mglAabb.Equals(urhoAabb));
    mglRay = urhoRay;
    ASSERT_TRUE(mglRay.Equals(urhoRay));
    mglPlane = urhoPlane;
    ASSERT_TRUE(mglPlane.Equals(urhoPlane));
    mglSphere = urhoSphere;
    ASSERT_TRUE(mglSphere.Equals(urhoSphere));
    tundraColor = tundraColor;
    ASSERT_TRUE(tundraColor.Equals(tundraColor));
}

TUNDRA_TEST_MAIN();

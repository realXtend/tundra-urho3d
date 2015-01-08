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

#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Math/Vector3.h>
#include <Urho3D/Math/Vector4.h>
#include <Urho3D/Math/Matrix3.h>
#include <Urho3D/Math/Matrix3x4.h>
#include <Urho3D/Math/Matrix4.h>
#include <Urho3D/Math/Quaternion.h>
#include <Urho3D/Math/BoundingBox.h>
#include <Urho3D/Math/Ray.h>
#include <Urho3D/Math/Plane.h>
#include <Urho3D/Math/Sphere.h>
#include <Urho3D/Math/Color.h>

#include <Urho3D/Core/ProcessUtils.h>

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

    // Use very small epsilon as we're doing direct copies or assignments for the values when converting.
    const float epsilon = 1e-12f;

    Log("Tundra Color");

    Log(PadString("Tundra::Color",16) + "Urho3D::Color", 2);
    Urho3D::Color urhoColor = tundraColor;
    ASSERT_TRUE(math::EqualAbs(urhoColor.r_, tundraColor.r, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.g_, tundraColor.g, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.b_, tundraColor.b, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.a_, tundraColor.a, epsilon));

    Log(PadString("Tundra::Color",16) + "math::float4", 2);
    math::float4 tundraColorFloat4 = tundraColor;
    ASSERT_TRUE(math::EqualAbs(tundraColor.r, tundraColorFloat4.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(tundraColor.g, tundraColorFloat4.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(tundraColor.b, tundraColorFloat4.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(tundraColor.a, tundraColorFloat4.w, epsilon));

    Log(PadString("Urho3D::Color",16) + "Tundra::Color", 2);
    ASSERT_TRUE(math::EqualAbs(urhoColor.r_, tundraColor.r, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.g_, tundraColor.g, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.b_, tundraColor.b, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoColor.a_, tundraColor.a, epsilon));

    Log("MGL > Urho");

    Log(PadString("math::float2",16) + "Urho3D::Vector2", 2);
    Urho3D::Vector2 urhoFloat2 = mglFloat2;
    ASSERT_TRUE(math::EqualAbs(urhoFloat2.x_, mglFloat2.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat2.y_, mglFloat2.y, epsilon));

    Log(PadString("math::float3",16) + "Urho3D::Vector3", 2);
    Urho3D::Vector3 urhoFloat3 = mglFloat3;
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.x_, mglFloat3.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.y_, mglFloat3.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.z_, mglFloat3.z, epsilon));

    Log(PadString("math::float4",16) + "Urho3D::Vector4", 2);
    Urho3D::Vector4 urhoFloat4 = mglFloat4;
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.x_, mglFloat4.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.y_, mglFloat4.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.z_, mglFloat4.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.w_, mglFloat4.w, epsilon));

    Log(PadString("math::float3x3",16) + "Urho3D::Matrix3", 2);
    Urho3D::Matrix3 urhoFloat3x3 = mglFloat3x3;
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m00_, mglFloat3x3.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m01_, mglFloat3x3.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m02_, mglFloat3x3.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m10_, mglFloat3x3.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m11_, mglFloat3x3.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m12_, mglFloat3x3.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m20_, mglFloat3x3.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m21_, mglFloat3x3.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m22_, mglFloat3x3.v[2][2], epsilon));

    Log(PadString("math::float3x4",16) + "Urho3D::Matrix3x4", 2);
    Urho3D::Matrix3x4 urhoFloat3x4 = mglFloat3x4;
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m00_, mglFloat3x4.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m01_, mglFloat3x4.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m02_, mglFloat3x4.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m03_, mglFloat3x4.v[0][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m10_, mglFloat3x4.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m11_, mglFloat3x4.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m12_, mglFloat3x4.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m13_, mglFloat3x4.v[1][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m20_, mglFloat3x4.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m21_, mglFloat3x4.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m22_, mglFloat3x4.v[2][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m23_, mglFloat3x4.v[2][3], epsilon));

    Log(PadString("math::float4x4",16) + "Urho3D::Matrix4", 2);
    Urho3D::Matrix4 urhoFloat4x4 = mglFloat4x4;
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m00_, mglFloat4x4.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m01_, mglFloat4x4.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m02_, mglFloat4x4.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m03_, mglFloat4x4.v[0][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m10_, mglFloat4x4.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m11_, mglFloat4x4.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m12_, mglFloat4x4.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m13_, mglFloat4x4.v[1][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m20_, mglFloat4x4.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m21_, mglFloat4x4.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m22_, mglFloat4x4.v[2][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m23_, mglFloat4x4.v[2][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m30_, mglFloat4x4.v[3][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m31_, mglFloat4x4.v[3][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m32_, mglFloat4x4.v[3][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m33_, mglFloat4x4.v[3][3], epsilon));

    Log(PadString("math::Quat",16) + "Urho3D::Quaternion", 2);
    Urho3D::Quaternion urhoQuat = mglQuat;
    ASSERT_TRUE(math::EqualAbs(urhoQuat.x_, mglQuat.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.y_, mglQuat.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.z_, mglQuat.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.w_, mglQuat.w, epsilon));

    Log(PadString("math::AABB",16) + "Urho3D::BoundingBox", 2);
    Urho3D::BoundingBox urhoAabb = mglAabb;
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.x_, mglAabb.minPoint.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.y_, mglAabb.minPoint.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.z_, mglAabb.minPoint.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.x_, mglAabb.maxPoint.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.y_, mglAabb.maxPoint.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.z_, mglAabb.maxPoint.z, epsilon));

    Log(PadString("math::Ray",16) + "Urho3D::Ray", 2);
    Urho3D::Ray urhoRay = mglRay;
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.x_, mglRay.pos.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.y_, mglRay.pos.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.z_, mglRay.pos.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.x_, mglRay.dir.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.y_, mglRay.dir.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.z_, mglRay.dir.z, epsilon));

    Log(PadString("math::Plane",16) + "Urho3D::Plane", 2);
    Urho3D::Plane urhoPlane = mglPlane;
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.x_, mglPlane.normal.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.y_, mglPlane.normal.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.z_, mglPlane.normal.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.d_, mglPlane.d, epsilon));

    Log(PadString("math::Sphere",16) + "Urho3D::Sphere", 2);
    Urho3D::Sphere urhoSphere = mglSphere;
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.x_, mglSphere.pos.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.y_, mglSphere.pos.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.z_, mglSphere.pos.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.radius_, mglSphere.r, epsilon));

    Log("Urho > MGL");

    mglFloat2 = urhoFloat2;
    Log(PadString("math::float2",16) + "Urho3D::Vector2", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat2.x_, mglFloat2.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat2.y_, mglFloat2.y, epsilon));

    mglFloat3 = urhoFloat3;
    Log(PadString("math::float3",16) + "Urho3D::Vector3", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.x_, mglFloat3.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.y_, mglFloat3.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3.z_, mglFloat3.z, epsilon));

    mglFloat4 = urhoFloat4;
    Log(PadString("math::float4",16) + "Urho3D::Vector4", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.x_, mglFloat4.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.y_, mglFloat4.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.z_, mglFloat4.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4.w_, mglFloat4.w, epsilon));

    mglFloat3x3 = urhoFloat3x3;
    Log(PadString("math::float3x3",16) + "Urho3D::Matrix3", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m00_, mglFloat3x3.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m01_, mglFloat3x3.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m02_, mglFloat3x3.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m10_, mglFloat3x3.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m11_, mglFloat3x3.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m12_, mglFloat3x3.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m20_, mglFloat3x3.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m21_, mglFloat3x3.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x3.m22_, mglFloat3x3.v[2][2], epsilon));

    mglFloat3x4 = urhoFloat3x4;
    Log(PadString("math::float3x4",16) + "Urho3D::Matrix3x4", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m00_, mglFloat3x4.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m01_, mglFloat3x4.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m02_, mglFloat3x4.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m03_, mglFloat3x4.v[0][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m10_, mglFloat3x4.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m11_, mglFloat3x4.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m12_, mglFloat3x4.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m13_, mglFloat3x4.v[1][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m20_, mglFloat3x4.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m21_, mglFloat3x4.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m22_, mglFloat3x4.v[2][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat3x4.m23_, mglFloat3x4.v[2][3], epsilon));

    mglFloat4x4 = urhoFloat4x4;
    Log(PadString("math::float4x4",16) + "Urho3D::Matrix4", 2);
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m00_, mglFloat4x4.v[0][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m01_, mglFloat4x4.v[0][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m02_, mglFloat4x4.v[0][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m03_, mglFloat4x4.v[0][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m10_, mglFloat4x4.v[1][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m11_, mglFloat4x4.v[1][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m12_, mglFloat4x4.v[1][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m13_, mglFloat4x4.v[1][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m20_, mglFloat4x4.v[2][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m21_, mglFloat4x4.v[2][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m22_, mglFloat4x4.v[2][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m23_, mglFloat4x4.v[2][3], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m30_, mglFloat4x4.v[3][0], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m31_, mglFloat4x4.v[3][1], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m32_, mglFloat4x4.v[3][2], epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoFloat4x4.m33_, mglFloat4x4.v[3][3], epsilon));

    mglQuat = urhoQuat;
    Log(PadString("math::Quat",16) + "Urho3D::Quaternion", 2);
    ASSERT_TRUE(math::EqualAbs(urhoQuat.x_, mglQuat.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.y_, mglQuat.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.z_, mglQuat.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoQuat.w_, mglQuat.w, epsilon));

    mglAabb = urhoAabb;
    Log(PadString("math::AABB",16) + "Urho3D::BoundingBox", 2);
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.x_, mglAabb.minPoint.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.y_, mglAabb.minPoint.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.min_.z_, mglAabb.minPoint.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.x_, mglAabb.maxPoint.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.y_, mglAabb.maxPoint.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoAabb.max_.z_, mglAabb.maxPoint.z, epsilon));

    mglRay = urhoRay;
    Log(PadString("math::Ray",16) + "Urho3D::Ray", 2);
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.x_, mglRay.pos.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.y_, mglRay.pos.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.origin_.z_, mglRay.pos.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.x_, mglRay.dir.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.y_, mglRay.dir.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoRay.direction_.z_, mglRay.dir.z, epsilon));

    mglPlane = urhoPlane;
    Log(PadString("math::Plane",16) + "Urho3D::Plane", 2);
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.x_, mglPlane.normal.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.y_, mglPlane.normal.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.normal_.z_, mglPlane.normal.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoPlane.d_, mglPlane.d, epsilon));

    mglSphere = urhoSphere;
    Log(PadString("math::Sphere",16) + "Urho3D::Sphere", 2);
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.x_, mglSphere.pos.x, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.y_, mglSphere.pos.y, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.center_.z_, mglSphere.pos.z, epsilon));
    ASSERT_TRUE(math::EqualAbs(urhoSphere.radius_, mglSphere.r, epsilon));
}

TUNDRA_TEST_MAIN();

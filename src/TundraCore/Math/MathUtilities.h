// For conditions of distribution and use, see copyright notice in LICENSE

/// \todo To be obsoleted if/when MathGeoLib has direct Urho3D interoperability implemented

#include "Math/float3x3.h"
#include "Math/float3x4.h"
#include "Math/float4x4.h"
#include "Math/Quat.h"
#include "Geometry/AABB.h"

#include <Matrix3x4.h>
#include <Matrix4.h>
#include <Engine/Math/BoundingBox.h>

namespace Tundra
{

/// Convert float3 to Urho3D Vector3.
inline Urho3D::Vector3 ToVector3(const float3& v) { return Urho3D::Vector3(v.x, v.y, v.z); }

/// Convert float4 to Urho3D Vector4.
inline Urho3D::Vector4 ToVector4(const float4& v) { return Urho3D::Vector4(v.x, v.y, v.z, v.w); }

/// Convert Quat to Urho3D Quaternion
inline Urho3D::Quaternion ToQuaternion(const Quat& q) { return Urho3D::Quaternion(q.w, q.x, q.y, q.z); }

/// Convert float3x3 to Urho3D Matrix3
inline Urho3D::Matrix3 ToMatrix3(const float3x3& m) { return Urho3D::Matrix3(m.ptr()); }

/// Convert float3x4 to Urho3D Matrix3x4
inline Urho3D::Matrix3x4 ToMatrix3(const float3x4& m) { return Urho3D::Matrix3(m.ptr()); }

/// Convert float4x4 to Urho3D Matrix4
inline Urho3D::Matrix4 ToMatrix4(const float4x4& m) { return Urho3D::Matrix3(m.ptr()); }

/// Convert AABB to Urho3D BoundingBox
inline Urho3D::BoundingBox ToBoundingBox(const AABB& b) { return Urho3D::BoundingBox(ToVector3(b.minPoint), ToVector3(b.maxPoint)); }


/// Convert Urho3D Vector3 to float3
inline float3 ToFloat3(const Urho3D::Vector3& v) { return float3(v.x_, v.y_, v.z_); }

/// Convert Urho3D Vector4 to float4.
inline float4 ToFloat4(const Urho3D::Vector4& v) { return float4(v.x_, v.y_, v.z_, v.w_); }

/// Convert Urho3D Quaternion to Quat.
inline Quat ToQuat(const Urho3D::Quaternion& q) { return Quat(q.x_, q.y_, q.z_, q.w_); }

/// Convert Urho3D Matrix3 to float3x3.
inline float3x3 ToFloat3x3(const Urho3D::Matrix3& m) { return float3x3(m.m00_, m.m01_, m.m02_, m.m10_, m.m11_, m.m12_, m.m20_, m.m21_, m.m22_); }

/// Convert Urho3D Matrix3x4 to float3x4.
inline float3x4 ToFloat3x4(const Urho3D::Matrix3x4& m) { return float3x4(m.m00_, m.m01_, m.m02_, m.m03_, m.m10_, m.m11_, m.m12_, m.m13_, m.m20_, m.m21_, m.m22_, m.m23_); }

/// Convert Urho3D Matrix4 to float4x4.
inline float4x4 ToFloat4x4(const Urho3D::Matrix4& m) { return float4x4(m.m00_, m.m01_, m.m02_, m.m03_, m.m10_, m.m11_, m.m12_, m.m13_, m.m20_, m.m21_, m.m22_, m.m23_, m.m30_, m.m31_, m.m32_, m.m33_); }

/// Convert Urho3D Matrix3x4 to float4x4.
inline float4x4 ToFloat4x4(const Urho3D::Matrix3x4& m) { return float4x4(m.m00_, m.m01_, m.m02_, m.m03_, m.m10_, m.m11_, m.m12_, m.m13_, m.m20_, m.m21_, m.m22_, m.m23_, 0.0f, 0.0f, 0.0f, 1.0f); }

/// Convert Urho3D BoundingBox to AABB.
inline AABB ToAABB(const Urho3D::BoundingBox& b) { return AABB(ToFloat3(b.min_), ToFloat3(b.max_)); }

}

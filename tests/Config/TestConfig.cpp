// For conditions of distribution and use, see copyright notice in LICENSE

#include "TestRunner.h"
#include "TestBenchmark.h"

#include "ConfigAPI.h"

#include "Algorithm/Random/LCG.h"

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

using namespace Tundra;
using namespace Tundra::Test;

const ConfigData CfgWrite("Config Test", "Test Write");
const ConfigData CfgWriteFile("Config Test", "Test Write File");

// Note we use MGL Urho interop functionality as MLG has convinient support for random data.
static LCG Random;

/* @todo We would like to test negative values as well with Random.Int(Urho3D::M_MIN_INT, Urho3D::M_MAX_INT)
    but it always returns -2147483648 or whatever the first param is? */
int   RandomInt()       { return Random.Int(); }
float RandomFloat()     { return Random.Float(); }
bool  RandomBool()      { return Random.Int(0,1) == 1; }

VariantList DataList()
{
    static Urho3D::Vector2 v2           = float2::RandomDir(Random);
    static Urho3D::Vector3 v3           = float3::RandomDir(Random);
    static Urho3D::Vector4 v4           = float4::RandomDir(Random);
    static Urho3D::Quaternion q         = Quat::RandomRotation(Random);
    static Urho3D::Matrix3 m3           = float3x3::RandomGeneral(Random, -RandomFloat(), RandomFloat());
    static Urho3D::Matrix3x4 m3x4       = float3x4::RandomGeneral(Random, -RandomFloat(), RandomFloat());
    static Urho3D::Matrix4 m4           = float4x4::RandomGeneral(Random, -RandomFloat(), RandomFloat());

    static Variant Value_VAR_INT        = RandomInt();
    static Variant Value_VAR_BOOL       = RandomBool();
    static Variant Value_VAR_FLOAT      = Random.Float();
    Variant Value_VAR_VECTOR2           = v2;
    Variant Value_VAR_VECTOR3           = v3;
    Variant Value_VAR_VECTOR4           = v4;
    Variant Value_VAR_QUATERNION        = q;
    static Variant Value_VAR_COLOR      = Urho3D::Color(RandomFloat(), RandomFloat(), RandomFloat(), RandomFloat());
    static Variant Value_VAR_STRING     = "This [is] a = a test } { @ ?!";
    // Variant Value_VAR_BUFFER;
    // Variant Value_VAR_VOIDPTR;
    // Variant Value_VAR_RESOURCEREF = Urho3D::ResourceRef(StringHash("TestName"), "TestType");
    // Variant Value_VAR_RESOURCEREFLIST;
    // Variant Value_VAR_VARIANTVECTOR;
    // Variant Value_VAR_VARIANTMAP;
    static Variant Value_VAR_INTRECT    = Urho3D::IntRect(RandomInt(), RandomInt(), RandomInt(), RandomInt());
    static Variant Value_VAR_INTVECTOR2 = Urho3D::IntVector2(RandomInt(), RandomInt());
    // const Variant Value_VAR_PTR;
    Variant Value_VAR_MATRIX3           = m3;
    Variant Value_VAR_MATRIX3X4         = m3x4;
    Variant Value_VAR_MATRIX4           = m4;

    VariantList data;
    data.Push(Value_VAR_INT);
    data.Push(Value_VAR_BOOL);
    data.Push(Value_VAR_FLOAT);
    data.Push(Value_VAR_VECTOR2);
    data.Push(Value_VAR_VECTOR3);
    data.Push(Value_VAR_VECTOR4);
    data.Push(Value_VAR_QUATERNION);
    data.Push(Value_VAR_COLOR);
    data.Push(Value_VAR_STRING);
    // data.Push(Value_VAR_BUFFER);
    // data.Push(Value_VAR_VOIDPTR);
    // data.Push(Value_VAR_RESOURCEREF);
    // data.Push(Value_VAR_RESOURCEREFLIST);
    // data.Push(Value_VAR_VARIANTVECTOR);
    // data.Push(Value_VAR_VARIANTMAP);
    data.Push(Value_VAR_INTRECT);
    data.Push(Value_VAR_INTVECTOR2);
    // data.Push(Value_VAR_PTR);
    data.Push(Value_VAR_MATRIX3);
    data.Push(Value_VAR_MATRIX3X4);
    data.Push(Value_VAR_MATRIX4);
    return data;
}

HashMap<String, Variant> DataMap()
{
    HashMap<String, Variant> data;
    VariantList list = DataList();
    foreach(auto variant, list)
    {
        String key = Urho3D::ToString("Test %d %s", static_cast<int>(variant.GetType()), variant.GetTypeName().CString());
        String expected = variant.ToString();
        data[key] = variant;
    }
    return data;
}

TEST_F(Runner, WriteConfig)
{
    // ConfigAPI::Read

    const HashMap<String, Variant> data = DataMap();
    for(auto iter=data.Begin(); iter!=data.End(); ++iter)
    {
        const String &key = iter->first_;
        const Variant &value = iter->second_;
        String expected = value.ToString();

        Log(PadString(static_cast<int>(value.GetType()), 3) + value.GetTypeName(), 2);

        // Write to memory
        framework->Config()->Write(CfgWrite, key, value);

        // Read from memory
        Variant read = framework->Config()->Read(CfgWrite, key, Variant::EMPTY);
        String received = read.ToString();
        ASSERT_EQ(read.GetType(), value.GetType());
        ASSERT_STREQ(expected.CString(), received.CString());
    }

    // ConfigAPI::GetFile + ConfigFile::Set + ConfigAPI::WriteFile

    ConfigFile &config = framework->Config()->GetFile(CfgWriteFile.file);
    for(auto iter=data.Begin(); iter!=data.End(); ++iter)
    {
        const String &key = iter->first_;
        const Variant &value = iter->second_;
        String expected = value.ToString();

        // Write to memory
        config.Set(CfgWriteFile.section, key, value);

        // Read from memory
        Variant read = config.Get(CfgWriteFile.section, key, Variant::EMPTY);
        String received = read.ToString();
        ASSERT_EQ(read.GetType(), value.GetType());
        ASSERT_STREQ(expected.CString(), received.CString());
    }

    // Write to disk
    framework->Config()->WriteFile(CfgWriteFile.file);
}

TEST_F(Runner, ReadConfig)
{
    Log("ConfigAPI::Read", 2);

    const HashMap<String, Variant> data = DataMap();
    for(auto iter=data.Begin(); iter!=data.End(); ++iter)
    {
        const String &key = iter->first_;
        const Variant &value = iter->second_;
        String expected = value.ToString();

        {
            Variant read = framework->Config()->Read(CfgWrite, key, Variant::EMPTY);
            String received = read.ToString();
            ASSERT_EQ(read.GetType(), value.GetType());
            ASSERT_STREQ(expected.CString(), received.CString());
        }
        {
            Variant read = framework->Config()->Read(CfgWriteFile, key, Variant::EMPTY);
            String received = read.ToString();
            ASSERT_EQ(read.GetType(), value.GetType());
            ASSERT_STREQ(expected.CString(), received.CString());
        }
    }

    Log("ConfigAPI::GetFile + ConfigFile::Get", 2);

    ConfigFile config = framework->Config()->GetFile(CfgWrite.file);
    ConfigFile configFile = framework->Config()->GetFile(CfgWriteFile.file);
    for(auto iter=data.Begin(); iter!=data.End(); ++iter)
    {
        const String &key = iter->first_;
        const Variant &value = iter->second_;
        String expected = value.ToString();

        {
            Variant read = config.Get(CfgWrite.section, key, Variant::EMPTY);
            String received = read.ToString();
            ASSERT_EQ(read.GetType(), value.GetType());
            ASSERT_STREQ(expected.CString(), received.CString());
        }
        {
            Variant read = configFile.Get(CfgWriteFile.section, key, Variant::EMPTY);
            String received = read.ToString();
            ASSERT_EQ(read.GetType(), value.GetType());
            ASSERT_STREQ(expected.CString(), received.CString());
        }
    }
}

TUNDRA_TEST_MAIN();

/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   IAttribute.cpp
    @brief  Abstract base class and template class for entity-component attributes. */

#include "StableHeaders.h"

#include "IAttribute.h"
#include "Entity.h"
#include "IComponent.h"
#include "CoreTypes.h"
#include "CoreDefines.h"
#include "CoreStringUtils.h"
#include "AssetReference.h"
#include "EntityReference.h"
#include "Math/Transform.h"
#include "Math/Quat.h"
#include "Math/float2.h"
#include "Math/float3.h"
#include "Math/float3.h"
#include "Math/MathFunc.h"
#include "Math/Color.h"
#include "Math/Point.h"

#include <StringUtils.h>
#include <Log.h>

#include <kNet/DataDeserializer.h>
#include <kNet/DataSerializer.h>

namespace Tundra
{

IAttribute::IAttribute(IComponent* owner_, const char* id_) :
    id(id_),
    name(id_),
    metadata(0),
    dynamic(false),
    owner(0),
    index(0),
    valueChanged(true)
{
    if (owner_)
        owner_->AddAttribute(this);
}

IAttribute::IAttribute(IComponent* owner_, const char* id_, const char* name_) :
    id(id_),
    name(name_),
    metadata(0),
    dynamic(false),
    owner(0),
    index(0),
    valueChanged(true)
{
    if (owner_)
        owner_->AddAttribute(this);
}


void IAttribute::Changed(AttributeChange::Type change)
{
    if (owner)
        owner->EmitAttributeChanged(this, change);
}

void IAttribute::SetName(const String& newName)
{
    name = newName;
}

void IAttribute::SetMetadata(AttributeMetadata *meta)
{
    metadata = meta;
    EmitAttributeMetadataChanged();
}

void IAttribute::EmitAttributeMetadataChanged()
{
    if (owner && metadata)
        owner->EmitAttributeMetadataChanged(this);
}

AttributeMetadata *IAttribute::Metadata() const
{
    return metadata;
}

// Hide all template implementations from being included to public documentation
/// @cond PRIVATE

// TYPEID TEMPLATE IMPLEMENTATIONS
template<> u32 TUNDRACORE_API Attribute<String>::TypeId() const { return cAttributeString; }
template<> u32 TUNDRACORE_API Attribute<int>::TypeId() const { return cAttributeInt; }
template<> u32 TUNDRACORE_API Attribute<float>::TypeId() const { return cAttributeReal; }
template<> u32 TUNDRACORE_API Attribute<Color>::TypeId() const { return cAttributeColor; }
template<> u32 TUNDRACORE_API Attribute<float2>::TypeId() const { return cAttributeFloat2; }
template<> u32 TUNDRACORE_API Attribute<float3>::TypeId() const { return cAttributeFloat3; }
template<> u32 TUNDRACORE_API Attribute<float4>::TypeId() const { return cAttributeFloat4; }
template<> u32 TUNDRACORE_API Attribute<bool>::TypeId() const { return cAttributeBool; }
template<> u32 TUNDRACORE_API Attribute<uint>::TypeId() const { return cAttributeUInt; }
template<> u32 TUNDRACORE_API Attribute<Quat>::TypeId() const { return cAttributeQuat; }
template<> u32 TUNDRACORE_API Attribute<AssetReference>::TypeId() const { return cAttributeAssetReference; }
template<> u32 TUNDRACORE_API Attribute<AssetReferenceList>::TypeId() const { return cAttributeAssetReferenceList; }
template<> u32 TUNDRACORE_API Attribute<EntityReference>::TypeId() const { return cAttributeEntityReference; }
template<> u32 TUNDRACORE_API Attribute<Variant>::TypeId() const { return cAttributeVariant; }
template<> u32 TUNDRACORE_API Attribute<VariantList>::TypeId() const { return cAttributeVariantList; }
template<> u32 TUNDRACORE_API Attribute<Transform>::TypeId() const { return cAttributeTransform; }
template<> u32 TUNDRACORE_API Attribute<Point>::TypeId() const { return cAttributePoint; }

// DefaultValue implementations
template<> String TUNDRACORE_API Attribute<String>::DefaultValue() const { return String(); }
template<> int TUNDRACORE_API Attribute<int>::DefaultValue() const { return 0; }
template<> float TUNDRACORE_API Attribute<float>::DefaultValue() const { return 0.f; }
template<> Color TUNDRACORE_API Attribute<Color>::DefaultValue() const { return Color(); }
template<> float2 TUNDRACORE_API Attribute<float2>::DefaultValue() const { return float2::zero; }
template<> float3 TUNDRACORE_API Attribute<float3>::DefaultValue() const { return float3::zero; }
template<> float4 TUNDRACORE_API Attribute<float4>::DefaultValue() const { return float4::zero; }
template<> bool TUNDRACORE_API Attribute<bool>::DefaultValue() const { return false; }
template<> uint TUNDRACORE_API Attribute<uint>::DefaultValue() const { return 0; }
template<> Quat TUNDRACORE_API Attribute<Quat>::DefaultValue() const { return Quat::identity; }
template<> AssetReference TUNDRACORE_API Attribute<AssetReference>::DefaultValue() const { return AssetReference(); }
template<> AssetReferenceList TUNDRACORE_API Attribute<AssetReferenceList>::DefaultValue() const { return AssetReferenceList(); }
template<> EntityReference TUNDRACORE_API Attribute<EntityReference>::DefaultValue() const { return EntityReference(); }
template<> Variant TUNDRACORE_API Attribute<Variant>::DefaultValue() const { return Variant(); }
template<> VariantList TUNDRACORE_API Attribute<VariantList>::DefaultValue() const { return VariantList(); }
template<> Transform TUNDRACORE_API Attribute<Transform>::DefaultValue() const { return Transform(); }
template<> Point TUNDRACORE_API Attribute<Point>::DefaultValue() const { return Point(); }

// TOSTRING TEMPLATE IMPLEMENTATIONS.

template<> String TUNDRACORE_API Attribute<String>::ToString() const
{
    ///\todo decode/encode XML-risky characters
    return Get();
}

template<> String TUNDRACORE_API Attribute<bool>::ToString() const
{
    return Get() ? "true" : "false";
}

template<> String TUNDRACORE_API Attribute<int>::ToString() const
{
    char str[256];
    sprintf(str, "%i", Get());
    return str;
}

template<> String TUNDRACORE_API Attribute<uint>::ToString() const
{
    char str[256];
    sprintf(str, "%u", Get());
    return str;
}

template<> String TUNDRACORE_API Attribute<float>::ToString() const
{
    char str[256];
    sprintf(str, "%.9g", Get());
    return str;
}

template<> String TUNDRACORE_API Attribute<Quat>::ToString() const
{
    return String(Get().SerializeToString().c_str());
}

template<> String TUNDRACORE_API Attribute<float2>::ToString() const
{
    return String(Get().SerializeToString().c_str());
}

template<> String TUNDRACORE_API Attribute<float3>::ToString() const
{
    return String(Get().SerializeToString().c_str());
}

template<> String TUNDRACORE_API Attribute<float4>::ToString() const
{
    return String(Get().SerializeToString().c_str());
}

template<> String TUNDRACORE_API Attribute<Color>::ToString() const
{
    return Get().SerializeToString();
}

template<> String TUNDRACORE_API Attribute<Point>::ToString() const
{
    return Get().SerializeToString();
}

template<> String TUNDRACORE_API Attribute<AssetReference>::ToString() const
{
    return Get().ref;
}

template<> String TUNDRACORE_API Attribute<AssetReferenceList>::ToString() const
{
    String stringValue;
    const AssetReferenceList &values = Get();
    for(int i = 0; i < values.Size(); ++i)
    {
        stringValue += values[i].ref;
        if (i < values.Size() - 1)
            stringValue += ";";
    }

    return stringValue;
}

template<> String TUNDRACORE_API Attribute<EntityReference>::ToString() const
{
    return Get().ref;
}

template<> String TUNDRACORE_API Attribute<Variant>::ToString() const
{
    return Get().ToString();
}

template<> String TUNDRACORE_API Attribute<VariantList>::ToString() const
{
    const VariantList &values = Get();

    String stringValue;
    for(unsigned i = 0; i < values.Size(); i++)
    {
        stringValue += values[i].ToString();
        if(i < values.Size() - 1)
            stringValue += ";";
    }
    return stringValue;
}

template<> String TUNDRACORE_API Attribute<Transform>::ToString() const
{
    return Get().SerializeToString();
}

// TYPENAMETOSTRING TEMPLATE IMPLEMENTATIONS.

template<> const String TUNDRACORE_API & Attribute<int>::TypeName() const
{
    return cAttributeIntTypeName;
}

template<> const String TUNDRACORE_API & Attribute<uint>::TypeName() const
{
    return cAttributeUIntTypeName;
}

template<> const String TUNDRACORE_API & Attribute<float>::TypeName() const
{
    return cAttributeRealTypeName;
}

template<> const String TUNDRACORE_API & Attribute<String>::TypeName() const
{
    return cAttributeStringTypeName;
}

template<> const String TUNDRACORE_API & Attribute<bool>::TypeName() const
{
    return cAttributeBoolTypeName;
}

template<> const String TUNDRACORE_API & Attribute<Quat>::TypeName() const
{
    return cAttributeQuatTypeName;
}

template<> const String TUNDRACORE_API & Attribute<float2>::TypeName() const
{
    return cAttributeFloat2TypeName;
}

template<> const String TUNDRACORE_API & Attribute<float3>::TypeName() const
{
    return cAttributeFloat3TypeName;
}

template<> const String TUNDRACORE_API & Attribute<float4>::TypeName() const
{
    return cAttributeFloat4TypeName;
}

template<> const String TUNDRACORE_API & Attribute<Color>::TypeName() const
{
    return cAttributeColorTypeName;
}

template<> const String TUNDRACORE_API & Attribute<AssetReference>::TypeName() const
{
    return cAttributeAssetReferenceTypeName;
}

template<> const String TUNDRACORE_API & Attribute<AssetReferenceList>::TypeName() const
{
    return cAttributeAssetReferenceListTypeName;
}

template<> const String TUNDRACORE_API & Attribute<EntityReference>::TypeName() const
{
    return cAttributeEntityReferenceTypeName;
}

template<> const String TUNDRACORE_API & Attribute<Variant>::TypeName() const
{
    return cAttributeVariantTypeName;
}

template<> const String TUNDRACORE_API & Attribute<VariantList >::TypeName() const
{
    return cAttributeVariantListTypeName;
}

template<> const String TUNDRACORE_API & Attribute<Transform>::TypeName() const
{
    return cAttributeTransformTypeName;
}

template<> const String TUNDRACORE_API & Attribute<Point>::TypeName() const
{
    return cAttributePointTypeName;
}

// FROMSTRING TEMPLATE IMPLEMENTATIONS.

template<> void TUNDRACORE_API Attribute<String>::FromString(const String& str, AttributeChange::Type change)
{
    ///\todo decode/encode XML-risky characters
    Set(str, change);
}

template<> void TUNDRACORE_API Attribute<bool>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Urho3D::ToBool(str), change);
}

template<> void TUNDRACORE_API Attribute<int>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Urho3D::ToInt(str), change);
}

template<> void TUNDRACORE_API Attribute<uint>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Urho3D::ToUInt(str), change);
}

template<> void TUNDRACORE_API Attribute<float>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Urho3D::ToFloat(str), change);
}

template<> void TUNDRACORE_API Attribute<Color>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Color::FromString(str.CString()), change);
}

template<> void TUNDRACORE_API Attribute<Quat>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Quat::FromString(str.CString()), change);
}

template<> void TUNDRACORE_API Attribute<float2>::FromString(const String& str, AttributeChange::Type change)
{
    Set(float2::FromString(str.CString()), change);
}

template<> void TUNDRACORE_API Attribute<float3>::FromString(const String& str, AttributeChange::Type change)
{
    Set(float3::FromString(str.CString()), change);
}

template<> void TUNDRACORE_API Attribute<float4>::FromString(const String& str, AttributeChange::Type change)
{
    Set(float4::FromString(str.CString()), change);
}

template<> void TUNDRACORE_API Attribute<AssetReference>::FromString(const String& str, AttributeChange::Type change)
{
    Set(AssetReference(str), change);
}

template<> void TUNDRACORE_API Attribute<AssetReferenceList>::FromString(const String& str, AttributeChange::Type change)
{
    AssetReferenceList value;
    const StringVector components = str.Split(';');
    for(int i = 0; i < (int)components.Size(); i++)
        value.Append(AssetReference(components[i]));
    if (value.Size() == 1 && value[0].ref.Trimmed().Empty())
        value.RemoveLast();

    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<EntityReference>::FromString(const String& str, AttributeChange::Type change)
{
    Set(EntityReference(str), change);
}

template<> void TUNDRACORE_API Attribute<Variant>::FromString(const String& str, AttributeChange::Type change)
{
    Set(str, change); /// \todo Variant always becomes essentially a string when deserializing
}

template<> void TUNDRACORE_API Attribute<VariantList>::FromString(const String& str, AttributeChange::Type change)
{
    VariantList value;
    const StringVector components = str.Split(';');
    for(int i = 0; i < (int)components.Size(); i++)
        value.Push(Variant(components[i])); /// \todo Variant always becomes essentially a string when deserializing
    if(value.Size() == 1 && value[0] == "")
        value.Pop();

    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<Transform>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Transform::FromString(str), change);
}

template<> void TUNDRACORE_API Attribute<Point>::FromString(const String& str, AttributeChange::Type change)
{
    Set(Point::FromString(str.CString()), change);
}


// TOBINARY TEMPLATE IMPLEMENTATIONS.

template<> void TUNDRACORE_API Attribute<String>::ToBinary(kNet::DataSerializer& dest) const
{
    WriteUtf8String(dest, value);
}

template<> void TUNDRACORE_API Attribute<bool>::ToBinary(kNet::DataSerializer& dest) const
{
    if (value)
        dest.Add<u8>(1);
    else
        dest.Add<u8>(0);
}

template<> void TUNDRACORE_API Attribute<int>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<s32>(value);
}

template<> void TUNDRACORE_API Attribute<uint>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<u32>(value);
}

template<> void TUNDRACORE_API Attribute<float>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value);
}

template<> void TUNDRACORE_API Attribute<Quat>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.x);
    dest.Add<float>(value.y);
    dest.Add<float>(value.z);
    dest.Add<float>(value.w);
}

template<> void TUNDRACORE_API Attribute<float2>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.x);
    dest.Add<float>(value.y);
}

template<> void TUNDRACORE_API Attribute<float3>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.x);
    dest.Add<float>(value.y);
    dest.Add<float>(value.z);
}

template<> void TUNDRACORE_API Attribute<float4>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.x);
    dest.Add<float>(value.y);
    dest.Add<float>(value.z);
    dest.Add<float>(value.w);
}

template<> void TUNDRACORE_API Attribute<Color>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.r);
    dest.Add<float>(value.g);
    dest.Add<float>(value.b);
    dest.Add<float>(value.a);
}

template<> void TUNDRACORE_API Attribute<AssetReference>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.AddString(value.ref.CString()); /**< @todo Allow longer strings than 255 chars */
}

template<> void TUNDRACORE_API Attribute<AssetReferenceList>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<u8>((u8)value.Size()); /**< @todo Use VLE, allow more than 255 refs */
    for(int i = 0; i < value.Size(); ++i)
        dest.AddString(value[i].ref.CString()); /**< @todo Allow longer strings than 255 chars */
}

template<> void TUNDRACORE_API Attribute<EntityReference>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.AddString(value.ref.CString()); /**< @todo Allow longer strings than 255 chars */
}

template<> void TUNDRACORE_API Attribute<Variant>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.AddString(value.ToString().CString()); /**< @todo Allow longer strings than 255 chars */
}

template<> void TUNDRACORE_API Attribute<VariantList>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<u8>((u8)value.Size()); /**< @todo Use VLE, allow more than 255 refs */
    for(u32 i = 0; i < value.Size(); ++i)
        dest.AddString(value[i].ToString().CString()); /**< @todo Allow longer strings than 255 chars */
}

template<> void TUNDRACORE_API Attribute<Transform>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<float>(value.pos.x);
    dest.Add<float>(value.pos.y);
    dest.Add<float>(value.pos.z);
    dest.Add<float>(value.rot.x);
    dest.Add<float>(value.rot.y);
    dest.Add<float>(value.rot.z);
    dest.Add<float>(value.scale.x);
    dest.Add<float>(value.scale.y);
    dest.Add<float>(value.scale.z);
}

template<> void TUNDRACORE_API Attribute<Point>::ToBinary(kNet::DataSerializer& dest) const
{
    dest.Add<s32>(value.x);
    dest.Add<s32>(value.y);
}

// FROMBINARY TEMPLATE IMPLEMENTATIONS.

template<> void TUNDRACORE_API Attribute<String>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Set(ReadUtf8String(source), change);
}

template<> void TUNDRACORE_API Attribute<bool>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Set(source.Read<u8>() ? true : false, change);
}

template<> void TUNDRACORE_API Attribute<int>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Set(source.Read<s32>(), change);
}

template<> void TUNDRACORE_API Attribute<uint>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Set(source.Read<u32>(), change);
}

template<> void TUNDRACORE_API Attribute<float>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Set(source.Read<float>(), change);
}

template<> void TUNDRACORE_API Attribute<Color>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Color value;
    value.r = source.Read<float>();
    value.g = source.Read<float>();
    value.b = source.Read<float>();
    value.a = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<Quat>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Quat value;
    value.x = source.Read<float>();
    value.y = source.Read<float>();
    value.z = source.Read<float>();
    value.w = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<float2>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    float2 value;
    value.x = source.Read<float>();
    value.y = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<float3>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    float3 value;
    value.x = source.Read<float>();
    value.y = source.Read<float>();
    value.z = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<float4>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    float4 value;
    value.x = source.Read<float>();
    value.y = source.Read<float>();
    value.z = source.Read<float>();
    value.w = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<AssetReference>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    AssetReference value;
    value.ref = source.ReadString().c_str(); /**< @todo Allow longer strings than 255 chars */
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<AssetReferenceList>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    AssetReferenceList value;
    const u8 numValues = source.Read<u8>();
    for(u32 i = 0; i < numValues; ++i)
        value.Append(AssetReference(source.ReadString().c_str())); /**< @todo Allow longer strings than 255 chars */

    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<EntityReference>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    EntityReference value;
    value.ref = source.ReadString().c_str(); /**< @todo Allow longer strings than 255 chars */
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<Variant>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    std::string str = source.ReadString(); /**< @todo Allow longer strings than 255 chars */
    Variant value(String(str.c_str()));
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<VariantList>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    VariantList value;
    
    const u8 numValues = source.Read<u8>();
    for(u32 i = 0; i < numValues; ++i)
    {
        std::string str = source.ReadString(); /**< @todo Allow longer strings than 255 chars */
        value.Push(Variant(String(str.c_str()))); /// \todo This makes the variant always of type String
    }
    
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<Transform>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Transform value;
    value.pos.x = source.Read<float>();
    value.pos.y = source.Read<float>();
    value.pos.z = source.Read<float>();
    value.rot.x = source.Read<float>();
    value.rot.y = source.Read<float>();
    value.rot.z = source.Read<float>();
    value.scale.x = source.Read<float>();
    value.scale.y = source.Read<float>();
    value.scale.z = source.Read<float>();
    Set(value, change);
}

template<> void TUNDRACORE_API Attribute<Point>::FromBinary(kNet::DataDeserializer& source, AttributeChange::Type change)
{
    Point value;
    value.x = source.Read<s32>();
    value.y = source.Read<s32>();
    Set(value, change);
}

// INTERPOLATE TEMPLATE IMPLEMENTATIONS

template<> void TUNDRACORE_API Attribute<String>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<String>::Interpolate invoked! String attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<bool>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<bool>::Interpolate invoked! bool attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<AssetReference>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<AssetReference>::Interpolate invoked! AssetReference attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<AssetReferenceList>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<AssetReferenceList>::Interpolate invoked! AssetReferenceList attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<EntityReference>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<EntityReference>::Interpolate invoked! EntityReference attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<Variant>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<Variant>::Interpolate invoked! Variant attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<VariantList>::Interpolate(IAttribute* /*start*/, IAttribute* /*end*/, float /*t*/, AttributeChange::Type /*change*/)
{
    LOGERROR("Attribute<VariantList>::Interpolate invoked! VariantList attributes cannot be animated!");
}

template<> void TUNDRACORE_API Attribute<int>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<int>* startInt = dynamic_cast<Attribute<int>*>(start);
    Attribute<int>* endInt = dynamic_cast<Attribute<int>*>(end);
    if (startInt && endInt)
        Set(RoundInt(Lerp((float)startInt->Get(), (float)endInt->Get(), t)), change);
}

template<> void TUNDRACORE_API Attribute<uint>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<uint>* startUint = dynamic_cast<Attribute<uint>*>(start);
    Attribute<uint>* endUint = dynamic_cast<Attribute<uint>*>(end);
    if (startUint && endUint)
        Set((uint)RoundInt(Lerp((float)startUint->Get(), (float)endUint->Get(), t)), change);
}

template<> void TUNDRACORE_API Attribute<float>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<float>* startFloat = dynamic_cast<Attribute<float>*>(start);
    Attribute<float>* endFloat = dynamic_cast<Attribute<float>*>(end);
    if (startFloat && endFloat)
        Set(Lerp(startFloat->Get(), endFloat->Get(), t), change);
}

template<> void TUNDRACORE_API Attribute<Quat>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<Quat>* startQuat = dynamic_cast<Attribute<Quat>*>(start);
    Attribute<Quat>* endQuat = dynamic_cast<Attribute<Quat>*>(end);
    if (startQuat && endQuat)
        Set(Slerp(startQuat->Get(), endQuat->Get(), t), change);
}

template<> void TUNDRACORE_API Attribute<float2>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<float2>* startVec = dynamic_cast<Attribute<float2>*>(start);
    Attribute<float2>* endVec = dynamic_cast<Attribute<float2>*>(end);
    if (startVec && endVec)
        Set(Lerp(startVec->Get(), endVec->Get(), t), change);
}

template<> void TUNDRACORE_API Attribute<float3>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<float3>* startVec = dynamic_cast<Attribute<float3>*>(start);
    Attribute<float3>* endVec = dynamic_cast<Attribute<float3>*>(end);
    if (startVec && endVec)
        Set(Lerp(startVec->Get(), endVec->Get(), t), change);
}

template<> void TUNDRACORE_API Attribute<float4>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<float4>* startVec = dynamic_cast<Attribute<float4>*>(start);
    Attribute<float4>* endVec = dynamic_cast<Attribute<float4>*>(end);
    if (startVec && endVec)
        Set(Lerp(startVec->Get(), endVec->Get(), t), change);
}

template<> void TUNDRACORE_API Attribute<Color>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<Color>* startColor = dynamic_cast<Attribute<Color>*>(start);
    Attribute<Color>* endColor = dynamic_cast<Attribute<Color>*>(end);
    if (startColor && endColor)
    {
        const Color& startValue = startColor->Get();
        const Color& endValue = endColor->Get();
        Color newColor;
        newColor.r = Lerp(startValue.r, endValue.r, t);
        newColor.g = Lerp(startValue.g, endValue.g, t);
        newColor.b = Lerp(startValue.b, endValue.b, t);
        newColor.a = Lerp(startValue.a, endValue.a, t);
        Set(newColor, change);
    }
}

template<> void TUNDRACORE_API Attribute<Transform>::Interpolate(IAttribute* start, IAttribute* end, float t, AttributeChange::Type change)
{
    Attribute<Transform>* startTrans = dynamic_cast<Attribute<Transform>*>(start);
    Attribute<Transform>* endTrans = dynamic_cast<Attribute<Transform>*>(end);
    if (startTrans && endTrans)
    {
        const Transform& startValue = startTrans->Get();
        const Transform& endValue = endTrans->Get();
        Transform newTrans;
        newTrans.pos = Lerp(startValue.pos, endValue.pos, t);
        newTrans.SetOrientation(Slerp(startValue.Orientation(), endValue.Orientation(), t));
        newTrans.scale = Lerp(startValue.scale, endValue.scale, t);
        Set(newTrans, change);
    }
}

/// @endcond

}

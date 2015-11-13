// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "AvatarDescAsset.h"
#include "AssetAPI.h"
#include "Math/Quat.h"
#include "Math/MathFunc.h"
#include "LoggingFunctions.h"

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Core/StringUtils.h>
#include <cstring>

namespace Tundra
{
/// @todo Move to StringUtils
bool ParseBool(const String& str, bool defaultValue)
{
    if (str.Trimmed().Empty())
        return defaultValue;
    else
        return Urho3D::ToBool(str);
}

static String QuatToLegacyRexString(const Quat& q)
{
    char str[256];
    sprintf(str, "%f %f %f %f", q.w, q.x, q.y, q.z);
    return str;
}

static Quat QuatFromLegacyRexString(const String& str)
{
    // If consists of 3 components split by spaces, interpret as Euler angles
    if (str.Split(' ').Size() == 3)
    {
        float3 e = DegToRad(float3::FromString(str.CString()));
        return Quat::FromEulerZYX(e.z, e.y, e.x);
    }
    
    // Else interpret as quaternion directly
    const char* cstr = str.CString();
    if (!cstr)
        return Quat();
    if (*cstr == '(')
        ++cstr;
    Quat q;
    q.w = (float)strtod(cstr, const_cast<char**>(&cstr));
    if (*cstr == ',' || *cstr == ';')
        ++cstr;
    q.x = (float)strtod(cstr, const_cast<char**>(&cstr));
    if (*cstr == ',' || *cstr == ';')
        ++cstr;
    q.y = (float)strtod(cstr, const_cast<char**>(&cstr));
    if (*cstr == ',' || *cstr == ';')
        ++cstr;
    q.z = (float)strtod(cstr, const_cast<char**>(&cstr));
    return q;
}

static const String modifierMode[] = {
    "relative",
    "absolute",
    "cumulative"
};

AvatarDescAsset::AvatarDescAsset(AssetAPI *owner, const String &type_, const String &name_) :
    IAsset(owner, type_, name_),
    height_(0.f),
    weight_(0.f)
{
}

AvatarDescAsset::~AvatarDescAsset()
{
    Unload();
}

void AvatarDescAsset::DoUnload()
{
    mesh_ = "";
    skeleton_ = "";
    materials_.Clear();
    attachments_.Clear();
    boneModifiers_.Clear();
    morphModifiers_.Clear();
    masterModifiers_.Clear();
    animations_.Clear();
    properties_.Clear();
}

bool AvatarDescAsset::DeserializeFromData(const u8 *data, uint numBytes, bool /*allowAsynchronous*/)
{
    // Store the raw XML as a string
    avatarAppearanceXML_ = String((const char*)data, numBytes);
    
    // Then try to parse
    Urho3D::XMLFile avatarDoc(GetContext());
    // If invalid XML, empty it so we will report IsLoaded == false
    if (!avatarDoc.FromString(avatarAppearanceXML_))
    {
        LogError("Failed to deserialize AvatarDescAsset from data.");
        avatarAppearanceXML_ = "";
        return false;
    }

    ReadAvatarAppearance(avatarDoc);
    AppearanceChanged.Emit();

    assetAPI->AssetLoadCompleted(Name());
    return true;
}

bool AvatarDescAsset::SerializeTo(Vector<u8> &dst, const String &/*serializationParameters*/) const
{
    Urho3D::XMLFile avatarDoc(GetContext());
    WriteAvatarAppearance(avatarDoc);
    String str = avatarDoc.ToString();
    
    if (!str.Length())
        return false;
    
    dst.Resize(str.Length());
    memcpy(&dst[0], &str[0], str.Length());
    return true;
}

bool AvatarDescAsset::IsLoaded() const
{
    return !avatarAppearanceXML_.Empty();
}

bool AvatarDescAsset::ReadAvatarAppearance(Urho3D::XMLFile& source)
{
    URHO3D_PROFILE(Avatar_ReadAvatarAppearance);
    
    Urho3D::XMLElement avatar = source.GetRoot("avatar");
    if (avatar.IsNull())
    {
        LogError("No avatar element");
        return false;
    }

    // Get mesh & skeleton
    Urho3D::XMLElement base_elem = avatar.GetChild("base");
    if (!base_elem.IsNull())
        mesh_ = base_elem.GetAttribute("mesh");
    
    Urho3D::XMLElement skeleton_elem = avatar.GetChild("skeleton");
    if (!skeleton_elem.IsNull())
        skeleton_ = skeleton_elem.GetAttribute("name");
    
    // Get height & weight
    Urho3D::XMLElement appearance_elem = avatar.GetChild("appearance");
    if (appearance_elem.HasAttribute("height"))
        height_ = appearance_elem.GetFloat("height");
    else
        height_ = 1.8f;
    if (appearance_elem.HasAttribute("weight"))
        weight_ = appearance_elem.GetFloat("weight");
    else
        weight_ = 1.0;

    // Get materials. Texture override is no longer supported.
    materials_.Clear();
    Urho3D::XMLElement material_elem = avatar.GetChild("material");
    while (!material_elem.IsNull())
    {
        materials_.Push(material_elem.GetAttribute("name"));
        material_elem = material_elem.GetNext("material");
    }
    
    // Main transform has never been used, so we don't read it
    
    // Get attachments
    Urho3D::XMLElement attachment_elem = avatar.GetChild("attachment");
    attachments_.Clear();
    while (!attachment_elem.IsNull())
    {
        ReadAttachment(attachment_elem);
        attachment_elem = attachment_elem.GetNext("attachment");
    }
    
    // Get bone modifiers
    Urho3D::XMLElement bonemodifier_elem = avatar.GetChild("dynamic_animation");
    boneModifiers_.Clear();
    while (!bonemodifier_elem.IsNull())
    {
        ReadBoneModifierSet(bonemodifier_elem);
        bonemodifier_elem = bonemodifier_elem.GetNext("dynamic_animation");
    }
    // Get bone modifier parameters
    Urho3D::XMLElement bonemodifierparam_elem = avatar.GetChild("dynamic_animation_parameter");
    while (!bonemodifierparam_elem.IsNull())
    {
        ReadBoneModifierParameter(bonemodifierparam_elem);
        bonemodifierparam_elem = bonemodifierparam_elem.GetNext("dynamic_animation_parameter");
    }
    
    // Get morph modifiers
    Urho3D::XMLElement morphmodifier_elem = avatar.GetChild("morph_modifier");
    morphModifiers_.Clear();
    while (!morphmodifier_elem.IsNull())
    {
        ReadMorphModifier(morphmodifier_elem);
        morphmodifier_elem = morphmodifier_elem.GetNext("morph_modifier");
    }
    
    // Get master modifiers
    Urho3D::XMLElement mastermodifier_elem = avatar.GetChild("master_modifier");
    masterModifiers_.Clear();
    while (!mastermodifier_elem.IsNull())
    {
        ReadMasterModifier(mastermodifier_elem);
        mastermodifier_elem = mastermodifier_elem.GetNext("master_modifier");
    }
    
    // Get animations
    Urho3D::XMLElement animation_elem = avatar.GetChild("animation");
    animations_.Clear();
    while (!animation_elem.IsNull())
    {
        ReadAnimationDefinition(animation_elem);
        animation_elem = animation_elem.GetNext("animation");
    }
    
    // Get properties
    Urho3D::XMLElement property_elem = avatar.GetChild("property");
    properties_.Clear();
    while (!property_elem.IsNull())
    {
        String name = property_elem.GetAttribute("name");
        String value = property_elem.GetAttribute("value");
        if ((name.Length()) && (value.Length()))
            properties_[name] = value;
        
        property_elem = property_elem.GetNext("property");
    }
    
    // Refresh slave modifiers
    CalculateMasterModifiers();
    
    // Assetmap not used (deprecated), as asset refs are stored directly
    return true;
}

void AvatarDescAsset::ReadBoneModifierSet(const Urho3D::XMLElement& source)
{
    BoneModifierSet modifier_set;
    modifier_set.name_ = source.GetAttribute("name");
    unsigned num_bones = 0;
    
    Urho3D::XMLElement bones = source.GetChild("bones");
    if (!bones.IsNull())
    {
        Urho3D::XMLElement bone = bones.GetChild("bone");
        while (!bone.IsNull())
        {
            BoneModifier modifier;
            modifier.bone_name_ = bone.GetAttribute("name");
            Urho3D::XMLElement rotation = bone.GetChild("rotation");
            Urho3D::XMLElement translation = bone.GetChild("translation");
            Urho3D::XMLElement scale = bone.GetChild("scale");
            
            modifier.start_.position_ = float3::FromString(translation.GetAttribute("start").CString());
            float3 e = DegToRad(float3::FromString(rotation.GetAttribute("start").CString()));
            modifier.start_.orientation_ = Quat::FromEulerZYX(e.z, e.y, e.x);////ParseEulerAngles(rotation.GetAttribute("start").toStdString());
            modifier.start_.scale_ = float3::FromString(scale.GetAttribute("start").CString());
            
            modifier.end_.position_ = float3::FromString(translation.GetAttribute("end").CString());
            e = DegToRad(float3::FromString(rotation.GetAttribute("end").CString()));
            modifier.end_.orientation_ = Quat::FromEulerZYX(e.z, e.y, e.x);//ParseEulerAngles(rotation.GetAttribute("end").toStdString());
            modifier.end_.scale_ = float3::FromString(scale.GetAttribute("end").CString());
            
            String trans_mode = translation.GetAttribute("mode");
            String rot_mode = rotation.GetAttribute("mode");
            
            if (trans_mode == "absolute")
                modifier.position_mode_ = BoneModifier::Absolute;
            if (trans_mode == "relative")
                modifier.position_mode_ = BoneModifier::Relative;
            
            if (rot_mode == "absolute")
                modifier.orientation_mode_ = BoneModifier::Absolute;
            if (rot_mode == "relative")
                modifier.orientation_mode_ = BoneModifier::Relative;
            if (rot_mode == "cumulative")
                modifier.orientation_mode_ = BoneModifier::Cumulative;
            
            modifier_set.modifiers_.Push(modifier);
            
            bone = bone.GetNext("bone");
            ++num_bones;
        }
    }
    
    if (num_bones)
        boneModifiers_.Push(modifier_set);
}

void AvatarDescAsset::ReadBoneModifierParameter(const Urho3D::XMLElement& source)
{
    // Find existing modifier from the vector
    String name = source.GetAttribute("name");
    for (unsigned i = 0; i < boneModifiers_.Size(); ++i)
    {
        if (boneModifiers_[i].name_ == name)
        {
            boneModifiers_[i].value_ = source.GetFloat("position");
            return;
        }
    }
}

void AvatarDescAsset::ReadMorphModifier(const Urho3D::XMLElement& source)
{
    MorphModifier morph;
    
    morph.name_ = source.GetAttribute("name");
    morph.morph_name_ = source.GetAttribute("internal_name");
    morph.value_ = source.GetFloat("influence");

    morphModifiers_.Push(morph);
}

void AvatarDescAsset::ReadMasterModifier(const Urho3D::XMLElement& source)
{
    MasterModifier master;
    
    master.name_ = source.GetAttribute("name");
    master.category_ = source.GetAttribute("category");
    master.value_ = source.GetFloat("position");
    
    Urho3D::XMLElement target = source.GetChild("target_modifier");
    while (!target.IsNull())
    {
        SlaveModifier targetmodifier;
        targetmodifier.name_ = target.GetAttribute("name");
        
        String targettype = target.GetAttribute("type");
        String targetmode = target.GetAttribute("mode");
        if (targettype == "morph")
            targetmodifier.type_ = AppearanceModifier::Morph;
        if (targettype == "bone")
            targetmodifier.type_ = AppearanceModifier::Bone;
        if (targettype == "dynamic_animation")
            targetmodifier.type_ = AppearanceModifier::Bone;
            
        Urho3D::XMLElement mapping = target.GetChild("position_mapping");
        while (!mapping.IsNull())
        {
            SlaveModifier::ValueMapping new_mapping;
            new_mapping.master_ = mapping.GetFloat("master");
            new_mapping.slave_ = mapping.GetFloat("target");
            targetmodifier.mapping_.Push(new_mapping);
            mapping = mapping.GetNext("position_mapping");
        }
        
        if (targetmode == "cumulative")
            targetmodifier.mode_ = SlaveModifier::Cumulative;
        else
            targetmodifier.mode_ = SlaveModifier::Average;
        
        master.modifiers_.Push(targetmodifier);
        
        target = target.GetNext("target_modifier");
    }
    
    masterModifiers_.Push(master);
}

void AvatarDescAsset::ReadAnimationDefinitions(Urho3D::XMLFile& source)
{
    URHO3D_PROFILE(Avatar_ReadAnimationDefinitions);
    
    animations_.Clear();
    
    Urho3D::XMLElement elem = source.GetRoot().GetChild("animation");
    while (!elem.IsNull())
    {
        ReadAnimationDefinition(elem);
        elem = elem.GetNext("animation");
    }
}

void AvatarDescAsset::ReadAnimationDefinition(const Urho3D::XMLElement& elem)
{
    if (elem.GetName() != "animation")
        return;

    String id = elem.GetAttribute("id");
    if (id.Empty())
        id = elem.GetAttribute("uuid"); // legacy
    if (id.Empty())
    {
        LogError("Missing animation identifier");
        return;
    }
    
    String intname = elem.GetAttribute("internal_name");
    if (intname.Empty())
        intname = elem.GetAttribute("ogrename"); // legacy
    if (intname.Empty())
    {
        LogError("Missing mesh animation name");
        return;
    }
    
    AnimationDefinition new_def;
    new_def.id_ = id;
    new_def.animation_name_ = intname;
    new_def.name_ = elem.GetAttribute("name");
    
    new_def.looped_ = ParseBool(elem.GetAttribute("looped"), true);
    new_def.exclusive_ = ParseBool(elem.GetAttribute("exclusive"), false);
    new_def.use_velocity_ = ParseBool(elem.GetAttribute("usevelocity"), false);
    new_def.always_restart_ = ParseBool(elem.GetAttribute("alwaysrestart"), false);
    new_def.fadein_ = elem.GetFloat("fadein");
    new_def.fadeout_ = elem.GetFloat("fadeout");
    new_def.speedfactor_ = elem.GetFloat("speedfactor");
    new_def.weightfactor_ = elem.GetFloat("weightfactor");
    
    animations_.Push(new_def);
}

void AvatarDescAsset::ReadAttachment(const Urho3D::XMLElement& elem)
{
    AvatarAttachment attachment;
    
    Urho3D::XMLElement name = elem.GetChild("name");
    if (!name.IsNull())
        attachment.name_ = name.GetAttribute("value");

    Urho3D::XMLElement material = elem.GetChild("material");
    while (!material.IsNull())
    {
        attachment.materials_.Push(material.GetAttribute("name"));
        material = material.GetNext("material");
    }

    Urho3D::XMLElement category = elem.GetChild("category");
    if (!category.IsNull())
    {
        attachment.category_ = category.GetAttribute("name");
    }
    
    Urho3D::XMLElement mesh = elem.GetChild("mesh");
    if (!mesh.IsNull())
    {
        attachment.mesh_ = mesh.GetAttribute("name");
        attachment.link_skeleton_ = ParseBool(mesh.GetAttribute("linkskeleton"), false);
    }
    else
    {
        LogError("Attachment without mesh element");
        return;
    }
    
    Urho3D::XMLElement avatar = elem.GetChild("avatar");
    if (!avatar.IsNull())
    {
        Urho3D::XMLElement bone = avatar.GetChild("bone");
        if (!bone.IsNull())
        {
            attachment.bone_name_ = bone.GetAttribute("name");
            if (attachment.bone_name_ == "None")
                attachment.bone_name_ = String();
            if (!bone.GetAttribute("offset").Empty())
                attachment.transform_.position_ = float3::FromString(bone.GetAttribute("offset").CString());
            if (!bone.GetAttribute("rotation").Empty())
                attachment.transform_.orientation_ = QuatFromLegacyRexString(bone.GetAttribute("rotation"));
            if (!bone.GetAttribute("scale").Empty())
                attachment.transform_.scale_ = float3::FromString(bone.GetAttribute("scale").CString());
        }
        
        Urho3D::XMLElement polygon = avatar.GetChild("avatar_polygon");
        while (!polygon.IsNull())
        {
            uint idx = polygon.GetUInt("idx");
            attachment.vertices_to_hide_.Push(idx);
            polygon = polygon.GetNext("avatar_polygon");
        }
    }
    else
    {
        LogError("Attachment without avatar element");
        return;
    }
    
    attachments_.Push(attachment);
}

void AvatarDescAsset::SetMasterModifierValue(const String& name, float value)
{
    for(uint i = 0; i < masterModifiers_.Size(); ++i)
        if (masterModifiers_[i].name_ == name)
        {
            masterModifiers_[i].value_ = Clamp(value, 0.0f, 1.0f);
            for (uint j = 0; j < masterModifiers_[i].modifiers_.Size(); ++j)
            {
                AppearanceModifier* mod = FindModifier(masterModifiers_[i].modifiers_[j].name_, masterModifiers_[i].modifiers_[j].type_);
                if (mod)
                    mod->manual_ = false;
            }
            CalculateMasterModifiers();
            DynamicAppearanceChanged.Emit();
            return;
        }
}

void AvatarDescAsset::SetModifierValue(const String& name, float value)
{
    value = Clamp(value, 0.0f, 1.0f);

    // Check first for a morph, then for bone
    AppearanceModifier* mod = FindModifier(name, AppearanceModifier::Morph);
    if (mod)
    {
        mod->value_ = value;
        mod->manual_ = true;
        DynamicAppearanceChanged.Emit();
        return;
    }
    mod = FindModifier(name, AppearanceModifier::Bone);
    if (mod)
    {
        mod->value_ = value;
        mod->manual_ = true;
        DynamicAppearanceChanged.Emit();
    }
}

void AvatarDescAsset::SetMaterial(uint index, const String& ref)
{
    if (index >= materials_.Size())
        return;
    materials_[index] = ref;
    
    //AssetReferencesChanged();
}

void AvatarDescAsset::RemoveAttachment(uint index)
{
    if (index < attachments_.Size())
    {
        attachments_.Erase(attachments_.Begin() + index);
        AppearanceChanged.Emit();
    }
    else
        LogError("Failed to remove attachment at index " + String(index) + "! Only " + String(attachments_.Size()) + "  attachments exist on the avatar asset!");
}

void AvatarDescAsset::RemoveAttachmentsByCategory(String category)
{
    Vector<int> toRemove;

    for (uint i = 0; i < attachments_.Size(); i++)
    {
        if (attachments_[i].category_ == category)
        {
            toRemove.Push(i);
        }
    }

    // Remove the attachments, starting from the end of the vector.
    for (int i = (int)toRemove.Size()-1; i >= 0; --i)
        RemoveAttachment(toRemove[i]);
}

void AvatarDescAsset::AddAttachment(AssetPtr assetPtr)
{
    Vector<u8> data;
    bool success = assetPtr->SerializeTo(data);
    if (!success || data.Size() == 0)
    {
        LogError("AvatarDescAsset::AddAttachment: Could not serialize attachment");
        return;
    }

    String string((const char*)&data[0], data.Size());

    Urho3D::XMLFile attachDoc(GetContext());
    if (!attachDoc.FromString(string))
    {
        LogError("AvatarDescAsset::AddAttachment: Could not parse attachment data");
        return;
    }

    Urho3D::XMLElement elem = attachDoc.GetRoot("attachment");

    if (!elem.IsNull())
    {
        ReadAttachment(elem);
        AppearanceChanged.Emit();
    }
    else
    {
        LogError("AvatarDescAsset::AddAttachment: Null attachment");
    }
}

bool AvatarDescAsset::HasProperty(const String &name) const
{
    HashMap<String, String>::ConstIterator i = properties_.Find(name);
    if (i == properties_.End())
        return false;
    return i->second_.Length() > 0;
}

const String& AvatarDescAsset::GetProperty(const String& name)
{
    return properties_[name];
}

void AvatarDescAsset::CalculateMasterModifiers()
{
    for(uint i = 0; i < morphModifiers_.Size(); ++i)
        morphModifiers_[i].ResetAccumulation();

    for(uint i = 0; i < boneModifiers_.Size(); ++i)
        boneModifiers_[i].ResetAccumulation();

    for(uint i = 0; i < masterModifiers_.Size(); ++i)
        for(uint j = 0; j < masterModifiers_[i].modifiers_.Size(); ++j)
        {
            AppearanceModifier* mod = FindModifier(masterModifiers_[i].modifiers_[j].name_, masterModifiers_[i].modifiers_[j].type_);
            if (mod)
            {
                float slave_value = masterModifiers_[i].modifiers_[j].GetMappedValue(masterModifiers_[i].value_);
                mod->AccumulateValue(slave_value, masterModifiers_[i].modifiers_[j].mode_ == SlaveModifier::Average);
            }
        }
}

AppearanceModifier* AvatarDescAsset::FindModifier(const String & name, AppearanceModifier::ModifierType type)
{
    for(uint i = 0; i < morphModifiers_.Size(); ++i)
        if (morphModifiers_[i].name_ == name && morphModifiers_[i].type_ == type)
            return &morphModifiers_[i];
    for (uint i = 0; i < boneModifiers_.Size(); ++i)
        if (boneModifiers_[i].name_ == name && boneModifiers_[i].type_ == type)
            return &boneModifiers_[i];
    return 0;
}

void AvatarDescAsset::WriteAvatarAppearance(Urho3D::XMLFile& dest) const
{
    // Avatar element
    Urho3D::XMLElement avatar = dest.CreateRoot("avatar");
    
    // Version element
    {
        Urho3D::XMLElement version = avatar.CreateChild("version");
        version.SetValue("0.2");
    }
    
    // Mesh element
    {
        Urho3D::XMLElement mesh = avatar.CreateChild("base");
        mesh.SetAttribute("name", "default");
        mesh.SetAttribute("mesh", mesh_);
    }
    
    // Appearance element
    {
        Urho3D::XMLElement appearance = avatar.CreateChild("appearance");
        appearance.SetFloat("height", height_);
        appearance.SetFloat("weight", weight_);
    }

    // Skeleton element
    if (skeleton_.Length())
    {
        Urho3D::XMLElement skeleton = avatar.CreateChild("skeleton");
        skeleton.SetAttribute("name", skeleton_);
    }
    
    // Material elements
    for (uint i = 0; i < materials_.Size(); ++i)
    {
        // Append elements in submesh order
        Urho3D::XMLElement material = avatar.CreateChild("material");
        material.SetAttribute("name", materials_[i]);
    }
    
    // Attachments
    for (uint i = 0; i < attachments_.Size(); ++i)
    {
        WriteAttachment(avatar, attachments_[i], mesh_);
    }
    
    // Bone modifiers
    for (uint i = 0; i < boneModifiers_.Size(); ++i)
        WriteBoneModifierSet(avatar, boneModifiers_[i]);
    
    // Morph modifiers
    for (uint i = 0; i < morphModifiers_.Size(); ++i)
        WriteMorphModifier(avatar, morphModifiers_[i]);
    
    // Master modifiers
    for (uint i = 0; i < masterModifiers_.Size(); ++i)
        WriteMasterModifier(avatar, masterModifiers_[i]);
    
    // Animations
    for (uint i = 0; i < animations_.Size(); ++i)
        WriteAnimationDefinition(avatar, animations_[i]);
    
    // Properties
    HashMap<String, String>::ConstIterator i = properties_.Begin();
    while (i != properties_.End())
    {
        Urho3D::XMLElement prop = avatar.CreateChild("property");
        prop.SetAttribute("name", i->first_);
        prop.SetAttribute("value", i->second_);
        ++i;
    }
}

void AvatarDescAsset::WriteAnimationDefinition(Urho3D::XMLElement& dest, const AnimationDefinition& anim) const
{
    Urho3D::XMLElement elem = dest.CreateChild("animation");
    
    elem.SetAttribute("name", anim.name_);
    elem.SetAttribute("id", anim.id_);
    elem.SetAttribute("internal_name", anim.animation_name_);
    elem.SetBool("looped", anim.looped_);
    elem.SetBool("usevelocity", anim.use_velocity_);
    elem.SetBool("alwaysrestart", anim.always_restart_);
    elem.SetFloat("fadein", anim.fadein_);
    elem.SetFloat("fadeout", anim.fadeout_);
    elem.SetFloat("speedfactor", anim.speedfactor_);
    elem.SetFloat("weightfactor", anim.weightfactor_);
}

void AvatarDescAsset::WriteBoneModifierSet(Urho3D::XMLElement& dest, const BoneModifierSet& bones) const
{
    Urho3D::XMLElement parameter = dest.CreateChild("dynamic_animation_parameter");
    Urho3D::XMLElement modifier = dest.CreateChild("dynamic_animation");
    
    parameter.SetAttribute("name", bones.name_);
    parameter.SetFloat("position", bones.value_);
    modifier.SetAttribute("name", bones.name_);

    Urho3D::XMLElement base_animations = modifier.CreateChild("base_animations");
    
    Urho3D::XMLElement bonelist = modifier.CreateChild("bones");
    for (uint i = 0; i < bones.modifiers_.Size(); ++i)
        WriteBone(bonelist, bones.modifiers_[i]);
}

void AvatarDescAsset::WriteBone(Urho3D::XMLElement& dest, const BoneModifier& bone) const
{
    Urho3D::XMLElement elem = dest.CreateChild("bone");
    elem.SetAttribute("name", bone.bone_name_);
    
    Urho3D::XMLElement rotation = elem.CreateChild("rotation");
    float3 e = RadToDeg(bone.start_.orientation_.ToEulerZYX());
    rotation.SetAttribute("start", float3(e.z, e.y, e.x).ToString().c_str()); //WriteEulerAngles(bone.start_.orientation_));
    e = RadToDeg(bone.end_.orientation_.ToEulerZYX());
    rotation.SetAttribute("end", float3(e.z, e.y, e.x).ToString().c_str()); //WriteEulerAngles(bone.end_.orientation_));
    rotation.SetAttribute("mode", modifierMode[bone.orientation_mode_]);
    
    Urho3D::XMLElement translation = elem.CreateChild("translation");
    translation.SetAttribute("start", bone.start_.position_.SerializeToString().c_str());
    translation.SetAttribute("end", bone.end_.position_.SerializeToString().c_str());
    translation.SetAttribute("mode", modifierMode[bone.position_mode_]);

    Urho3D::XMLElement scale = elem.CreateChild("scale");
    scale.SetAttribute("start", bone.start_.scale_.SerializeToString().c_str());
    scale.SetAttribute("end", bone.end_.scale_.SerializeToString().c_str());
}

void AvatarDescAsset::WriteMorphModifier(Urho3D::XMLElement& dest, const MorphModifier& morph) const
{
    Urho3D::XMLElement elem = dest.CreateChild("morph_modifier");
    elem.SetAttribute("name", morph.name_);
    elem.SetAttribute("internal_name", morph.morph_name_);
    elem.SetFloat("influence", morph.value_);
}

void AvatarDescAsset::WriteMasterModifier(Urho3D::XMLElement& dest, const MasterModifier& master) const
{
    Urho3D::XMLElement elem = dest.CreateChild("master_modifier");
    elem.SetAttribute("name", master.name_);
    elem.SetFloat("position", master.value_);
    elem.SetAttribute("category", master.category_);
    for (uint i = 0; i < master.modifiers_.Size(); ++i)
    {
        Urho3D::XMLElement target_elem = elem.CreateChild("target_modifier");
        target_elem.SetAttribute("name", master.modifiers_[i].name_);
        if (master.modifiers_[i].type_ == AppearanceModifier::Morph)
            target_elem.SetAttribute("type", "morph");
        else
            target_elem.SetAttribute("type", "dynamic_animation");
        if (master.modifiers_[i].mode_ == SlaveModifier::Cumulative)
            target_elem.SetAttribute("mode", "cumulative");
        else
            target_elem.SetAttribute("mode", "average");
        for(uint j = 0; j < master.modifiers_[i].mapping_.Size(); ++j)
        {
            Urho3D::XMLElement mapping_elem = target_elem.CreateChild("position_mapping");
            mapping_elem.SetFloat("master", master.modifiers_[i].mapping_[j].master_);
            mapping_elem.SetFloat("target", master.modifiers_[i].mapping_[j].slave_);
        }
    }
}

void AvatarDescAsset::WriteAttachment(Urho3D::XMLElement& dest, const AvatarAttachment& attachment, const String& mesh) const
{
    Urho3D::XMLElement elem = dest.CreateChild("attachment");
    
    Urho3D::XMLElement name_elem = elem.CreateChild("name");
    name_elem.SetAttribute("value", attachment.name_);
    
    Urho3D::XMLElement mesh_elem = elem.CreateChild("mesh");
    mesh_elem.SetAttribute("name", attachment.mesh_);
    int link = 0;
    if (attachment.link_skeleton_)
        link = 1;
    mesh_elem.SetInt("linkskeleton", link);
    
    for(unsigned i = 0; i < attachment.materials_.Size(); ++i)
    {
        Urho3D::XMLElement material_elem = elem.CreateChild("material");
        material_elem.SetAttribute("name", attachment.materials_[i]);
    }
    
    Urho3D::XMLElement category_elem = elem.CreateChild("category");
    category_elem.SetAttribute("name", attachment.category_);
    
    Urho3D::XMLElement avatar_elem = elem.CreateChild("avatar");
    avatar_elem.SetAttribute("name", mesh);
    
    {
        String boneName = attachment.bone_name_;
        if (boneName.Empty())
            boneName= "None";

        Urho3D::XMLElement bone_elem = avatar_elem.CreateChild("bone");
        bone_elem.SetAttribute("name", boneName);
        bone_elem.SetAttribute("offset", attachment.transform_.position_.SerializeToString().c_str());
        bone_elem.SetAttribute("rotation", QuatToLegacyRexString(attachment.transform_.orientation_));
        bone_elem.SetAttribute("scale", attachment.transform_.scale_.SerializeToString().c_str());

        for(uint i = 0; i < attachment.vertices_to_hide_.Size(); ++i)
        {
            Urho3D::XMLElement polygon_elem = avatar_elem.CreateChild("avatar_polygon");
            polygon_elem.SetUInt("idx", attachment.vertices_to_hide_[i]);
        }
    }
}

}

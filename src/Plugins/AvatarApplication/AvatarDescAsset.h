// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "AvatarDescHelpers.h"
#include "IAsset.h"
#include "Signals.h"

#include <XMLFile.h>

namespace Urho3D
{
    class XMLElement;
    class XMLFile;
}

namespace Tundra
{

/// Avatar appearance description asset
class AVATAR_API AvatarDescAsset : public IAsset
{
    OBJECT(AvatarDescAsset);

public:
    AvatarDescAsset(AssetAPI *owner, const String &type_, const String &name_);

    ~AvatarDescAsset();

    /// Deserialize from XML data
    bool DeserializeFromData(const u8 *data, uint numBytes, bool allowAsynchronous) override;
    /// Serialize to XML data
    bool SerializeTo(Vector<u8> &dst, const String &serializationParameters) const override;
    /// Check if asset is loaded. Checks only XML data size
    bool IsLoaded() const;

private:
    virtual void DoUnload();

    /// Parse from XML data. Return true if successful
    bool ReadAvatarAppearance(Urho3D::XMLFile& source);
    /// Read a bone modifier
    void ReadBoneModifierSet(const Urho3D::XMLElement& source);
    /// Read the weight value (0 - 1) for an already existing bone modifier
    void ReadBoneModifierParameter(const Urho3D::XMLElement& source);
    /// Read a morph modifier
    void ReadMorphModifier(const Urho3D::XMLElement& source);
    /// Read a master modifier
    void ReadMasterModifier(const Urho3D::XMLElement& source);
    /// Read animations
    void ReadAnimationDefinitions(Urho3D::XMLFile& source);
    /// Read one animation definition
    void ReadAnimationDefinition(const Urho3D::XMLElement& elem);
    /// Read attachment mesh definition
    void ReadAttachment(const Urho3D::XMLElement& elem);
    /// Recalculate master modifier values
    void CalculateMasterModifiers();
    /// Write to XML data
    void WriteAvatarAppearance(Urho3D::XMLFile& dest) const;
    /// Write bone modifier to XML
    void WriteBoneModifierSet(Urho3D::XMLElement& dest, const BoneModifierSet& bones) const;
    /// Write bone to XML
    void WriteBone(Urho3D::XMLElement& des, const BoneModifier& bone) const;
    /// Write morph modifier to XML
    void WriteMorphModifier(Urho3D::XMLElement& dest, const MorphModifier& morph) const;
    /// Write master modifier to XML
    void WriteMasterModifier(Urho3D::XMLElement& dest, const MasterModifier& morph) const;
    /// Write animation definition to XML
    void WriteAnimationDefinition(Urho3D::XMLElement& dest, const AnimationDefinition& anim) const;
    /// Write attachment to XML
    void WriteAttachment(Urho3D::XMLElement& dest, const AvatarAttachment& attachment, const String& mesh) const;
    /// Find modifier by name and type
    AppearanceModifier* FindModifier(const String & name, AppearanceModifier::ModifierType type);
    /// Add reference to a reference vector if not empty
    void AddReference(Vector<AssetReference>& refs, const String& ref) const;
    
public:
    /// Set a master modifier value. Triggers DynamicAppearanceChanged
    void SetMasterModifierValue(const String& name, float value);
    /// Set a morph or bone modifier value. It will be brought under manual control, ie. master modifiers no longer have an effect. Triggers DynamicAppearanceChanged
    void SetModifierValue(const String& name, float value);
    /// Change a material ref
    void SetMaterial(uint index, const String& ref);
    /// Remove an attachment
    /** @param index The index of the attachment in attachments_ to be removed. */
    void RemoveAttachment(uint index);
    /// Removes all attachments of given category.
    /** @param category The name of the category of attachments to be removed. */
    void RemoveAttachmentsByCategory(String category);
    /// Add an attachment
    /** @param data The attachment to be added to the avatar.*/
    void AddAttachment(AssetPtr assetPtr);
    /// Return whether a property exists
    bool HasProperty(const String &name) const;
    /// Return property value, or empty if does not exist
    const String& GetProperty(const String& value);

    /// Mesh, skeleton, mesh materials or attachment meshes have changed. The entity using this avatar desc should refresh its appearance completely
    Signal0<void> AppearanceChanged;
    /// Dynamic properties (morphs, bone modifiers) have changed. The entity using this avatar desc should refresh those parts of the appearance
    Signal0<void> DynamicAppearanceChanged;

    /// Stores the avatar appearance XML file as raw .xml data. Note: if parameters such as bonemodifiers change "live", this won't be updated.
    String avatarAppearanceXML_;
    
    /// Avatar mesh asset reference
    String mesh_;
    /// Avatar skeleton asset reference
    String skeleton_;
    /// Avatar material asset references
    Vector<String> materials_;
    /// Height
    float height_;
    /// Weight (unused)
    float weight_;
    
    /// Animation defines
    Vector<AnimationDefinition> animations_;
    /// Attachments
    Vector<AvatarAttachment> attachments_;
    /// Bone modifiers
    Vector<BoneModifierSet> boneModifiers_;
    /// Morph modifiers
    Vector<MorphModifier> morphModifiers_;
    /// Master modifiers, which may drive either bones or morphs
    Vector<MasterModifier> masterModifiers_; 
    /// Miscellaneous properties (freedata)
    HashMap<String, String> properties_;
};

typedef SharedPtr<AvatarDescAsset> AvatarDescAssetPtr;

}

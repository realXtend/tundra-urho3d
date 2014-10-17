/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneDesc.h
    @brief  Light-weight structures for describing scene and its contents. */

#pragma once

#include "TundraCoreApi.h"
#include "SceneFwd.h"

#include <HashMap.h>
#include <Vector.h>

namespace Tundra
{

/// Cache that can be utilized on a SceneDesc import.
/** Resolving Tundras asset references can involde recursive disk searches
    that can get slow on large imports.
    @see SceneDest::assetCache. */
struct TUNDRACORE_API AssetDescCache
{
    /// First = AssetDesc::source, Second = AssetDesc::destinationName.
    typedef Pair<String, String> FileInfoPair;

    String basePath;
    HashMap<String, FileInfoPair > cache;

    /// Fills @c desc source and destinationName attributes.
    /** @return True if found and filled, false otherwise. */
    bool Fill(const String assetRef, AssetDesc &desc);

    /// Add @c desc to cache.
    /** @return Returns if added. */
    bool Add(const String &assetRef, const AssetDesc &desc);
};

/// Description of an asset (*Asset, IAsset) or an asset reference (AssetReference).
struct TUNDRACORE_API AssetDesc
{
    String source; ///< Specifies the source filename for the location of this asset.
    PODVector<unsigned char> data; ///< Specifies in-memory content for the asset data.

    /// If true, the data for this asset is loaded in memory, and specified by the member field 'data'. Otherwise,
    /// the data is loaded from disk, specified by the filename 'source'.
    bool dataInMemory;

    String subname; ///< If the source filename is a container for multiple files, subname represents name within the file.
    String typeName; ///< Type name of the asset.
    String destinationName; ///< Name for the asset in the destination asset storage.

    AssetDesc() : dataInMemory(false) {}

#define LEX_CMP(a, b) if ((a) < (b)) return true; else if ((a) > (b)) return false;

    /// Less than operator. Compares source and subname only.
    bool operator <(const AssetDesc &rhs) const
    {
        LEX_CMP(source, rhs.source)
        LEX_CMP(subname, rhs.subname)
        return false;
    }

#undef LEX_CMP

    /// Equality operator. Returns true if filenames match, false otherwise.
    bool operator ==(const AssetDesc &rhs) const { return source == rhs.source; }
};

/** Helper for tracking, managing and fixing parent child
    relationships when a group of Entities is created.

    @note This utility cannot be used by multiple imports at the same
    time. You need to create a struct per import or wait for the first
    one to return true from Ack() and call FixParenting(). */
struct TUNDRACORE_API ParentingTracker
{
    typedef Vector<entity_id_t> EntityIdList;
    typedef HashMap<entity_id_t, entity_id_t> EntityIdMap;

    EntityIdList unacked;
    EntityIdMap unackedToAcked;

    /// Returns if tracking is in progress.
    bool IsTracking() const;

    /// Add entity to tracking
    /** @note This needs to be called after Entity has
        been created and before server acks a new Entity id for it. */
    void Track(Entity *ent);

    /// Acks a expected entity.
    /** @param New acked Entity id.
        @param Old Entity id that was replaced with @c newId
        @return True if all pending unacked Entities have now been acked.
        False if still pending or no acks were expected. */
    void Ack(Scene *scene, entity_id_t newId, entity_id_t oldId);

    /// Fixes parenting that is referencing unacked and no longer existing Entities.
    /** @note Do not call this function from outside. Use Track and Ack instead. */
    void _fixParenting(Scene *scene);
};

/// Description of an attribute (IAttribute).
/** @note Attribute's type name, name and ID names are handled case-insensitively internally by the SceneAPI,
    so a case-insensitive comparison is always recommended these values. */
struct TUNDRACORE_API AttributeDesc
{
    String typeName;       ///< Attribute type name, f.ex. "Color".
    String name;           ///< Human-readable attribute name, f.ex. "Ambient light color".
    String value;          ///< Value of the attribute serialized to string, f.ex. "0.333 0.667 0.333 1.0".
    String id;             ///< Unique ID (within the parent component), i.e. the variable name, of the attribute, f.ex. "ambientLightColor".

#define LEX_CMP(a, b, sensitivity) if (a.Compare(b, sensitivity) < 0) return true; else if (a.Compare(b, sensitivity) > 0) return false;

    /// Less than operator.
    /** @note typeName, id, and name are compared case-insensitively, value as case-sensitively. */
    bool operator <(const AttributeDesc &rhs) const
    {
        LEX_CMP(typeName, rhs.typeName, false);
        LEX_CMP(id, rhs.id, false);
        LEX_CMP(name, rhs.name, false);
        LEX_CMP(value, rhs.value, true);
        return false;
    }

#undef LEX_CMP

    /// Equality operator. Returns true if all values match, false otherwise.
    /** @note typeName, id, and name are compared case-insensitively, value as case-sensitively. */
    bool operator ==(const AttributeDesc &rhs) const
    {
        return typeName.Compare(rhs.typeName, false) == 0 &&
            name.Compare(rhs.name, false) == 0 &&
            id.Compare(rhs.id, false) == 0 &&
            value  == rhs.value;
    }

    bool operator != (const AttributeDesc &rhs) const { return !(*this == rhs); }
};

/// Description of an entity-component (IComponent).
struct TUNDRACORE_API ComponentDesc
{
    u32 typeId;                         ///< Unique type ID, if available, 0xffffffff if not.
    /** @note 'typeName' Type name should not have the "EC_"-prefix anymore, enforce this with IComponent::EnsureTypeNameWithoutPrefix. */
    String typeName;                   ///< Unique type name.
    String name;                       ///< Name (if applicable).

    bool sync;                          ///< Synchronize component.
    AttributeDescList attributes;       ///< List of attributes the component has.

    ComponentDesc() : sync(true), typeId(0xffffffff) {}

    /// Equality operator. Returns true if all values match, false otherwise.
    bool operator ==(const ComponentDesc &rhs) const
    {
        return typeName == rhs.typeName && name == rhs.name && attributes == rhs.attributes;
    }

    bool operator != (const ComponentDesc &rhs) const { return !(*this == rhs); }
};

/// Description of an entity (Entity).
struct TUNDRACORE_API EntityDesc
{
    String id;                         ///< ID (if applicable).
    String name;                       ///< Name (Name::name).
    String group;                      ///< Group (Name::group).
    
    bool local;                         ///< Is entity local.
    bool temporary;                     ///< Is entity temporary.
    
    ComponentDescList components;       ///< List of components the entity has.
    EntityDescList children;            ///< List of child entities the entity has.

    /// Default constructor.
    EntityDesc() : local(false), temporary(false) {}

    /// Constructor with full input param list.
    EntityDesc(const String &entityId, const String &entityName = "", bool isLocal = false, bool isTemporary = false) :
        id(entityId),
        name(entityName),
        local(isLocal),
        temporary(isTemporary)
    {
    }

    bool IsParentFor(const EntityDesc &ent) const
    {
        if (ent.id.Empty())
            return false;

        for (unsigned i=0, len=children.Size(); i<len; ++i)
        {
            if (children[i].id == ent.id)
                return true;
        }
        return false;
    }

    /// Equality operator. Returns true if ID and name match, false otherwise.
    bool operator ==(const EntityDesc &rhs) const
    {
        return id == rhs.id && name == rhs.name /*&& local == rhs.local && temporary == rhs.temporary && components == rhs.components*/;
    }

    bool operator != (const EntityDesc &rhs) const { return !(*this == rhs); }
};


/// Description of a Scene.
/** A source-agnostic scene graph description of a Tundra scene.
    A Tundra scene consist of entities, components, attributes and assets references.
    @sa EntityDesc, ComponentDesc, AttributeDesc and AssetDesc */
struct TUNDRACORE_API SceneDesc
{
    typedef Pair<String, String> AssetMapKey;    ///< source-subname pair used to idenfity assets.
    typedef HashMap<AssetMapKey, AssetDesc> AssetMap;  ///< Map of assets.

    String filename;                   ///< Name of the file from which the description was created.
    String name;                       ///< Name.
    bool viewEnabled;                   ///< Is scene view enabled (ie. rendering-related components actually create stuff)

    EntityDescList entities;            ///< List of (root-level) entities the scene has.
    AssetMap assets;                    ///< Map of unique assets.
    AssetDescCache assetCache;          ///< Cache for assets encountered in this scene.

    /// Default constructor.
    SceneDesc(const String &_filename = "");

    /// Returns true if the scene description has no entities, false otherwise.
    bool IsEmpty() const
    {
        return entities.Empty();
    }

    /// Equality operator. Returns true if all values match, false otherwise.
    bool operator ==(const SceneDesc &rhs) const
    {
        return name == rhs.name && viewEnabled == rhs.viewEnabled && entities == rhs.entities;
    }
};

}

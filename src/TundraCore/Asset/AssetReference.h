// For conditions of distribution and use, see copyright notice in LICENSE

#pragma once

#include "TundraCoreApi.h"

#include <Str.h>

namespace Tundra
{

/// Represents a reference to an asset.
/** This structure can be used as a parameter type to an EC attribute. */
struct TUNDRACORE_API AssetReference
{
public:
    /// Default constructor
    AssetReference() {}

    /// Constructs an asset reference pointing to the given asset.
    /// @param reference The URL of the asset to point to, e.g. "local://myasset.mesh", or "http://www.website.com/texture.png".
    explicit AssetReference(const String &reference) : ref(reference) {}

    /// @note This form of a ctor should not be used, since asset references can contain unicode characters, which a std::string cannot represent.
    explicit AssetReference(const char *reference) : ref(reference) {}

    AssetReference(const String &reference, const String &type_) : ref(reference), type(type_) {}

    /// Perform assignment
    /** @note This will not modify the type if already set. Set the type explicitly if required. */
    AssetReference& operator = (const AssetReference &rhs) { ref = rhs.ref; if (type.Empty()) type = rhs.type; return *this; }
    
    bool operator ==(const AssetReference &rhs) const { return this->ref == rhs.ref; }

    bool operator !=(const AssetReference &rhs) const { return !(*this == rhs); }

    bool operator <(const AssetReference &rhs) const { return ref < rhs.ref; }

    /// Specifies the URL of the asset that is being pointed to.
    String ref;

    /// Specifies the type of the asset to load from that URL. If "", the type is interpreted directly from the ref string.
    /** Not all asset types can support this kind of interpretation. For example, avatar assets are of type .xml, which can
        only be distinguished from generic xml files by explicitly specifying the type here.

        @sa AssetAPI::GetResourceTypeFromAssetRef() */
    String type;
};

/// Represents list of asset references.
/** This structure can be used as a parameter type to an EC attribute. */
struct TUNDRACORE_API AssetReferenceList
{
    /// Default constructor.
    AssetReferenceList() {}

    /// Constructor taking preferred asset type.
    /** @param type Preferred asset type for the list. */
    AssetReferenceList(const String &preferredType) { type = preferredType; }

    /// Removes the last item in the list.
    /** The list must not be empty. If the list can be empty, call IsEmpty() before calling this function. */
    void RemoveLast() { refs.Pop(); }

    /// Perform assignment.
    /** @note This will not modify the type if already set. Set the type explicitly if required. */
    AssetReferenceList& operator = (const AssetReferenceList &rhs) { refs = rhs.refs; if (type.Empty()) type = rhs.type; return *this; }

    /// Removes empty items
    void RemoveEmpty()
    {
        unsigned size = refs.Size();
        for(unsigned i = size - 1; i < size; --i)
        {
            if (refs[i].ref.Trimmed().Empty())
                refs.Erase(refs.Begin() + i);
        }
    }

    /// Return size of the list.
    int Size() const { return refs.Size(); }

    /// Inserts @ref at the end of the list.
    void Append(const AssetReference &ref) { refs.Push(ref); }

    /// Returns true if the list contains no items, false otherwise.
    bool IsEmpty() const { return refs.Empty(); }

    /// Sets new value in the list.
    /** @param i Index.
        @ref New asset reference value. */
    void Set(int i, const AssetReference& ref)
    {
        if (i >= 0 && i < (int)refs.Size())
            refs[i] = ref;
    }

    /// Subscript operator. If index @c i is invalid and empty AssetReference is returned.
    /** @note Doesn't return reference for script-compatibility/safety.*/
    const AssetReference operator[] (int i)
    {
        if (i < 0 || i >= (int)refs.Size())
            return AssetReference();
        else
            return refs[i];
    }

    /// @overload
    /** @note Doesn't return reference for script-compatibility/safety. */
    const AssetReference operator[] (int i) const
    {
        if (i < 0 || i >= (int)refs.Size())
            return AssetReference();
        else
            return refs[i];
    }

    /// Returns true if @c rhs is equal to this list, otherwise false.
    bool operator ==(const AssetReferenceList &rhs) const 
    {
        if (this->refs.Size() != rhs.refs.Size())
            return false;
        for(uint i = 0; i < (uint)this->refs.Size(); ++i)
            if (this->refs[i].ref != rhs.refs[i].ref)
                return false;
        return true;
    }

    /// Returns true if @c rhs is not equal to this list, otherwise false.
    bool operator !=(const AssetReferenceList &rhs) const { return !(*this == rhs); }

    /// List of asset references.
    Vector<AssetReference> refs;

    /// Preferred type for asset refs in the list
    /** @sa AssetReference::type;
        @sa AssetAPI::GetResourceTypeFromAssetRef() */
    String type;
};

}

// For conditions of distribution and use, see copyright notice in license.txt

#pragma once

namespace Tundra
{

template<typename T>
SharedPtr<T> AssetAPI::FindAsset(String assetRef) const
{
    return Urho3D::DynamicCast<T>(FindAsset(assetRef));
}

template<typename T>
Vector<SharedPtr<T> > AssetAPI::AssetsOfType() const
{
    Vector<SharedPtr<T> > ret;
    for(AssetMap::const_iterator i = assets.begin(); i != assets.end(); ++i)
    {
        SharedPtr<T> asset = Urho3D::DynamicCast<T>(i->second);
        if (asset)
            ret.push_back(asset);
    }
    return ret;
}

template<typename T>
SharedPtr<T> AssetAPI::AssetProvider() const
{
    const Vector<AssetProviderPtr> providers = AssetProviders();
    for(uint i = 0; i < providers.Size(); ++i)
    {
        SharedPtr<T> provider = Urho3D::DynamicCast<T>(providers[i]);
        if (provider)
            return provider;
    }
    return SharedPtr<T>();
}

}

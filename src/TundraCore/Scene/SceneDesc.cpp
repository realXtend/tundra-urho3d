// For conditions of distribution and use, see copyright notice in LICENSE

#include "StableHeaders.h"
#include "SceneDesc.h"

#include "Scene.h"
#include "Entity.h"
#include "IAttribute.h"
#include "EntityReference.h"

#include <FileSystem.h>
#include <Log.h>

namespace Tundra
{

// SceneDesc

SceneDesc::SceneDesc(const String &_filename) :
    viewEnabled(false),
    filename(_filename)
{
    if (!filename.Empty())
        assetCache.basePath = GetPath(_filename);
}

// AssetDescCache

bool AssetDescCache::Fill(const String assetRef, AssetDesc &desc)
{
    bool found = cache.Contains(assetRef);
    if (found)
    {
        FileInfoPair value = cache[assetRef];
        desc.source = value.first_;
        desc.destinationName = value.second_;
    }
    return found;
}

bool AssetDescCache::Add(const String &assetRef, const AssetDesc &desc)
{
    if (cache.Contains(assetRef) || desc.source.Empty() || desc.destinationName.Empty())
        return false;

    cache[assetRef] = FileInfoPair(desc.source, desc.destinationName);
    return true;
}

// ParentingTracker

bool ParentingTracker::IsTracking() const
{
    return !unacked.Empty();
}

void ParentingTracker::Track(Entity *ent)
{
    if (ent)
    {
        LOGDEBUGF("[ParentingTracker]: Tracking unacked id %d", ent->Id());
        unacked.Push(ent->Id());
    }
}

void ParentingTracker::Ack(Scene *scene, entity_id_t newId, entity_id_t oldId)
{
    // Check that we are tracking this entity.
    if (unacked.Empty() || !unacked.Contains(oldId))
        return;
    while (unacked.Contains(oldId))
        unacked.Remove(oldId);
    
    // Store the new to old id for later processing.
    unackedToAcked[oldId] = newId;
    
    // If new ids for all tracked entities are now known, return true.
    if (unacked.Empty())
    {
        _fixParenting(scene);
        unackedToAcked.Clear();
    }
}

void ParentingTracker::_fixParenting(Scene *scene)
{
    LOGINFOF("[ParentingTracker]: Received new ids for %d tracked Entities. Processing scene hierarchy.", unackedToAcked.Size());
    
    /** @todo Check and fix ent->Parent() stuff here!? See if ent->Parent()->Id()
        is old unacked or new acked at this point! Or if there is a better place 
        to handle this adjustment? */

    Vector<Entity*> entitiesToCheck;
    
    Vector<entity_id_t> ackedIds = unackedToAcked.Values();
    for (unsigned ai=0, ailen=ackedIds.Size(); ai<ailen; ++ai)
    {
        EntityPtr ent = scene->EntityById(ackedIds[ai]);
        if (!ent)
        {
            LOGWARNINGF("[ParentingTracker]: Failed to find Entity by new acked id %d", ackedIds[ai]);
            continue; 
        }
        entitiesToCheck.Push(ent);
    }
    scene->FixPlaceableParentIds(entitiesToCheck, unackedToAcked, AttributeChange::Replicate, true);
}

}

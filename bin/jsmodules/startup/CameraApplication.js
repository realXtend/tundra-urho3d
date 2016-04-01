// A startup script that hooks to scene added & scene cleared signals, and creates a local freelook camera upon either signal.
// Also adds Locate functionality to the SceneStructureWindow.

if (!framework.IsHeadless())
{
    framework.Scene().SceneCreated.Connect(OnSceneCreated);
}

var _scene = null;
var cameraEntityId = 0;
var createdCameraEntityId = 0;

function OnSceneCreated(scene)
{
    if (scene != null)
    {
        scene.SceneCleared.Connect(OnSceneCleared);
        scene.EntityCreated.Connect(OnEntityCreated)
        CreateCamera(scene);
    }
}

function OnSceneCleared(scene)
{
    CreateCamera(scene);
}

function OnEntityCreated(entity, change)
{
    // This was the signal for our camera, ignore
    if (entity.id == cameraEntityId)
        return;

    // If a freelookcamera entity is loaded from the scene, use it instead; delete the one we created
    if (entity.name == "FreeLookCamera")
    {
        if (entity.camera != null && entity.placeable != null)
        {
            entity.camera.SetActive();
            if (createdCameraEntityId != 0)
            {
                scene.RemoveEntity(createdCameraEntityId);
                createdCameraEntityId = 0;
            }
            cameraEntityId = entity.id;
        }
    }
    // If a camera spawnpos entity is loaded, copy the transform
    if (entity.name == "FreeLookCameraSpawnPos" && _scene)
    {
        var cameraEntity = _scene.EntityById(cameraEntityId);
        if (cameraEntity)
            cameraEntity.placeable.transform = entity.placeable.transform;
    }
}

function CreateCamera(scene)
{
    _scene = scene;

    if (scene.EntityByName("FreeLookCamera") != null)
        return;

    var entity = scene.CreateLocalEntity(["Placeable", "Script", "Camera", "Name"]);
    entity.name = "FreeLookCamera";
    entity.temporary = true;
    entity.script.runOnLoad = true;
    entity.script.scriptRef = new AssetReference("local://FreeLookCamera.js");

    cameraEntityId = entity.id;
    createdCameraEntityId = entity.id;
}

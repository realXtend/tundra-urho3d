setlocal EnableDelayedExpansion

cd..
doxygen BulletPhysicsBindings\BulletPhysicsBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe BulletPhysicsBindings\BulletPhysicsDocs\xml BulletPhysicsBindings . PhysicsConstraint PhysicsMotor PhysicsWorld PhysicsRaycastResult RigidBody VolumeTrigger _float2 _float3 _float4 _Quat _float3x3 _float3x4 _float4x4 _Ray _Color _Point _Transform _Entity _IComponent _IAsset _IAssetStorage _IAssetTransfer _RayQueryResult

setlocal EnableDelayedExpansion

cd..
doxygen BulletPhysicsBindings\BulletPhysicsBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe BulletPhysicsBindings\BulletPhysicsDocs\xml BulletPhysicsBindings . PhysicsConstraint PhysicsMotor PhysicsWorld PhysicsRaycastResult RigidBody VolumeTrigger 
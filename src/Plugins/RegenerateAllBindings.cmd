set ORIGINAL_DIR=%CD%

cd JavaScript\CoreBindings
call RegenerateCoreBindings.cmd
cd %ORIGINAL_DIR%

cd JavaScript\JavaScriptBindings
call RegenerateJavaScriptBindings.cmd
cd %ORIGINAL_DIR%

cd BulletPhysics\BulletPhysicsBindings
call RegenerateBulletPhysicsBindings.cmd
cd %ORIGINAL_DIR%

cd UrhoRenderer\UrhoRendererBindings
call RegenerateUrhoRendererBindings.cmd
cd %ORIGINAL_DIR%

cd TundraLogic\TundraLogicBindings
call RegenerateTundraLogicBindings.cmd
cd %ORIGINAL_DIR%

cd AvatarApplication\AvatarApplicationBindings
call RegenerateAvatarApplicationBindings.cmd
cd %ORIGINAL_DIR%


setlocal EnableDelayedExpansion

set TUNDRACORE_DIR=%CD%\..\..\..\TundraCore
set ORIGINAL_DIR=%CD%

copy %CD%\CoreBindings.doxyfile %TUNDRACORE_DIR%
cd %TUNDRACORE_DIR%
doxygen CoreBindings.doxyfile
cd %ORIGINAL_DIR%
bin\release\BindingsGenerator.exe %TUNDRACORE_DIR%\CoreDocs\xml ../CoreBindings %TUNDRACORE_DIR% Scene Entity EntityAction IComponent AttributeChange Framework FrameAPI SceneAPI ConfigAPI AssetAPI IAsset AssetReference AssetReferenceList Transform float3 float4 Quat float3x3 float3x4 float4x4

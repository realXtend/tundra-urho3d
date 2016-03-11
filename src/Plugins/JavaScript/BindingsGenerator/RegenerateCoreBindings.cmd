setlocal EnableDelayedExpansion

set TUNDRACORE_DIR=%CD%\..\..\..\TundraCore
set ORIGINAL_DIR=%CD%

cd %TUNDRACORE_DIR%
doxygen ..\Plugins\JavaScript\BindingsGenerator\CoreBindings.doxyfile
cd %ORIGINAL_DIR%
bin\release\BindingsGenerator.exe ..\CoreBindings\CoreDocs\xml ..\CoreBindings %TUNDRACORE_DIR% Scene Entity EntityAction IComponent Name DynamicComponent AttributeChange Framework FrameAPI SceneAPI ConfigAPI AssetAPI IAsset AssetReference AssetReferenceList Transform float3 float4 Quat float3x3 float3x4 float4x4

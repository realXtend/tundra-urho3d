setlocal EnableDelayedExpansion

set TUNDRACORE_DIR=%CD%\..\..\..\TundraCore
set ORIGINAL_DIR=%CD%

cd %TUNDRACORE_DIR%
doxygen ..\Plugins\JavaScript\CoreBindings\CoreBindings.doxyfile
cd %ORIGINAL_DIR%
..\BindingsGenerator\bin\release\BindingsGenerator.exe CoreDocs\xml . %TUNDRACORE_DIR% Scene Entity EntityAction IComponent Name DynamicComponent AttributeChange Framework FrameAPI SceneAPI ConfigAPI AssetAPI IAsset IAssetStorage IAssetTransfer IAssetBundle ConsoleAPI InputAPI InputContext KeyEvent MouseEvent AssetReference AssetReferenceList RayQueryResult Transform Color Point _float3 _float4 _Quat _float3x3 _float3x4 _float4x4 _Ray

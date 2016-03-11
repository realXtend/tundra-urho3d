setlocal EnableDelayedExpansion

cd..
doxygen UrhoRendererBindings\UrhoRendererBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe UrhoRendererBindings\UrhoRendererDocs\xml UrhoRendererBindings . Placeable _float3 _float4 _Quat _float3x3 _float3x4 _float4x4 _Entity _IComponent _IAsset

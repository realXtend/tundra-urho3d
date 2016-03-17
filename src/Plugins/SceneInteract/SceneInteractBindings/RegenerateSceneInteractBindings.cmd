setlocal EnableDelayedExpansion

cd..
doxygen SceneInteractBindings\SceneInteractBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe SceneInteractBindings\SceneInteractDocs\xml SceneInteractBindings . SceneInteract _float2 _float3 _float4 _Quat _float3x3 _float3x4 _float4x4 _Ray _Color _Point _Transform _Entity _IComponent _RayQueryResult

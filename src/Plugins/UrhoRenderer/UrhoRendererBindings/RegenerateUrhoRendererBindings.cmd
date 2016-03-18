setlocal EnableDelayedExpansion

cd..
doxygen UrhoRendererBindings\UrhoRendererBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe UrhoRendererBindings\UrhoRendererDocs\xml UrhoRendererBindings . AnimationController Camera EnvironmentLight Fog Light Mesh ParticleSystem Placeable Sky Terrain WaterPlane GraphicsWorld UrhoRenderer
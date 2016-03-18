setlocal EnableDelayedExpansion

cd..
doxygen SceneInteractBindings\SceneInteractBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe SceneInteractBindings\SceneInteractDocs\xml SceneInteractBindings . SceneInteract

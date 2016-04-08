setlocal EnableDelayedExpansion

cd..
doxygen AvatarApplicationBindings\AvatarApplicationBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe AvatarApplicationBindings\AvatarApplicationDocs\xml AvatarApplicationBindings . AvatarDescAsset Avatar
setlocal EnableDelayedExpansion

set TUNDRACORE_DIR=%CD%\..\..\..\TundraCore
set ORIGINAL_DIR=%CD%

copy %CD%\CoreBindings.doxyfile %TUNDRACORE_DIR%
cd %TUNDRACORE_DIR%
doxygen CoreBindings.doxyfile
cd %ORIGINAL_DIR%
bin\release\BindingsGenerator.exe %TUNDRACORE_DIR%\CoreDocs\xml ../CoreBindings %TUNDRACORE_DIR% Scene Entity IComponent

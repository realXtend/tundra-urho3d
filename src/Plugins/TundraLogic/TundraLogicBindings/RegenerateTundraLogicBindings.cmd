setlocal EnableDelayedExpansion

cd..
doxygen TundraLogicBindings\TundraLogicBindings.doxyfile
..\JavaScript\BindingsGenerator\bin\release\BindingsGenerator.exe TundraLogicBindings\TundraLogicDocs\xml TundraLogicBindings . Client Server SyncManager UserConnection
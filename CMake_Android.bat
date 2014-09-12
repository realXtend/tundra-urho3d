@echo off
set TUNDRA_DIR=%CD%
set TUNDRA_DEP_PATH=%CD%\deps-android
set URHO3D_HOME=%TUNDRA_DEP_PATH%\urho3d
cd src\android
cmake -G "Unix Makefiles" -DANDROID=1 -DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake ../..
if ERRORLEVEL 0 goto Success
goto End
:Success
cd ..\..
echo CMake for Android completed. Go to src\android directory and execute make 
echo to build Tundra-urho3d. Then use eg. ant debug to package the apk. Refer 
echo to README.md for details
goto End
:End

@echo off

set TUNDRA_DIR=%CD%
set DEPS=%CD%\deps-android

cd src\android

cmake ../.. ^
    -G "Unix Makefiles" -DANDROID=1 ^
    -DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake ^
    -DURHO3D_HOME=%DEPS%\urho3d ^
    -DMATHGEOLIB_HOME=%DEPS%\MathGeoLib\build ^
    -DKNET_HOME=%DEPS%\kNet ^
    -DGTEST_HOME=%DEPS%\gtest ^
    -DCURL_HOME=%DEPS%\curl\build ^
    -DZZIPLIB_HOME=%DEPS%\zziplib

if ERRORLEVEL 0 goto Success
goto End

:Success
cd ..\..
echo.
echo CMake for Android completed. Go to src\android directory and execute make 
echo to build Tundra-urho3d. Then use eg. ant debug to package the apk. Refer 
echo to README.md for details
goto End

:End

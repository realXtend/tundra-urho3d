@echo off
echo.

:: Enable the delayed environment variable expansion needed in VSConfig.cmd.
setlocal EnableDelayedExpansion

:: Populate path variables
cd ..\..
set ORIGINAL_PATH=%PATH%
set PATH=%PATH%;"%CD%\tools\Windows\Utils"

:: TOOLS is the path where the Windows build scripts reside.
set TOOLS=%CD%\tools\Windows
set TUNDRA_DIR="%CD%"
set TUNDRA_BIN=%CD%\bin
set DEPS=%CD%\deps-android

cecho {F0}This script fetches and builds all Tundra dependencies for Android{# #}{\n}
echo.
cecho {0A}Requirements for a successful execution:{# #}{\n}
echo    1. Install and make sure 'git' and 'svn' are accessible from PATH.
echo    2. Install Android SDK and NDK and Apache Ant. Make sure Ant and the
echo       following Android SDK directories are accessible from PATH.
echo       - YourAndroidSDKInstallDir\tools
echo       - YourAndroidSDKInstallDir\platform-tools
echo    3. Setup ANDROID_NDK environment variable to point to the root of the
echo       Android NDK.
echo    4. Setup ANDROID_SDK environment variable to point to the root of the
echo       Android SDK.
echo    5. Have a 'make' utility accessible from PATH, for example Android NDK or
echo       MinGW make. You can eg. copy the make utility to a directory that
echo       is already accessible from PATH; the NDK does not otherwise need to
echo       be accessible.
echo    6. Execute this file from a normal Windows Command Prompt.
echo.
echo If you are not ready with the above, press Ctrl-C to abort!
pause
echo.

:: Setup env for windows build script
:: todo Can we assume x86 for android?

set TARGET_ARCH=x86
set GENERATOR="Unix Makefiles" -DCMAKE_MAKE_PROGRAM:FILEPATH=make

set TUNDRA_ANDROID=1
set TUNDRA_INIT_MSVC_ENV=FALSE
set TUNDRA_TOOLCHAIN=-DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake

cd %TOOLS%
call BuildDeps.cmd

endlocal
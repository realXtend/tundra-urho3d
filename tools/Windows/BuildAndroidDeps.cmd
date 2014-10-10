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


:: Make sure deps folder exists.
IF NOT EXIST "%DEPS%". mkdir "%DEPS%"

cecho {F0}This script fetches and builds all Tundra dependencies for Android{# #}{\n}
echo.
cecho {0A}Requirements for a successful execution:{# #}{\n}
echo    1. Install Git and make sure 'git' is accessible from PATH.
echo     - http://code.google.com/p/tortoisegit/
echo    2. Install Android SDK and NDK and make sure they are accessible from PATH
echo    3. Setup ANDROID_NDK environment variable to point to the root of the
echo       Android NDK.
echo    4. Have a make utility accessible from PATH, for example Android NDK or
echo       MinGW make
echo If you are not ready with the above, press Ctrl-C to abort!
pause
echo.

:: MathGeoLib
IF NOT EXIST "%DEPS%\MathGeoLib\". (
    cecho {0D}Cloning MathGeoLib into "%DEPS%\MathGeoLib".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/juj/MathGeoLib MathGeoLib
    cd "%DEPS%\MathGeoLib\"
    IF NOT EXIST "%DEPS%\MathGeoLib\.git" GOTO :ERROR
) ELSE (
    cd "%DEPS%\MathGeoLib\"
    git pull
)

cecho {0D}Running CMake for MathGeoLib.{# #}{\n}
cmake . -G "Unix Makefiles" -DANDROID=1 -DCMAKE_INSTALL_PREFIX=%DEPS%\MathGeoLib\build -DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Building MathGeoLib. Please be patient, this will take a while.{# #}{\n}
make -j%NUMBER_OF_PROCESSORS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: Install the correct build type into MathGeoLib/build
cecho {0D}Installing MathGeoLib{# #}{\n}
make install
IF NOT %ERRORLEVEL%==0 GOTO :ERROR


:: kNet
IF NOT EXIST "%DEPS%\kNet\". (
    cecho {0D}Cloning kNet into "%DEPS%\kNet".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/juj/kNet kNet
    cd "%DEPS%\kNet\"
    IF NOT EXIST "%DEPS%\kNet\.git" GOTO :ERROR
    git checkout master
) ELSE (
    cd "%DEPS%\kNet\"
    git pull
)

cecho {0D}Running CMake for kNet.{# #}{\n}
cmake . -G "Unix Makefiles" -DANDROID=1 -DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Building kNet. Please be patient, this will take a while.{# #}{\n}
make -j%NUMBER_OF_PROCESSORS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR


:: Urho3D
:: latest master for now, if a "last known good version" is not needed
IF NOT EXIST "%DEPS%\urho3d\". (
    cecho {0D}Cloning Urho3D into "%DEPS%\urho3d".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/urho3d/Urho3D.git urho3d
    cd "%DEPS%\urho3d\"
    IF NOT EXIST "%DEPS%\urho3d\.git" GOTO :ERROR
) ELSE (
    cd "%DEPS%\urho3d\"
    git pull
)

cecho {0D}Running CMake for Urho3D.{# #}{\n}
cd Source\Android
cmake .. -G "Unix Makefiles" -DANDROID=1 -DURHO3D_ANGELSCRIPT=0 -DURHO3D_LUA=0 -DURHO3D_TOOLS=0 -DURHO3D_NETWORK=0 -DURHO3D_PHYSICS=0 -DURHO3D_LIB_TYPE=SHARED -DLIBRARY_OUTPUT_PATH_ROOT=. -DCMAKE_TOOLCHAIN_FILE=%TUNDRA_DIR%\cmake\android.toolchain.cmake
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Building %BUILD_TYPE% Urho3D. Please be patient, this will take a while.{# #}{\n}
make -j%NUMBER_OF_PROCESSORS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR


echo.
%TOOLS%\Utils\cecho {0A}Tundra dependencies built.{# #}{\n}
set PATH=%ORIGINAL_PATH%
cd %TOOLS%
GOTO :EOF

:ERROR
echo.
%TOOLS%\Utils\cecho {0C}An error occurred! Aborting!{# #}{\n}
set PATH=%ORIGINAL_PATH%
cd %TOOLS%
pause

endlocal
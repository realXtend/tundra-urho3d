@echo off
echo.

:: Enable the delayed environment variable expansion needed in VSConfig.cmd.
setlocal EnableDelayedExpansion

:: Make sure we're running in Visual Studio Command Prompt
IF "%VSINSTALLDIR%"=="" (
   Utils\cecho {0C}Batch file not executed from Visual Studio Command Prompt - cannot proceed!{# #}{\n}
   GOTO :ERROR
)

:: Set up variables depending on the used Visual Studio version
call VSConfig.cmd %1

:: Set up variables depending on the used build type.
set BUILD_TYPE=%2

:: Possible build types provided by CMake
set BUILD_TYPE_MINSIZEREL=MinSizeRel
set BUILD_TYPE_RELEASE=Release
set BUILD_TYPE_RELWITHDEBINFO=RelWithDebInfo
set BUILD_TYPE_DEBUG=Debug
set BUILD_TYPE_DEFAULT=%BUILD_TYPE_RELWITHDEBINFO%
IF "!BUILD_TYPE!"=="" (
    set BUILD_TYPE=%BUILD_TYPE_DEFAULT%
    Utils\cecho {0E}VSConfig.cmd: Warning: BUILD_TYPE not specified - using the default %BUILD_TYPE_DEFAULT%{# #}{\n}
    pause
)
IF NOT !BUILD_TYPE!==%BUILD_TYPE_MINSIZEREL% IF NOT !BUILD_TYPE!==%BUILD_TYPE_RELEASE% IF NOT !BUILD_TYPE!==%BUILD_TYPE_RELWITHDEBINFO% IF NOT !BUILD_TYPE!==%BUILD_TYPE_DEBUG% (
    Utils\cecho {0C}VSConfig.cmd: Invalid or unsupported CMake build type passed: !BUILD_TYPE!. Cannot proceed, aborting!{# #}{\n}
    pause
    GOTO :EOF
)
:: DEBUG_OR_RELEASE and DEBUG_OR_RELEASE_LOWERCASE are "Debug" and "debug" for Debug build and "Release" and "release"
:: for all of the Release variants. Lowercase version exists for Qt/nmake/jom.
:: POSTFIX_D, POSTFIX_UNDERSCORE_D and POSTFIX_UNDERSCORE_DEBUG are helpers for performing file copies and checking
:: for existence of files. In release build these variables are empty.
set DEBUG_OR_RELEASE=Release
set DEBUG_OR_RELEASE_LOWERCASE=release
set POSTFIX_D=
set POSTFIX_UNDERSCORE_D=
set POSTFIX_UNDERSCORE_DEBUG=
IF %BUILD_TYPE%==Debug (
    set DEBUG_OR_RELEASE=Debug
    set DEBUG_OR_RELEASE_LOWERCASE=debug
    set POSTFIX_D=d
    set POSTFIX_UNDERSCORE_D=_d
    set POSTFIX_UNDERSCORE_DEBUG=_debug
)

:: Make sure deps folder exists.
IF NOT EXIST "%DEPS%". mkdir "%DEPS%"

:: If we use VS2008, framework path (for msbuild) may not be correctly set. Manually attempt to add in that case
IF %VS_VER%==vs2008 set PATH=C:\Windows\Microsoft.NET\Framework\v3.5;%PATH%

:: Print user-defined variables
cecho {F0}This script fetches and builds all Tundra dependencies{# #}{\n}
echo.
cecho {0A}Script configuration:{# #}{\n}
cecho {0D}  CMake Generator      = %GENERATOR%{# #}{\n}
echo    - Passed to CMake -G option.
cecho {0D}  Target Architecture  = %TARGET_ARCH%{# #}{\n}
echo    - Whether were doing 32-bit (x86) or 64-bit (x64) build.
cecho {0D}  Dependency Directory = %DEPS%{# #}{\n}
echo    - The directory where Tundra dependencies are fetched and built.
cecho {0D}  Build Type           = %BUILD_TYPE%{# #}{\n}
echo    - The used build type for the dependencies.
echo      Defaults to RelWithDebInfo if not specified.
IF %BUILD_TYPE%==MinSizeRel cecho {0E}     WARNING: MinSizeRel build can suffer from a significant performance loss.{# #}{\n}

:: Print scripts usage information
cecho {0A}Requirements for a successful execution:{# #}{\n}
echo    1. Install Git and make sure 'git' is accessible from PATH.
echo     - http://code.google.com/p/tortoisegit/
echo    2. Install DirectX SDK June 2010.
echo     - http://www.microsoft.com/download/en/details.aspx?id=6812
echo    3. Install CMake and make sure 'cmake' is accessible from PATH.
echo     - http://www.cmake.org/
echo    4. Install Visual Studio 2008/2010 (Express is ok, but see section 5).
echo     - http://www.microsoft.com/visualstudio/eng/downloads
cecho {0E}   5. Optional: Make sure you have the Visual Studio x64 tools installed{# #}{\n}
cecho {0E}      before installing the Visual Studio Service Pack 1 (section 6), if{# #}{\n}
cecho {0E}      wanting to build Tundra as a 64-bit application.{# #}{\n}
cecho {0E}      NOTE: The x64 tools are not available for the Express editions.{# #}{\n}
echo    6. Install Visual Studio 2008/2010 Service Pack 1.
echo     - http://www.microsoft.com/en-us/download/details.aspx?id=23691
echo    7. Install Windows SDK.
echo     - http://www.microsoft.com/download/en/details.aspx?id=8279
echo    8. Execute this file from Visual Studio 2008/2010 ^(x64^) Command Prompt.

echo If you are not ready with the above, press Ctrl-C to abort!
pause
echo.

:: Urho3D engine
:: latest master for now, if a "last known good version" is not needed
IF NOT EXIST "%DEPS%\urho3d\". (
    cecho {0D}Cloning Urho3D into "%DEPS%\urho3d".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/urho3d/Urho3D.git urho3d
    cd "%DEPS%\urho3d\"
    git checkout 671d2c45ac75f69d2f073f8bf6f4dad8f517971b
    IF NOT EXIST "%DEPS%\urho3d\.git" GOTO :ERROR
)

cecho {0D}Running CMake for Urho3D.{# #}{\n}
cd "%DEPS%\urho3d\"
IF NOT EXIST "Build" mkdir "Build"
cd Build
IF %TARGET_ARCH%==x64 (
    set URHO3D_64BIT=1
) ELSE (
    set URHO3D_64BIT=0
)
cmake ../Source -G %GENERATOR% -DURHO3D_LIB_TYPE=SHARED -DURHO3D_64BIT=%URHO3D_64BIT% -DURHO3D_ANGELSCRIPT=0 -DURHO3D_LUA=0 -DURHO3D_TOOLS=0
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Building %BUILD_TYPE% Urho3D. Please be patient, this will take a while.{# #}{\n}
MSBuild Urho3D.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%NUMBER_OF_PROCESSORS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Deploying Urho3D DLL to Tundra bin\ directory.{# #}{\n}
copy /Y "%DEPS%\urho3D\Bin\*.dll" "%TUNDRA_BIN%"
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

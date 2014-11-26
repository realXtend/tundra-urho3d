:: This batch file expects build type as the first parameter and a valid CMake generator string as the second
:: parameter, e.g. BuildTundra.cmd RelWithDebInfo "Visual Studio 12 Win64". If no CMake generator is provided,
:: the default generator (VS 2010 x86 currently) is used. Rest of the parameters are passed for the MSBuild call.
:: Typically you may want to pass at least /m to enable parallel build.

@echo off

:: Enable the delayed environment variable expansion needed in VSConfig.cmd.
setlocal EnableDelayedExpansion

set CALL_CECHO=%CD%\Utils\cecho

:: TODO Calling VSConfig.cmd here and once again in RunCMake.cmd.
call VSConfig.cmd %2
:: If TARGET_ARCH is empty, VSConfig has failed.
IF "%TARGET_ARCH%"=="" GOTO :Error

set BUILD_PATH=build-%VS_VER%-%TARGET_ARCH%
set BUILD_TYPE=%1
IF "%BUILD_TYPE%"=="" set BUILD_TYPE=RelWithDebInfo
:: If no build type would be passed for MSBuild, it seems to default to Debug.

call RunCMake.cmd %2

cd ..\..\%BUILD_PATH%

%CALL_CECHO% {0D}Building %BUILD_TYPE% Tundra-Urho3D.{# #}{\n}
MSBuild tundra-urho3d.sln /p:Configuration=%BUILD_TYPE% %3 %4 %5 %6 %7 %8 %9
IF NOT %ERRORLEVEL%==0 GOTO :Error
echo.

%CALL_CECHO% {0A}%BUILD_TYPE% Tundra-Urho3D build finished.{# #}{\n}
goto :End

:Error
echo.
%CALL_CECHO% {0C}%BUILD_TYPE% Tundra-Urho3D build failed!{# #}{\n}

:End
:: Finish in same directory we started in.
cd ..\tools\Windows
endlocal

:: This batch file expects a valid CMake generator string passed as the first parameter, or no parameters at all.
:: If no CMake generator is provided, the default generator (VS 2010 x86 currently) is used. All of the parameters
:: provided, if any, are passed to the CMake invocation.

@echo off
echo.

:: Enable the delayed environment variable expansion needed in VSConfig.cmd.
setlocal EnableDelayedExpansion

call VSConfig.cmd %1

cd ..\..

:: Print user-defined variables
cecho {0A}Script configuration:{# #}{\n}
echo CMake Generator = %GENERATOR%
echo All arguments   = %*

echo.

SET BUILD_PATH=build-%VS_VER%-%TARGET_ARCH%

IF NOT EXIST %BUILD_PATH% mkdir %BUILD_PATH%
cd %BUILD_PATH%

IF NOT EXIST tundra-urho3d.sln. (
    IF EXIST CMakeCache.txt. del /Q CMakeCache.txt
    cecho {0D}Running CMake for Tundra.{# #}{\n}
    IF "%2"=="" (
        REM No extra arguments provided, trust that GENERATOR is set properly.    
        cmake.exe .. -G %GENERATOR% -DURHO3D_HOME=%DEPS%\urho3d -DMATHGEOLIB_HOME=%DEPS%\MathGeoLib\build -DKNET_HOME=%DEPS%\kNet
    ) ELSE (
        REM Extra arguments has been provided. As CMake options are typically of format -DINSTALL_BINARIES_ONLY:BOOL=ON,
        REM i.e. they contain an equal sign, they will mess up the batch file argument parsing if the arguments are passed on
        REM by splitting them %2 %3 %4 %5 %6 %7 %8 %9. In the extra argument case trust that user has provided the generator
        REM as the first argument as pass all arguments as is by using %*.
        cmake.exe .. -G %*
    )
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
) ELSE (
    cecho {0A}tundra-urho3d.sln exists. Skipping CMake call for Tundra.{# #}{\n}
    cecho {0A}Delete %CD%\tundra-urho3d.sln to trigger a CMake rerun.{# #}{\n}
)
echo.

:: Finish in same directory we started in.
cd ..\tools\Windows
set PATH=%ORIGINAL_PATH%
GOTO :EOF

:ERROR
echo.
cecho {0C}An error occurred! Aborting!{# #}{\n}
:: Finish in same directory we started in.
cd ..\tools\Windows
set PATH=%ORIGINAL_PATH%

endlocal

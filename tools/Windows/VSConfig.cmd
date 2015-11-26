:: This script initializes various Visual Studio -related environment variables needed for building
:: NOTE: The delayed environment variable expansion needs to be enabled before calling this.

@echo off

set GENERATOR=%1

:: Supported Visual Studio versions:
set GENERATOR_VS2015="Visual Studio 14 2015"
set GENERATOR_VS2015_WIN64="Visual Studio 14 2015 Win64"
set GENERATOR_VS2013="Visual Studio 12"
set GENERATOR_VS2013_WIN64="Visual Studio 12 Win64"
set GENERATOR_VS2012="Visual Studio 11"
set GENERATOR_VS2012_WIN64="Visual Studio 11 Win64"
set GENERATOR_VS2010="Visual Studio 10"
set GENERATOR_VS2010_WIN64="Visual Studio 10 Win64"
set GENERATOR_DEFAULT=%GENERATOR_VS2010%

IF "!GENERATOR!"=="" (
    set GENERATOR=%GENERATOR_DEFAULT%
    Utils\cecho {0E}VSConfig.cmd: Warning: Generator not passed - using the default %GENERATOR_DEFAULT%.{# #}{\n}
)

:: TODO Some nicer check for this.
IF NOT !GENERATOR!==%GENERATOR_VS2010% IF NOT !GENERATOR!==%GENERATOR_VS2010_WIN64% IF NOT !GENERATOR!==%GENERATOR_VS2012% IF NOT !GENERATOR!==%GENERATOR_VS2012_WIN64% IF NOT !GENERATOR!==%GENERATOR_VS2013% IF NOT !GENERATOR!==%GENERATOR_VS2013_WIN64% (
IF NOT !GENERATOR!==%GENERATOR_VS2015% IF NOT !GENERATOR!==%GENERATOR_VS2015_WIN64%  (
    Utils\cecho {0C}VSConfig.cmd: Invalid or unsupported CMake generator string passed: !GENERATOR!. Cannot proceed, aborting!{# #}{\n}
    exit /b 1
))

:: Figure out the build configuration from the CMake generator string.
:: Are we building 32-bit or 64-bit version.
set TARGET_ARCH=x86
:: TODO This can be probably removed.
set INTEL_ARCH=ia32
:: Visual Studio platform name.
set VS_PLATFORM=Win32

:: Split the string for closer inspection.
:: VS_VER and VC_VER are convenience variables used f.ex. for filenames.
set GENERATOR_NO_DOUBLEQUOTES=%GENERATOR:"=%
set GENERATOR_SPLIT=%GENERATOR_NO_DOUBLEQUOTES: =,%
FOR %%i IN (%GENERATOR_SPLIT%) DO (
    IF %%i==14 (
        set VS_VER=vs2015
        set VC_VER=vc14
    )
    IF %%i==12 (
        set VS_VER=vs2013
        set VC_VER=vc12
    )
    IF %%i==11 (
        set VS_VER=vs2012
        set VC_VER=vc11
    )
    IF %%i==10 (
        set VS_VER=vs2010
        set VC_VER=vc10
    )
    REM Are going to perform a 64-bit build?
    IF %%i==Win64 (
        set TARGET_ARCH=x64
        set INTEL_ARCH=intel64
        set VS_PLATFORM=x64
    )
)

:: VS project file extension is vcxproj on VS 2010 and newer always.
set VCPROJ_FILE_EXT=vcxproj
:: Populate path variables
cd ..\..
set ORIGINAL_PATH=%PATH%
set PATH=%PATH%;"%CD%\tools\Windows\Utils"
:: TOOLS is the path where the Windows build scripts reside.
set TOOLS=%CD%\tools\Windows
set TUNDRA_DIR="%CD%"
set TUNDRA_BIN=%CD%\bin

:: Fetch and build the dependencies to a dedicated directory depending on the used VS version and target architecture.
set DEPS=%CD%\deps-%VS_VER%-%TARGET_ARCH%

cd %TOOLS%

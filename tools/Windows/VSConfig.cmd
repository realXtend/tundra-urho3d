:: This script initializes various Visual Studio -related environment variables needed for building
:: This batch file expects CMake generator as %1 and build configuration type as %2.
:: NOTE This batch file expects the generator string to be CMake 3.0.0 and newer format, i.e.
:: "Visual Studio 10 2010" instead of "Visual Studio 10". However, one can use this batch file
:: also with CMake 2 as the generator will be converted into the older format if necessary.

:: NOTE: The delayed environment variable expansion needs to be enabled before calling this.

@echo off

set GENERATOR=%1

:: Supported Visual Studio versions:
set GENERATORS[0]="Visual Studio 14 2015"
set GENERATORS[1]="Visual Studio 14 2015 Win64"
set GENERATORS[2]="Visual Studio 12 2013"
set GENERATORS[3]="Visual Studio 12 2013 Win64"
set GENERATORS[4]="Visual Studio 11 2012"
set GENERATORS[5]="Visual Studio 11 2012 Win64"
set GENERATORS[6]="Visual Studio 10 2010"
set GENERATORS[7]="Visual Studio 10 2010 Win64"
set NUM_GENERATORS=7
set GENERATOR_DEFAULT=%GENERATORS[6]%

IF "!GENERATOR!"=="" (
    set GENERATOR=%GENERATOR_DEFAULT%
    Utils\cecho {0E}VSConfig.cmd: Warning: Generator not passed - using the default %GENERATOR_DEFAULT%.{# #}{\n}
)

FOR /l %%i in (0,1,%NUM_GENERATORS%) DO (
    IF !GENERATOR!==!GENERATORS[%%i]! GOTO :GeneratorValid
)
Utils\cecho {0C}VSConfig.cmd: Invalid or unsupported CMake generator string passed: !GENERATOR!. Cannot proceed, aborting!{# #}{\n}
echo Supported CMake generator strings:
FOR /l %%i in (0,1,%NUM_GENERATORS%) DO (
    echo !GENERATORS[%%i]!
)
exit /b 1

:GeneratorValid
:: Figure out the build configuration from the CMake generator string.
:: Are we building 32-bit or 64-bit version.
set ARCH_BITS=32
set TARGET_ARCH=x86
:: TODO This can be probably removed.
set INTEL_ARCH=ia32
:: Visual Studio platform name.
set VS_PLATFORM=Win32

:: Split the string for closer inspection.
:: VS_VER and VC_VER are convenience variables used f.ex. for filenames.
:: TODO VS_VER and VC_VER might be nicer without vs and vc prefixes
set GENERATOR_NO_DOUBLEQUOTES=%GENERATOR:"=%
set GENERATOR_SPLIT=%GENERATOR_NO_DOUBLEQUOTES: =,%
FOR %%i IN (%GENERATOR_SPLIT%) DO (
    call :StrLength LEN %%i
    IF !LEN!==2 (
        set VC_VER_NUM=%%i
        set VC_VER=vc%%i
    )
    IF !LEN!==4 set VS_VER=vs%%i
    REM Are going to perform a 64-bit build?
    IF %%i==Win64 (
        set ARCH_BITS=64
        set TARGET_ARCH=x64
        set INTEL_ARCH=intel64
        set VS_PLATFORM=x64
    )
)

:: Check CMake version and convert possible new format (>= 3.0) generator names to the old versions if using older CMake for VS <= 2013,
:: see http://www.cmake.org/cmake/help/v3.0/release/3.0.0.html#other-changes
FOR /f "delims=" %%i in ('where cmake') DO set CMAKE_PATH=%%i
IF NOT "%CMAKE_PATH%"=="" (
    FOR /f "delims=" %%i in ('cmake --version ^| findstr /C:"cmake version 3"') DO GOTO :CMake3AndNewer
)
:: CMake older than 3.0.0: convert new format generators to the old format (simple brute force for simplicity)
set GENERATOR=%GENERATOR: 2013=%
set GENERATOR=%GENERATOR: 2012=%
set GENERATOR=%GENERATOR: 2010=%
:CMake3AndNewer

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
GOTO :EOF
:: http://geekswithblogs.net/SoftwareDoneRight/archive/2010/01/30/useful-dos-batch-functions-substring-and-length.aspx
:StrLength
set #=%2%
set length=0
:stringLengthLoop
if defined # (set #=%#:~1%&set /A length += 1&goto stringLengthLoop)
::echo the string is %length% characters long!
set "%~1=%length%"
GOTO :EOF

@echo off
echo.

:: Enable the delayed environment variable expansion needed in VSConfig.cmd.
setlocal EnableDelayedExpansion

:: NUMBER_OF_PROCESSORS usage can be overridden with TUNDRA_DEPS_CPUS env variable.
IF "%TUNDRA_DEPS_CPUS%"=="" (
    set TUNDRA_DEPS_CPUS=%NUMBER_OF_PROCESSORS%
)
IF "%TUNDRA_ANDROID%"=="" (
    set TUNDRA_ANDROID=0
)
IF "%TUNDRA_INIT_MSVC_ENV%"=="" (
    set TUNDRA_INIT_MSVC_ENV=TRUE
)

:: Android cross compilation can jump over the Visual Studio env init
if %TUNDRA_INIT_MSVC_ENV%==FALSE GOTO :SKIP_MSVC_ENV_INIT

:: Make sure we're running in Visual Studio Command Prompt
IF "%VSINSTALLDIR%"=="" (
   %TOOLS%\Utils\cecho {0C}Batch file not executed from Visual Studio Command Prompt - cannot proceed!{# #}{\n}
   GOTO :ERROR
)

:: Set up variables depending on the used Visual Studio version
call VSConfig.cmd %1
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

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
    cecho {0E}VSConfig.cmd: Warning: BUILD_TYPE not specified - using the default %BUILD_TYPE_DEFAULT%{# #}{\n}
    pause
)
IF NOT !BUILD_TYPE!==%BUILD_TYPE_MINSIZEREL% IF NOT !BUILD_TYPE!==%BUILD_TYPE_RELEASE% IF NOT !BUILD_TYPE!==%BUILD_TYPE_RELWITHDEBINFO% IF NOT !BUILD_TYPE!==%BUILD_TYPE_DEBUG% (
    cecho {0C}VSConfig.cmd: Invalid or unsupported CMake build type passed: !BUILD_TYPE!. Cannot proceed, aborting!{# #}{\n}
    GOTO :ERROR
)

:: DEBUG_OR_RELEASE and DEBUG_OR_RELEASE_LOWERCASE are "Debug" and "debug" for Debug build and "Release" and "release"
:: for all of the Release variants. 
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
echo    1. Install Git and SVN and make sure 'git' and 'svn' are accessible from PATH.
:: TODO Print the following only if using VS 2010/2012 and/or Express editions?
echo    2. Install DirectX SDK June 2010.
echo     - http://www.microsoft.com/download/en/details.aspx?id=6812
echo    3. Install CMake and make sure 'cmake' is accessible from PATH.
echo     - http://www.cmake.org/
echo    4. Install Perl.
echo     - https://www.perl.org/get.html#win32
echo    5. Install Visual Studio 2010 or newer with latest updates (Express is ok, but see section 5 for 2010).
echo     - http://www.microsoft.com/visualstudio/eng/downloads
:: TODO Print these only if using VS 2010
cecho {0E}   6. Optional: Make sure you have the Visual Studio x64 tools installed{# #}{\n}
cecho {0E}      before installing the Visual Studio 2010 Service Pack 1, {# #}{\n}
cecho {0E}      http://www.microsoft.com/en-us/download/details.aspx?id=23691 {# #}{\n}
cecho {0E}      if wanting to build Tundra as a 64-bit application.{# #}{\n}
:: TODO Print the following only if using VS 2010/2012 and/or Express editions?
echo    7. Install Windows SDK.
echo     - http://www.microsoft.com/download/en/details.aspx?id=8279
echo    8. Execute this file with Visual Studio environment variables set.
echo.    - https://msdn.microsoft.com/en-us/library/ms229859(v=vs.110).aspx
echo.
echo If you are not ready with the above, press Ctrl-C to abort!
pause
echo.

:: Check that required tools are in PATH
FOR %%i IN (cmake git svn perl) DO (
    where %%i 1> NUL 2> NUL || cecho {0C}Required tool '%%i' not installed or not added to PATH!{# #}{\n} && goto :ERROR
)

:SKIP_MSVC_ENV_INIT

:: Make sure deps folder exists.
IF NOT EXIST "%DEPS%". mkdir "%DEPS%"




:::::::::::::::::::::::: MathGeoLib

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

:: pre build

cecho {0D}Running CMake for MathGeoLib.{# #}{\n}
cmake . -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
        -DANDROID=%TUNDRA_ANDROID% ^
        -DCMAKE_DEBUG_POSTFIX=_d  ^
        -DCMAKE_INSTALL_PREFIX=%DEPS%\MathGeoLib\build
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build

cecho {0D}Building %BUILD_TYPE% MathGeoLib. Please be patient, this will take a while.{# #}{\n}
IF %TUNDRA_ANDROID%==0 (
    MSBuild MathGeoLib.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    MSBuild INSTALL.%VCPROJ_FILE_EXT% /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
    make install
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: kNet

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

:: pre build

cecho {0D}Running CMake for kNet.{# #}{\n}
cmake . -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
        -DANDROID=%TUNDRA_ANDROID% ^
        -DCMAKE_DEBUG_POSTFIX=_d
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build

cecho {0D}Building %BUILD_TYPE% kNet. Please be patient, this will take a while.{# #}{\n}
IF %TUNDRA_ANDROID%==0 (
    MSBuild kNet.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: Urho3D engine

IF NOT EXIST "%DEPS%\urho3d\". (
    cecho {0D}Cloning Urho3D into "%DEPS%\urho3d".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/urho3d/Urho3D.git urho3d
    cd "%DEPS%\urho3d\"
    IF NOT EXIST "%DEPS%\urho3d\.git" GOTO :ERROR
    git checkout 5acb708
) ELSE (
    cd "%DEPS%\urho3d\"
)

:: pre build

cecho {0D}Running CMake for Urho3D.{# #}{\n}
IF NOT EXIST "Build" mkdir "Build"
cd Build
IF %TARGET_ARCH%==x64 (
    set URHO3D_64BIT=1
) ELSE (
    set URHO3D_64BIT=0
)

set URHO_CMAKE_EXTRAS=
set URHO_CMAKE_RELATIVE_DIR=..
IF %TUNDRA_ANDROID%==1 (
    cd ..\Android
    set URHO_CMAKE_RELATIVE_DIR=..
    set URHO_CMAKE_EXTRAS=-DLIBRARY_OUTPUT_PATH_ROOT=.
)

cmake %URHO_CMAKE_RELATIVE_DIR% ^
    -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
    -DANDROID=%TUNDRA_ANDROID% ^
    -DURHO3D_LIB_TYPE=SHARED ^
    -DURHO3D_64BIT=%URHO3D_64BIT% ^
    -DURHO3D_ANGELSCRIPT=0 ^
    -DURHO3D_LUA=0 ^
    -DURHO3D_TOOLS=0 ^
    -DURHO3D_PHYSICS=0 ^
    -DURHO3D_NETWORK=0 ^
    -DURHO3D_NAVIGATION=0 ^
    -DURHO3D_SAMPLES=0 ^
    -DURHO3D_URHO2D=0 %URHO_CMAKE_EXTRAS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build

cecho {0D}Building %BUILD_TYPE% Urho3D. Please be patient, this will take a while.{# #}{\n}
IF %TUNDRA_ANDROID%==0 (
    MSBuild Urho3D.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: deploy

IF %TUNDRA_ANDROID%==0 (
    cecho {0D}Deploying Urho3D DLL to Tundra bin\ directory.{# #}{\n}
    copy /Y "%DEPS%\urho3D\Build\Bin\*.dll" "%TUNDRA_BIN%"
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: gtest
:: Uncomment below goto if google svn is not responding
:: GOTO :SKIP_GTEST

IF NOT EXIST "%DEPS%\gtest\". (
    cecho {0D}Cloning Google C++ Testing Framework into "%DEPS%\gtest".{# #}{\n}
    cd "%DEPS%"
    svn checkout http://googletest.googlecode.com/svn/tags/release-1.7.0/ gtest
    IF NOT EXIST "%DEPS%\gtest\.svn" GOTO :ERROR
)

cd "%DEPS%\gtest"
IF NOT EXIST "build" mkdir "build"
cd build

IF %TUNDRA_ANDROID%==0 (
    cecho {0D}Running CMake for Google C++ Testing Framework.{# #}{\n}
    cmake ../ -G %GENERATOR% -Dgtest_force_shared_crt=TRUE
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR

    cecho {0D}Building %BUILD_TYPE% Google C++ Testing Framework. Please be patient, this will take a while.{# #}{\n}
    MSBuild gtest.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    cmake ../ -G %GENERATOR% %TUNDRA_TOOLCHAIN%
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR

    make -j%TUNDRA_DEPS_CPUS%
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:SKIP_GTEST




:::::::::::::::::::::::: openssl
:: For now only build openssl once in release mode

IF %TUNDRA_ANDROID%==0 (
    IF NOT EXIST "%DEPS%\openssl\". (
        cecho {0D}Cloning OpenSSL into "%DEPS%\openssl".{# #}{\n}
        cd "%DEPS%"
        git clone https://github.com/openssl/openssl.git openssl
        IF NOT EXIST "%DEPS%\openssl\.git" GOTO :ERROR
        cd openssl
        REM 1.0.2d required for VS 2015 suppport. 1.0.1 series fail to build.
        git checkout OpenSSL_1_0_2d
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    )
    IF NOT EXIST "%DEPS%\openssl\build". (
        cd "%DEPS%\openssl"
        IF NOT EXIST "build" mkdir "build"

        cecho {0D}Running pre build for OpenSSL.{# #}{\n}
        IF %TARGET_ARCH%==x64 (
            perl Configure VC-WIN64A --prefix="%DEPS%\openssl\build"
            call ms\do_win64a.bat
        ) ELSE (
            IF NOT EXIST "%TOOLS%\Utils\nasm-2.11.06". (
                curl -o "%TOOLS%\Utils\nasm.zip" http://www.nasm.us/pub/nasm/releasebuilds/2.11.06/win32/nasm-2.11.06-win32.zip
                7za x -y -o"%TOOLS%\Utils" "%TOOLS%\Utils\nasm.zip"
                del /Q "%TOOLS%\Utils\nasm.zip"
            )
            set PATH=!PATH!;%TOOLS%\Utils\nasm-2.11.06
            perl Configure VC-WIN32 --prefix="%DEPS%\openssl\build"
            call ms\do_nasm.bat
        )
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR
        cecho {0D}Building OpenSSL.{# #}{\n}
        nmake -f ms\nt.mak
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR
        nmake -f ms\nt.mak install
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    )
) ELSE (
    IF NOT EXIST "%DEPS%\openssl\". (
        cd "%DEPS%"
        %TOOLS%\Utils\curl -o openssl-cmake-1.0.1e-src.tar.gz -k -L https://launchpad.net/openssl-cmake/1.0.1e/1.0.1e-1/+download/openssl-cmake-1.0.1e-src.tar.gz
        7za e -y openssl-cmake-1.0.1e-src.tar.gz
        7za x -y openssl-cmake-1.0.1e-src.tar
        del /Q openssl-cmake-1.0.1e-src.tar.gz
        del /Q openssl-cmake-1.0.1e-src.tar
        ren openssl-cmake-1.0.1e-src openssl
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    )
    IF NOT EXIST "%DEPS%\openssl\build". (
        cd "%DEPS%/openssl"
        cmake . -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
            -DCMAKE_INSTALL_PREFIX=./build
        make -j%TUNDRA_DEPS_CPUS%
        make install
    )
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: curl

IF NOT EXIST "%DEPS%\curl\". (
    cecho {0D}Cloning Curl into "%DEPS%\curl".{# #}{\n}
    cd "%DEPS%"
    git clone --branch curl-7_38_0 https://github.com/bagder/curl.git curl
    IF NOT EXIST "%DEPS%\curl\.git" GOTO :ERROR
    cd curl
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    IF %TUNDRA_ANDROID%==1 (
        patch --strip=1 < %TOOLS%\Patches\android-curl-0001-hack-find-openssl.patch
    )
)

cd "%DEPS%\curl"
IF NOT EXIST "build-src" mkdir "build-src"
cd build-src

:: pre build

set CURL_EXTRAS=-DOPENSSL_ROOT_DIR="%DEPS%\openssl\build"
IF %TUNDRA_ANDROID%==1 (
    :: Android build hacks around FindOpenSSL.cmake with above android-curl-0001-hack-find-openssl.patch
    :: and defines all the needed values via cmd line.
    set CURL_EXTRAS=-DOPENSSL_INCLUDE_DIR="%DEPS%\openssl\build\include" -DOPENSSL_LIBRARIES="%DEPS%\openssl\build\lib\libssl.a;%DEPS%\openssl\build\lib\libcrypto.a"
)
echo '%CURL_EXTRAS%'

cecho {0D}Running CMake for Curl.{# #}{\n}
cmake ../ -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
    -DCMAKE_DEBUG_POSTFIX=_d ^
    -DCMAKE_INSTALL_PREFIX="%DEPS%\curl\build" ^
    -DBUILD_CURL_EXE=OFF -DBUILD_CURL_TESTS=OFF -DCURL_STATICLIB=ON ^
    %CURL_EXTRAS%

:: With Android cross compilation the first cmake will fail.
:: Run cmake again with above cache and a file that manually
:: defines results for failed compiler/platform feature checks.
if %TUNDRA_ANDROID%==1 (
    cmake -C "%TOOLS%\Mods\android-curl-try-run-results.cmake" ../
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build
IF %TUNDRA_ANDROID%==0 (
    cecho {0D}Building %BUILD_TYPE% Curl. Please be patient, this will take a while.{# #}{\n}
    MSBuild CURL.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    MSBUILD INSTALL.vcxproj /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
    make install
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR
:: For Tundra cmake convenience copy openssl static libs to curl lib dir
copy /Y "%DEPS%\openssl\build\lib\*" "%DEPS%\curl\build\lib"
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: zlib
:: Android OS from API >=3 ships zlib in the system

IF %TUNDRA_ANDROID%==1 GOTO :SKIP_ZLIB

IF NOT EXIST "%DEPS%\zlib\". (
    cecho {0D}Cloning zlib into "%DEPS%\zlib".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/madler/zlib.git zlib
    IF NOT EXIST "%DEPS%\zlib\.git" GOTO :ERROR
    cd zlib
    git checkout v1.2.8
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
)

cd "%DEPS%\zlib"
IF NOT EXIST "build-src" mkdir "build-src"
cd build-src

:: pre build

cecho {0D}Running CMake for zlib.{# #}{\n}
cmake ../ -G %GENERATOR% ^
    -DCMAKE_INSTALL_PREFIX="%DEPS%\zlib\build" ^
    -DCMAKE_DEBUG_POSTFIX=_d
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build

cecho {0D}Building %BUILD_TYPE% zlib. Please be patient, this will take a while.{# #}{\n}
MSBuild zlib.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
IF NOT %ERRORLEVEL%==0 GOTO :ERROR
MSBUILD INSTALL.vcxproj /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:SKIP_ZLIB




:::::::::::::::::::::::: zziplib

IF NOT EXIST "%DEPS%\zziplib\". (
    cecho {0D}Cloning zziplib into "%DEPS%\zziplib".{# #}{\n}
    cd "%DEPS%"
    svn checkout svn://svn.code.sf.net/p/zziplib/svn/tags/V_0_13_62  zziplib
    IF NOT EXIST "%DEPS%\zziplib\.svn" GOTO :ERROR
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
    cd "%DEPS%\zziplib"
    svn patch %TOOLS%\Patches\zziplib-0001-add-cmake.patch

    :: in zziplib zzip/_config.h is generated by running ./configure. It is non trivial
    :: to automate for the Android cross compilation. This file has been
    :: generated by running configure against Android API 12 sysroot and compiler.
    :: The file is mostly about defining size of 'long long' etc. so I'm assuming
    :: this will do a fine job for out use cases and target devices. 64bit android
    :: builds might change this in the future, to be revisited then.
    IF %TUNDRA_ANDROID%==1 (
        copy /Y "%TOOLS%\Mods\android-zziplib-config.h" zzip\_config.h
    )
    IF NOT %ERRORLEVEL%==0 GOTO :ERROR
)

cd "%DEPS%\zziplib"
IF NOT EXIST "build" mkdir "build"
cd build

:: pre build

cmake ../ -G %GENERATOR% %TUNDRA_TOOLCHAIN% ^
    -DCMAKE_DEBUG_POSTFIX=_d ^
    -DZLIB_ROOT="%DEPS%\zlib\build"
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

:: build

IF %TUNDRA_ANDROID%==0 (
    cecho {0D}Building %BUILD_TYPE% zziplib. Please be patient, this will take a while.{# #}{\n}
    MSBuild zziplib.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




:::::::::::::::::::::::: Bullet physics engine
IF NOT EXIST "%DEPS%\bullet\". (
    cecho {0D}Cloning Bullet into "%DEPS%\bullet".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/bulletphysics/bullet3 bullet
    cd "%DEPS%/bullet"
    git checkout 2.83.6
    IF NOT EXIST "%DEPS%\bullet\.git" GOTO :ERROR
)

cd "%DEPS%\bullet\"
cecho {0D}Running CMake for Bullet.{# #}{\n}
cmake . -G %GENERATOR% -DBUILD_EXTRAS:BOOL=OFF -DBUILD_UNIT_TESTS:BOOL=OFF ^
    -DBUILD_BULLET3:BOOL=OFF -DBUILD_BULLET2_DEMOS:BOOL=OFF ^
    -DCMAKE_DEBUG_POSTFIX=_d -DUSE_MSVC_RUNTIME_LIBRARY_DLL:BOOL=ON ^
    -DCMAKE_MINSIZEREL_POSTFIX= -DCMAKE_RELWITHDEBINFO_POSTFIX=
IF NOT %ERRORLEVEL%==0 GOTO :ERROR

cecho {0D}Building %BUILD_TYPE% Bullet. Please be patient, this will take a while.{# #}{\n}
IF %TUNDRA_ANDROID%==0 (
    MSBuild BULLET_PHYSICS.sln /p:configuration=%BUILD_TYPE% /clp:ErrorsOnly /nologo /m:%TUNDRA_DEPS_CPUS%
) ELSE (
    make -j%TUNDRA_DEPS_CPUS%
)
IF NOT %ERRORLEVEL%==0 GOTO :ERROR




::::::::::::::::::::::::: Boost (for websocketpp)
set BOOST_VERSION="1.54.0"
:: Version string with underscores instead of dots.
set BOOST_VER=%BOOST_VERSION:.=_%
set BOOST_ROOT=%DEPS%\boost
set BOOST_INCLUDEDIR=%DEPS%\boost
set BOOST_LIBRARYDIR=%DEPS%\boost\stage\lib

IF %TUNDRA_ANDROID%==0 (
    IF NOT EXIST "%DEPS%\boost". (
        cecho {0D}%BOOST_ROOT%{# #}{\n}
        cecho {0D}Downloading and extracting Boost %BOOST_VERSION% into "%DEPS%\boost".{# #}{\n}
        cd "%DEPS%"
        IF NOT EXIST boost_%BOOST_VER%.zip. (
            wget http://downloads.sourceforge.net/project/boost/boost/%BOOST_VERSION%/boost_%BOOST_VER%.zip
            IF NOT EXIST boost_%BOOST_VER%.zip. GOTO :ERROR
        )
        7za x boost_%BOOST_VER%.zip
        ren boost_%BOOST_VER% boost
        IF NOT EXIST "%DEPS%\boost\boost.css" GOTO :ERROR
    
        cd "%DEPS%\boost"
        cecho {0D}Building Boost build script.{# #}{\n}
        call bootstrap
        IF NOT %ERRORLEVEL%==0 GOTO :ERROR

        cd "%DEPS%\boost"
        cecho {0D}Building Boost. Please be patient, this will take a while.{# #}{\n}
        REM NOTE The ".0" postfix below doesn't necessarily work for all future VS versions.
        call .\b2 --toolset=msvc-%VC_VER_NUM%.0 address-model=%ARCH_BITS% -j %NUMBER_OF_PROCESSORS% --with-system stage
    ) ELSE (
        ::TODO Even if %DEPS%\boost exists, we have no guarantee that boost is built successfully for real
        cecho {0D}Boost already built. Skipping.{# #}{\n}
    )
)




:::::::::::::::::::::::: websocketpp

IF NOT EXIST "%DEPS%\websocketpp\". (
    cecho {0D}Cloning websocketpp library from https://https://github.com/realXtend/websocketpp.git into "%DEPS%\websocketpp".{# #}{\n}
    cd "%DEPS%"
    git clone https://github.com/realXtend/websocketpp.git websocketpp
)




:::::::::::::::::::::::: All done

echo.
cecho {0A}Tundra dependencies built.{# #}{\n}
GOTO :END

:::::::::::::::::::::::: Error exit handler

:ERROR

echo.
%~dp0\Utils\cecho {0C}An error occurred! Aborting!{# #}{\n}
pause
:END
set PATH=%ORIGINAL_PATH%
cd %TOOLS%

endlocal

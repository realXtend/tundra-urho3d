[![Build Status](https://travis-ci.org/realXtend/tundra-urho3d.svg?branch=master)](https://travis-ci.org/realXtend/tundra-urho3d)

Tundra-Urho3D
=============

Lightweight reimplementation of the realXtend Tundra core functionality primarily aimed at mobile platforms.
The [Urho3D] engine is used for the platform abstraction and rendering.

Tundra-Urho3D is licensed under [Apache 2.0].

Compiling from Sources
----------------------

Tundra-Urho3D uses [CMake] as its build system.

Partial support for C++11 is required to compile successfully: in particular support for the `auto`, `override`,
and `nullptr` keywords. Also the C99 `<stdint.h>`/`<cstdint>` header is required. More C++11 features may be 
considered later if found necessary. Currently minimum compiler requirements are VS 2010 or newer (VS 2008 not 
supported) or GCC 4.7 or newer. All Clang versions should support the needed feature set.

### Windows, targeting Windows

Make sure that you have the latest Visual Studio Service Packs or Updates installed. Visual Studio 2012 should work,
but is not tested regularly and no batch scripts are provided currently.

1. Open up the Visual Studio (x64 Win64) Command Prompt which is required in order to have the required build tools
   and several other utilities in your PATH.
2. Navigate to `<Tundra-Urho3D>\tools\Windows\VS<VersionNumber>\`
3. Run `BuildDeps-<BuildType>.cmd`, or `BuildDeps-x64-<BuildType>.cmd` (if wanting to do a 64-bit build).
   `RelWithDebInfo` is recommended for the common development work, but you probably want to have the `Debug`
   build available too.
   The build script will print information what you need in order to proceed, follow the instructions carefully.
   You can abort the script with Ctrl+C at this point and setup your environment.
4. After the script has completed, the dependencies can be found `deps-vs<VersionNumber>-<TargetArchitecture>\`.
   The needed runtime libraries are automatically copied to `bin\`.
   **Note:** If building with the Windows 8 (or newer) SDK, `d3dcompiler_46.dll` needs to be available in `bin\`
   so that the executables will run correctly. `BuildDeps.cmd` should copy the file (from `C:\Program Files (x86)\Windows Kits\8.0\Redist\D3D\<x86|x64>`)
   automatically, but if it fails, you must to it manually.
5. Now run CMake batch script corresponding to your desired build configuration. This script will set up the needed
   build environment variables for CMake and invoke CMake to generate a tundra-urho3d.sln solution into the
   `build-vs<VersionNumber>-<TargetArchitecture>` subdirectory.
6. Build Tundra-Urho3D using the solution file that can be found from `build-vs<VersionNumber>-<TargetArchitecture>\`,
or use the batch files in `tools\Windows`.


### Windows, targeting Android

This is a quick summary. For more details refer to the [wiki page](https://github.com/realXtend/tundra-urho3d/wiki/Android:-Cross-compiling-with-Windows).

1. Navigate to `<Tundra-Urho3D>\tools\Windows\`
2. Run `BuildAndroidDeps.cmd`. The build script will print information what you need in order to proceed, follow the
   instructions carefully. You can abort the script with Ctrl+C at this point and setup your environment.
3. After the script has completed, the Android dependencies have been produced in `deps-android`.
4. Run `cmake-android.bat` from the checkout root directory to prepare the CMake build of Tundra-Urho3D for Android.
5. Navigate to `<Tundra-Urho3D>\src\Android\` and execute `make`.
6. Copy assets from the Tundra bin directory by executing `CopyData.bat` (todo: automatize/remove this step)
7. Create the ant build files: `android update project -p . -t <targetAPINumber>`. Only needed when building the first
   time.
   (Use `android list targets` to list your installed Android target API numbers).
8. Build the apk: `ant debug` (use `ant release` and sign your apk when creating an actual release build).
9. The apk is written to `<Tundra-Urho3D>\src\Android\bin` directory where it can be uploaded to a device or run
    on an emulator.

[Apache 2.0]: http://www.apache.org/licenses/LICENSE-2.0.txt "Apache 2.0 license"
[Urho3D]: http://urho3d.github.io "Urho3D homepage"
[CMake]: http://www.cmake.org/ "CMake homepage"

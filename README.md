Tundra-urho3d
=============

Lightweight reimplementation of the realXtend Tundra core functionality primarily aimed at mobile platforms. The [Urho3D] engine is used for the platform abstraction and rendering.

Tundra-urho3d is licensed under [Apache 2.0].

Compiling from Sources
----------------------

Tundra-urho3d uses [CMake] as its build system.

### Windows, targeting Windows

1. Open up the Visual Studio (x64 Win64) Command Prompt which is required in order to have the required build tools and several other utilities in your PATH.
2. Navigate to `<Tundra-urho3d>\tools\Windows\VS<VersionNumber>\`
3. Run `BuildDeps_<BuildType>`, or `BuildDepsX64_<BuildType>` (if wanting to do a 64-bit build). RelWithDebInfo is recommended for the common development work, but you probably want to have the Debug builds available too.
   The build script will print information what you need in order to proceed, follow the instructions carefully. You can abort the script with Ctrl+C at this point and setup your environment.
4. After the script has completed, the dependencies can be found `deps-vs<VersionNumber>-<TargetArchitecture>\`. The needed runtime libraries are automatically copied to `bin\`.
5. Now run CMake batch script corresponding to your desired build configration. This script will set up the needed build environment variables for CMake and invoke CMake to generate a tundra-urho3d.sln solution into the <Tundra-urho3d>\build subdirectory.
6. Build Tundra-urho3d using the solution file.

### Windows, targeting Android

1. Navigate to `<Tundra-urho3d>\tools\Windows\`
2. Run BuildDeps_Android.bat
   The build script will print information what you need in order to proceed, follow the instructions carefully. You can abort the script with Ctrl+C at this point and setup your environment.
3. After the script has completed, the Android dependencies have been produced in `deps-android`.
4. Run `CMake_Android.bat` from the checkout root directory to prepare the CMake build of Tundra-urho3d for Android.
5. Navigate to `<Tundra-urho3d>\src\Android\` and execute
   make
6. Build the apk. The 'android update project' line is only needed when building for the first time.
   android update project -p . -t <targetAPINumber>. (Use 'android list targets' to list your installed Android target API numbers for the -t parameter)
   ant debug (use ant release and sign your APK when building an actual release build)
6. The apk is written to `<Tundra-urho3d>\src\Android\bin` directory where it can be uploaded to a device or run on an emulator.

[Apache 2.0]: http://www.apache.org/licenses/LICENSE-2.0.txt "Apache 2.0 license"
[Urho3D]: http://urho3d.github.io "Urho3D homepage"
[CMake]: http://www.cmake.org/ "CMake homepage"

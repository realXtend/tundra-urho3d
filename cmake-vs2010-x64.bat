@echo off
IF EXIST build-vs2010-x64\tundra-urho3d.sln del /Q build-vs2010-x64\tundra-urho3d.sln
pushd tools\Windows\
call RunCMake "Visual Studio 10 2010 Win64" %*
popd
pause

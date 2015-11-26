@echo off
IF EXIST build-vs2015-x86\tundra-urho3d.sln del /Q build-vs2015-x86\tundra-urho3d.sln
pushd tools\Windows\
call RunCMake "Visual Studio 14 2015" %*
popd
pause

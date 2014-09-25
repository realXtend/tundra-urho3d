@echo off
IF EXIST build-vs2013-x64\tundra-urho3d.sln del /Q build-vs2013-x64\tundra-urho3d.sln
cd tools\Windows\
call RunCMake "Visual Studio 12 Win64"
cd ..\..
pause

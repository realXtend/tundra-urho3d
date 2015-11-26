@echo off
IF EXIST build-vs2015-x64\tundra-urho3d.sln del /Q build-vs2015-x64\tundra-urho3d.sln
cd tools\Windows\
call RunCMake "Visual Studio 14 2015 Win64" %*
cd ..\..
pause

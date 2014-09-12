@echo off
IF EXIST build\tundra-urho3d.sln del /Q build\tundra-urho3d.sln
cd tools\Windows\
call RunCMake "Visual Studio 10"
cd ..\..
pause

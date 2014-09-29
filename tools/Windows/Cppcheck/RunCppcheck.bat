:: Assumes that Cppcheck is installed in the default location or accessible via PATH by default.
:: The following warnings are suppressed:
:: - unusedFunction: suppressed because as an SDK, not all of the functions are not used by the library itself.
:: In Suppressions.txt:
:: - uninitMemberVar:TundraCore\JSON\JSON.cpp: 'data' member variable is left uninitialized intentionally.

:: TODO missingInclude warning suppressed for now until deps directory is configured to be included.

@echo off

set ORIG_PATH=%PATH%
set PATH=%PATH%;C:\Program Files (x86)\Cppcheck

call cppcheck --version
IF ERRORLEVEL 1 GOTO NoCppcheck

set SRC_DIR=../../../src/
call cppcheck --template "{file}({line}): ({severity}) ({id}): {message}" -DTUNDRACORE_API= -I%SRC_DIR% ^
    -rp=%SRC_DIR% --enable=all --suppress=unusedFunction --suppress=missingInclude --suppressions-list=Suppressions.txt ^
    %SRC_DIR%

GOTO End

:NoCppcheck
echo Could not find the cppcheck executable! Please add it to system PATH and try again!

:End
set PATH=%ORIG_PATH%

pause

@echo off
rem Runs all tests of one build: the NUnit test assemblies and the native Google Test binary.
rem The build of that configuration and platform must exist; this script builds nothing.
rem Usage: RunTests.cmd <Debug|Release> <x64|x86>
rem Exit code: 0 = all tests of both frameworks passed, 1 = a test failed, 2 = wrong call or missing build.
setlocal

set "CONFIGURATION=%~1"
set "PLATFORM=%~2"
if "%CONFIGURATION%"=="" goto :usage
if "%PLATFORM%"=="" goto :usage

set "BUILD_DIR=%~dp0build\%CONFIGURATION%\%PLATFORM%"
set "NATIVE_TESTS=%BUILD_DIR%\MCP.Debugger.NativeTests\MCP.Debugger.NativeTests.exe"
if not exist "%NATIVE_TESTS%" (
  echo No build found in "%BUILD_DIR%". Build the solution for %CONFIGURATION% ^| %PLATFORM% first.
  exit /b 2
)

set "RESULT=0"

rem Both parts always run, so one failing framework does not hide the result of the other.
rem Every exit code other than 0 counts as a failure: a test process that crashes ends with a
rem negative code, which "if errorlevel 1" would take for success.
dotnet test "%BUILD_DIR%\MCP.*Tests\MCP.*Tests.dll"
if not "%ERRORLEVEL%"=="0" set "RESULT=1"

"%NATIVE_TESTS%"
if not "%ERRORLEVEL%"=="0" set "RESULT=1"

exit /b %RESULT%

:usage
echo Usage: RunTests.cmd ^<Debug^|Release^> ^<x64^|x86^>
exit /b 2

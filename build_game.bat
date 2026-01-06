@echo off
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
set "VC_VARS=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"
if exist "%VC_VARS%" (
    call "%VC_VARS%" x86 -vcvars_ver=14.44.35207
) else (
    echo Error: vcvarsall.bat not found.
    exit /b 1
)

set "MSVC_VER=14.44.35207"
set "ATL_ROOT=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\atlmfc"
set "INCLUDE=%ATL_ROOT%\include;%INCLUDE%"
set "LIB=%ATL_ROOT%\lib\x86;%LIB%"

echo Starting Targeted Build for z_generals...
cmake --build --preset win32 --target z_generals
if %errorlevel% neq 0 (
    echo Build Failed!
    exit /b %errorlevel%
)

echo Build Successful.
copy "build\win32\GeneralsMD\Release\generalszh.exe" "Output\ZeroHour\generalszh.exe"

@echo off
set "VS_PATH=C:\Program Files\Microsoft Visual Studio\2022\Enterprise"
set "VC_VARS=%VS_PATH%\VC\Auxiliary\Build\vcvarsall.bat"

if "%1"=="clean" (
    echo Cleaning build directory...
    if exist "build" rmdir /s /q build
)

:: 1. Skip Environment Setup if already loaded
if defined VSCMD_VER (
    echo Visual Studio environment already loaded. Skipping vcvarsall.bat...
) else (
    if not exist "%VC_VARS%" (
        echo Error: Could not find vcvarsall.bat
        exit /b 1
    )
    call "%VC_VARS%" x86 -vcvars_ver=14.44.35207
)

set "MSVC_VER=14.44.35207"
set "ATL_ROOT=%VS_PATH%\VC\Tools\MSVC\%MSVC_VER%\atlmfc"
set "INCLUDE=%ATL_ROOT%\include;%INCLUDE%"
set "LIB=%ATL_ROOT%\lib\x86;%LIB%"

echo Using ATL from: %ATL_ROOT%

echo Starting Build...

:: 2. Incremental Build Optimization
:: If build.ninja exists, we can try to build directly without re-configuring.
set BUILD_CMD=cmake --workflow --preset win32

if exist "build\win32\build.ninja" (
    echo Found existing Ninja build file. Attempting incremental build...
    cmake --build --preset win32
    if not errorlevel 1 goto :BuildSuccess
    echo Incremental build failed. Falling back to full configure and build...
)

:: Full configure and build (fallback or fresh start)
%BUILD_CMD%
if %errorlevel% neq 0 (
    echo Build failed.
    exit /b %errorlevel%
)

:BuildSuccess
echo Build Successful. Copying binaries to Output...

if not exist "Output\Generals" mkdir "Output\Generals"
if not exist "Output\ZeroHour" mkdir "Output\ZeroHour"

echo Copying Generals (Base Game) files...
:: 3. Fast File Copy with Robocopy
:: Exit code < 8 means success (files copied or skipped)
robocopy "build\win32\Generals\Release" "Output\Generals" *.exe *.dll *.pdb /XO /NJH /NJS
if %errorlevel% geq 8 ( echo Robocopy failed for Generals & exit /b 1 )

echo Copying Zero Hour (GeneralsMD) files...
robocopy "build\win32\GeneralsMD\Release" "Output\ZeroHour" *.exe *.dll *.pdb /XO /NJH /NJS
if %errorlevel% geq 8 ( echo Robocopy failed for GeneralsMD & exit /b 1 )

echo ========================================================
echo All binaries copied to:
echo    %CD%\Output\Generals
echo    %CD%\Output\ZeroHour
echo ========================================================

set "GAME_DIR=d:\games\Command and Conquer - Generals\Command and Conquer Generals Zero Hour"
if exist "%GAME_DIR%" (
    echo Deploying Zero Hour binaries to Game Directory...
    robocopy "Output\ZeroHour" "%GAME_DIR%" *.exe *.dll *.pdb /XO /NJH /NJS
    if %errorlevel% geq 8 ( echo Deployment failed & exit /b 1 )
    echo Deployment Complete.
) else (
    echo Warning: Game directory not found at "%GAME_DIR%"
)

:: --------------------------------------------------------
:: 4. LOG CLEANUP AND DEV COPY (Yangi qo'shilgan qism)
:: --------------------------------------------------------

echo.
echo ========================================================
echo Cleaning debug log files (emptying content)...
:: Fayllarni ichini tozalash (0 bayt qilish)
type nul > "d:\dj_buildlist.txt"
type nul > "d:\djcc.txt"
type nul > "d:\djcc_ai.txt"
type nul > "d:\djcc_events.txt"
type nul > "d:\djcc_combat.txt"
echo Logs cleared.

echo.
echo Copying binaries to Original Code Directory...
set "DEV_DEST=d:\dj\genenerals c&c\org_code\GeneralsGameCode\Output\ZeroHour"

:: Agar papka yo'q bo'lsa yaratish (xavfsizlik uchun)
if not exist "%DEV_DEST%" mkdir "%DEV_DEST%"

:: Fayllarni ko'chirib o'tkazish va almashtirish (/Y so'ramasdan almashtiradi)
copy /Y "Output\ZeroHour\generalszh.exe" "%DEV_DEST%\generalszh.exe"
copy /Y "Output\ZeroHour\generalszh.pdb" "%DEV_DEST%\generalszh.pdb"

echo ========================================================
echo Final Operation Complete.

@echo off
setlocal enabledelayedexpansion
cd /d "%~dp0"

set "EXE_NAME=vex.exe"
set "NSIS_SCRIPT=installer.nsi"
set "STAGING_DIR=build"

set "ESC="
set "GREEN_BG=%ESC%[42;37m"
set "DG_WHITE=%ESC%[32;1m"
set "PURPLE=%ESC%[95m"
set "RED=%ESC%[91m"
set "GREY=%ESC%[90m"
set "RESET=%ESC%[0m"

chcp 65001 >nul

echo %GREEN_BG%▀████▀        ▀███▀                        %RESET%
echo %GREEN_BG%  ▀██         ▄█                           %RESET%
echo %GREEN_BG%   ██▄      ▄█        ▄▄█▀██    ▀██▀   ▀██▀%RESET%
echo %GREEN_BG%   ██▄    █▀        ▄█▀    ██     ▀██ ▄█▀  %RESET%
echo %GREEN_BG%   ▀██  █▀          ██▀▀▀▀▀▀       ███     %RESET%
echo %GREEN_BG%    ▄██▄            ██▄    ▄     ▄█▀ ██▄   %RESET%
echo %GREEN_BG%     ██              ▀█████▀   ▄██▄   ▄██▄ %RESET%
echo %RESET%

echo %DG_WHITE%Working directory: %CD%%RESET%
echo.
echo %DG_WHITE%[1/5] Detecting Qt installation...%RESET%
set "QT_ROOT="
set "QT_DIR="

for %%r in (C:\Qt D:\Qt "%PROGRAMFILES%\Qt" "%PROGRAMFILES(x86)%\Qt") do (
    if exist "%%~r" set "QT_ROOT=%%~r"
)

if not defined QT_ROOT (
    echo.
    echo %PURPLE%Qt not found in default locations.%RESET%
    echo %DG_WHITE%Enter Qt installation path [C:\Qt]:%RESET%
    set /p QT_ROOT=""
    if "!QT_ROOT!"=="" set "QT_ROOT=C:\Qt"
)

if not exist "%QT_ROOT%" (
    echo %RED%[ERROR] %QT_ROOT% not found%RESET%
    pause
    exit /b 1
)

echo.
set "VERSION_COUNT=0"
for /d %%v in ("%QT_ROOT%\6.*") do (
    set /a VERSION_COUNT+=1
    set "VERSION_!VERSION_COUNT!=%%~nxv"
    if exist "%%v\mingw_64\bin\windeployqt.exe" (
        echo %DG_WHITE%!VERSION_COUNT!: %%~nxv%RESET%
    ) else (
        echo %GREY%!VERSION_COUNT!: %%~nxv [no mingw_64]%RESET%
    )
)

if %VERSION_COUNT% equ 0 (
    echo %RED%[ERROR] No Qt 6 versions found in %QT_ROOT%%RESET%
    pause
    exit /b 1
)

:choose_version
echo.
set /p VERSION_CHOICE="%DG_WHITE%Select version (1-%VERSION_COUNT%): %RESET%"

if "%VERSION_CHOICE%"=="" goto choose_version
if %VERSION_CHOICE% lss 1 goto choose_version
if %VERSION_CHOICE% gtr %VERSION_COUNT% goto choose_version

set "SELECTED_VERSION=!VERSION_%VERSION_CHOICE%!"
set "QT_DIR=%QT_ROOT%\%SELECTED_VERSION%\mingw_64\bin"

if not exist "%QT_DIR%\windeployqt.exe" (
    echo %RED%[ERROR] %QT_DIR%\windeployqt.exe not found%RESET%
    pause
    exit /b 1
)

echo %DG_WHITE%Using: %QT_DIR%%RESET%
echo.

echo %DG_WHITE%[2/5] Detecting build directory...%RESET%
set "BUILD_DIR="
set "BUILD_COUNT=0"
set "BUILD_PATHS="

if exist "..\..\build\" (
    echo %GREY%Searching in ..\..\build\...%RESET%
    for /d %%i in ("..\..\build\*") do (
        if exist "%%i\%EXE_NAME%" (
            set /a BUILD_COUNT+=1
            set "BUILD_PATH_!BUILD_COUNT!=%%i"
            echo %DG_WHITE%!BUILD_COUNT!: %%i%RESET%
        ) else (
            if exist "%%i\release\%EXE_NAME%" (
                set /a BUILD_COUNT+=1
                set "BUILD_PATH_!BUILD_COUNT!=%%i\release"
                echo %DG_WHITE%!BUILD_COUNT!: %%i\release%RESET%
            )
            if exist "%%i\debug\%EXE_NAME%" (
                set /a BUILD_COUNT+=1
                set "BUILD_PATH_!BUILD_COUNT!=%%i\debug"
                echo %DG_WHITE%!BUILD_COUNT!: %%i\debug%RESET%
            )
            if exist "%%i\minsizerel\%EXE_NAME%" (
                set /a BUILD_COUNT+=1
                set "BUILD_PATH_!BUILD_COUNT!=%%i\minsizerel"
                echo %DG_WHITE%!BUILD_COUNT!: %%i\minsizerel%RESET%
            )
            if exist "%%i\relwithdebinfo\%EXE_NAME%" (
                set /a BUILD_COUNT+=1
                set "BUILD_PATH_!BUILD_COUNT!=%%i\relwithdebinfo"
                echo %DG_WHITE%!BUILD_COUNT!: %%i\relwithdebinfo%RESET%
            )
        )
    )
)

if %BUILD_COUNT% equ 0 (
    echo %PURPLE%No build directories found automatically.%RESET%
    echo %DG_WHITE%Enter path to folder containing %EXE_NAME%:%RESET%
    set /p BUILD_DIR="Path: "
) else if %BUILD_COUNT% equ 1 (
    set "BUILD_DIR=!BUILD_PATH_1!"
    echo %DG_WHITE%Using: !BUILD_PATH_1!%RESET%
) else (
    echo.
    echo %DG_WHITE%Multiple build locations found:%RESET%
    for /l %%n in (1,1,%BUILD_COUNT%) do (
        echo %DG_WHITE%  %%n: !BUILD_PATH_%%n!%RESET%
    )
    echo.
    :choose_build
    set /p BUILD_CHOICE="%DG_WHITE%Select build directory (1-%BUILD_COUNT%): %RESET%"
    if "!BUILD_CHOICE!"=="" goto choose_build
    if !BUILD_CHOICE! lss 1 goto choose_build
    if !BUILD_CHOICE! gtr %BUILD_COUNT% goto choose_build
    set "BUILD_DIR=!BUILD_PATH_%BUILD_CHOICE%!"
)

if not exist "%BUILD_DIR%\%EXE_NAME%" (
    echo %RED%[ERROR] %EXE_NAME% not found in %BUILD_DIR%%RESET%
    pause
    exit /b 1
)
echo %DG_WHITE%Using: %BUILD_DIR%%RESET%
echo.

echo %DG_WHITE%[3/5] Preparing staging directory...%RESET%
echo ___________________________________________
echo.

if exist "%STAGING_DIR%" rmdir /s /q "%STAGING_DIR%"
mkdir "%STAGING_DIR%"
echo %DG_WHITE%Staging: %CD%\%STAGING_DIR%%RESET%
echo.

echo %DG_WHITE%[4/5] Generating installer graphics...%RESET%
echo ___________________________________________
echo.

if not exist "assets" mkdir assets
cd assets

where magick >nul 2>&1
if %errorlevel% neq 0 (
    echo %PURPLE%[WARNING] ImageMagick not found. Skipping.%RESET%
    echo %DG_WHITE%Manually copy .bmp and .ico to assets folder.%RESET%
) else (
    if exist "..\src\banner.svg" (
        echo %GREY%banner.svg ^> banner.bmp%RESET%
        magick "..\src\banner.svg" -resize 164x314 -extent 164x314 -gravity center BMP3:banner.bmp
    )

    if exist "..\src\header.svg" (
        echo %GREY%header.svg ^> header.bmp%RESET%
        magick "..\src\header.svg" -resize 150x57 -extent 150x57 -gravity center BMP3:header.bmp
    )

    if exist "..\..\header.svg" (
        echo %GREY%header.svg ^> header.bmp%RESET%
        magick "..\src\header.svg" -resize 150x57 -extent 150x57 -gravity center BMP3:header.bmp
    )
    if exist "..\vex.svg" (
        echo %GREY%vex.svg ^> vex.ico%RESET%
        magick -size 512x512 xc:none "..\vex.svg" -composite -define icon:auto-resize=512,256,128,96,64,48,32,24,16 vex.ico
    )

    if exist "..\src\txt.svg" (
        echo %GREY%txt.svg ^> txt.ico%RESET%
        magick -size 512x512 -background transparent xc:transparent "..\src\txt.svg" -composite -define icon:auto-resize=512,256,128,96,64,48,32,24,16 txt.ico
    )

    if exist "..\src\vexpkg.svg" (
        echo %GREY%vexpkg.svg ^> vexpkg.ico%RESET%
        magick -size 512x512 -background transparent xc:transparent "..\src\vexpkg.svg" -composite -define icon:auto-resize=512,256,128,96,64,48,32,24,16 vexpkg.ico
    )

    if exist "..\src\vxsyn.svg" (
        echo %GREY%vxsyn.svg ^> vxsyn.ico%RESET%
        magick -size 512x512 -background transparent xc:transparent "..\src\vxsyn.svg" -composite -define icon:auto-resize=512,256,128,96,64,48,32,24,16 vxsyn.ico
    )
)

cd ..
echo %DG_WHITE%Done.%RESET%
echo.

echo %DG_WHITE%[5/5] Deploying and packaging...%RESET%
echo ___________________________________________
echo.

copy "%BUILD_DIR%\%EXE_NAME%" "%STAGING_DIR%\" >nul

if exist "%BUILD_DIR%\LookAndFeelCore.dll" copy "%BUILD_DIR%\LookAndFeelCore.dll" "%STAGING_DIR%\" >nul 2>&1
if exist "%BUILD_DIR%\SyntaxCore.dll" copy "%BUILD_DIR%\SyntaxCore.dll" "%STAGING_DIR%\" >nul 2>&1
if exist "%BUILD_DIR%\VexCore.dll" copy "%BUILD_DIR%\VexCore.dll" "%STAGING_DIR%\" >nul 2>&1
copy "..\..\LICENSE" "%STAGING_DIR%\" >nul 2>&1

cd "%STAGING_DIR%"

if not exist "%EXE_NAME%" (
    echo %RED%[ERROR] %EXE_NAME% missing%RESET%
    cd ..
    pause
    exit /b 1
)

"%QT_DIR%\windeployqt.exe" "%EXE_NAME%" ^
    --dir . ^
    --no-opengl --no-opengl-sw --no-qml --no-quick ^
    --no-printsupport --no-multimedia --no-sql ^
    --no-network --no-xml --no-concurrent --no-svg ^
    --no-translations

if %errorlevel% neq 0 (
    echo %RED%[ERROR] windeployqt failed%RESET%
    cd ..
    pause
    exit /b %errorlevel%
)

cd ..
echo.

if exist "vex.ico" copy "vex.ico" "assets\" >nul 2>&1
if exist "assets\*.bmp" copy "assets\*.bmp" "%STAGING_DIR%\" >nul 2>&1
if exist "assets\*.ico" copy "assets\*.ico" "%STAGING_DIR%\" >nul 2>&1
copy "%NSIS_SCRIPT%" "%STAGING_DIR%\" >nul 2>&1

set "NSIS_PATH="
for %%p in (
    "C:\Program Files\NSIS\makensis.exe"
    "C:\Program Files (x86)\NSIS\makensis.exe"
) do (
    if exist "%%~p" set "NSIS_PATH=%%~p"
)

if not defined NSIS_PATH (
    echo %RED%[ERROR] NSIS not found. Get it from https://nsis.sourceforge.io%RESET%
    pause
    exit /b 1
)

cd "%STAGING_DIR%"
"%NSIS_PATH%" "%NSIS_SCRIPT%"

if %errorlevel% equ 0 (
    echo.
    echo %DG_WHITE%_______________________________________^<^!^>%RESET%
    echo %DG_WHITE%.%RESET%
    echo %DG_WHITE%Success! Vex_4.1_Setup.exe created%RESET%
    echo %DG_WHITE%_______________________________________________/%RESET%
    copy "Vex_4.1_Setup.exe" "..\..\Packages\" >nul
) else (
    echo %RED%[ERROR] NSIS compilation failed%RESET%
)

cd ..
echo.
pause

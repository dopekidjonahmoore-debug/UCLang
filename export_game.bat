@echo off
setlocal

if "%1"=="" (
    echo Usage: export_game ^<game.uc^> [output_dir]
    echo.
    echo   Exports a UCLang game as a standalone package.
    echo   Creates [output_dir]/ with the game .exe and bundled stdlib.
    echo.
    exit /b 1
)

set GAME_SCRIPT=%~f1
set GAME_NAME=%~n1
set OUT_DIR=%2
if "%OUT_DIR%"=="" set OUT_DIR=%~dp0%GAME_NAME%_export
set BUILD_DIR=%TEMP%\uclang_export_%GAME_NAME%

echo [1/5] Preparing export at %OUT_DIR%...

if exist "%OUT_DIR%" rmdir /s /q "%OUT_DIR%" 2>nul
mkdir "%OUT_DIR%" 2>nul
mkdir "%BUILD_DIR%" 2>nul

echo [2/5] Bundling stdlib into executable...

REM Generate a bundled temp .cpp that embeds ALL stdlib modules plus the game script
set TMP_CPP=%BUILD_DIR%\bundled_app.cpp

REM Get the directory of this script
set SCRIPT_DIR=%~dp0

REM Read the app template
set TEMPLATE=%SCRIPT_DIR%uclang_app_template.cpp

REM Build bundled content: per-file raw string literal variables (avoids MSVC 16380-char limit)
powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPT_DIR%embed-bundler.ps1" -gameScript "%GAME_SCRIPT%" -outputCpp "%TMP_CPP%"

if %errorlevel% neq 0 (
    echo [FAIL] Could not bundle game
    exit /b 1
)

echo [3/5] Copying assets...
if exist "%SCRIPT_DIR%assets\" (
    mkdir "%OUT_DIR%\assets" 2>nul
    xcopy /E /I /Y "%SCRIPT_DIR%assets\*" "%OUT_DIR%\assets\" >nul
)
if exist "%~dp1assets\" (
    mkdir "%OUT_DIR%\assets" 2>nul
    xcopy /E /I /Y "%~dp1assets\*" "%OUT_DIR%\assets\" >nul
)

echo [4/5] Copying runtime DLLs...
if exist "%SCRIPT_DIR%SDL2.dll" copy "%SCRIPT_DIR%SDL2.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%SDL2_image.dll" copy "%SCRIPT_DIR%SDL2_image.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%SDL2_mixer.dll" copy "%SCRIPT_DIR%SDL2_mixer.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%SDL2_ttf.dll" copy "%SCRIPT_DIR%SDL2_ttf.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%third_party\dll\SDL2.dll" copy "%SCRIPT_DIR%third_party\dll\SDL2.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%third_party\dll\SDL2_image.dll" copy "%SCRIPT_DIR%third_party\dll\SDL2_image.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%third_party\dll\SDL2_mixer.dll" copy "%SCRIPT_DIR%third_party\dll\SDL2_mixer.dll" "%OUT_DIR%\" >nul 2>&1
if exist "%SCRIPT_DIR%third_party\dll\SDL2_ttf.dll" copy "%SCRIPT_DIR%third_party\dll\SDL2_ttf.dll" "%OUT_DIR%\" >nul 2>&1

echo [5/5] Compiling standalone executable...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

pushd "%SCRIPT_DIR%"
cl /O2 /MT /std:c++17 /EHsc /I. /Isrc /Ithird_party\include /Ithird_party\glm "%TMP_CPP%" ^
    src\core.cpp lexer.cpp src\parser.cpp src\interpreter.cpp ^
    src\sdl_builtins.cpp src\math_builtins.cpp src\physics_builtins.cpp ^
    src\gl_builtins.cpp src\entity_builtins.cpp src\serialization_builtins.cpp ^
    src\animation_builtins.cpp src\asset_builtins.cpp ^
    /Fe:"%OUT_DIR%\%GAME_NAME%.exe" ^
    /link /SUBSYSTEM:WINDOWS /LIBPATH:third_party\lib ^
    user32.lib SDL2.lib SDL2_ttf.lib SDL2_image.lib SDL2_mixer.lib opengl32.lib
popd

if exist "%OUT_DIR%\%GAME_NAME%.exe" (
    echo.
    echo ====== EXPORT SUCCESS ======
    echo Game exported to: %OUT_DIR%
    echo Files:
    dir "%OUT_DIR%" /b
    echo.
    echo To distribute, zip the entire %OUT_DIR% folder.
    del "%TMP_CPP%" 2>nul
    rmdir /s /q "%BUILD_DIR%" 2>nul
) else (
    echo [FAIL] Compilation failed
    exit /b 1
)

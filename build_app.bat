@echo off
setlocal

if "%1"=="" (
    echo Usage: build_app ^<file.uc^> [output.exe]
    echo.
    echo   Builds a standalone .exe from a .uc script.
    exit /b 1
)

set UC_FILE=%~f1
set EXE_NAME=%2
if "%EXE_NAME%"=="" set EXE_NAME=%~n1.exe

set TMP_SRC=%TEMP%\uclang_app_%~n1.cpp

echo [1/3] Embedding %UC_FILE% into executable wrapper...

REM Use PowerShell to replace EMBEDDED_SCRIPT marker with actual script content.
REM The .Replace() method avoids regex/backreference issues with $ signs in scripts.
powershell -NoProfile -Command ^
    "$tmpl = [System.IO.File]::ReadAllText('uclang_app_template.cpp');" ^
    "$src  = [System.IO.File]::ReadAllText('%UC_FILE%');" ^
    "$out  = $tmpl.Replace('EMBEDDED_SCRIPT', $src);" ^
    "Set-Content -Path '%TMP_SRC%' -Value $out"
if %errorlevel% neq 0 (
    echo [FAIL] Could not embed script
    exit /b 1
)

echo [2/3] Compiling %EXE_NAME%...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1

cl /O2 /MT /std:c++17 /EHsc /I. /Isrc /Ithird_party/include /Ithird_party/glm "%TMP_SRC%" src/core.cpp lexer.cpp src/parser.cpp src/interpreter.cpp src/sdl_builtins.cpp src/math_builtins.cpp src/physics_builtins.cpp src/gl_builtins.cpp /Fe:%EXE_NAME% /link /SUBSYSTEM:WINDOWS /LIBPATH:third_party/lib user32.lib SDL2.lib SDL2_ttf.lib SDL2_image.lib SDL2_mixer.lib opengl32.lib

if exist %EXE_NAME% (
    echo [3/3] Success: %EXE_NAME%
    del "%TMP_SRC%" 2>nul
) else (
    echo [FAIL] Compilation failed
    del "%TMP_SRC%" 2>nul
    exit /b 1
)

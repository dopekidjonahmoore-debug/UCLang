@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /O2 /MT /std:c++17 /EHsc /I. /Isrc /Ithird_party/include /Ithird_party/glm src/main.cpp src/core.cpp lexer.cpp src/parser.cpp src/interpreter.cpp src/sdl_builtins.cpp src/math_builtins.cpp src/physics_builtins.cpp src/gl_builtins.cpp src/compiler.cpp /Fe:UCLangStudio.exe /link /SUBSYSTEM:WINDOWS /LIBPATH:third_party/lib user32.lib gdi32.lib comctl32.lib shell32.lib comdlg32.lib SDL2.lib SDL2_ttf.lib SDL2_image.lib SDL2_mixer.lib opengl32.lib
echo ======
if exist UCLangStudio.exe (
    echo Build successful: UCLangStudio.exe
) else (
    echo BUILD FAILED
)

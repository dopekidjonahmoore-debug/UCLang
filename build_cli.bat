@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /O2 /MT /std:c++17 /EHsc /I. /Isrc /Ithird_party/include /Ithird_party/glm uclang_cli.cpp src/core.cpp lexer.cpp src/parser.cpp src/interpreter.cpp src/sdl_builtins.cpp src/math_builtins.cpp src/physics_builtins.cpp src/gl_builtins.cpp src/entity_builtins.cpp src/serialization_builtins.cpp src/animation_builtins.cpp src/asset_builtins.cpp /Fe:uclang.exe /link /LIBPATH:third_party/lib SDL2.lib SDL2_ttf.lib SDL2_image.lib SDL2_mixer.lib opengl32.lib
echo ======
if exist uclang.exe (
    echo Build successful: uclang.exe
) else (
    echo BUILD FAILED
)

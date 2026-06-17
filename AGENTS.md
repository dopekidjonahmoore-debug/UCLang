# UCLang — Agent Guide

## Build Commands
- `build_cli.bat` → `uclang.exe` (console interpreter)
- `build_app.bat <script.uc>` → `<script>.exe` (standalone, no console window, MessageBox output)
- UCLangStudio.exe is pre-built in repo

## Test Commands
- Run any `.uc` script: `uclang.exe <file.uc>`
- Stdlib parse check: `uclang.exe stdlib\*.uc` — all should exit 0 (no main needed)
- The `--check` flag is **not** implemented in the CLI; just run the file directly

## Architecture
```
src/
  core.h/cpp          — C API (UCLangVM), runtime types (Value), native function registry
  lexer.h/cpp         — Tokenizer: identifiers lowercased, # line comments, OP_DOT token
  parser.h/cpp        — AST builder: parseNamespaceCall, If/GameLoop sugar, statement-level run.funcname()
  interpreter.h/cpp   — Bytecode compiler + VM executor, registerBuiltins(), Environment (set=local, setOrUpdate=parent-walk)
  sdl_builtins.h/cpp  — Window.*, Sprite.*, Audio.*, Draw.*, Input.*, Font.*, Timer.*, GameLoop
  math_builtins.h/cpp — Math.*, Random.*, list_*, int_to_str, type introspection
  physics_builtins.h/cpp — Physics.* (AABB, RigidBody2D, collision, spatial grid)
  gl_builtins.h/cpp   — Window.open_3d, Mesh.*, Texture.*, Shader.*, Camera3D.*, Light.*, Draw3D.*
uclang_cli.cpp        — main() entry for CLI interpreter
stdlib/               — UCLang standard library (loaded at runtime via run_file)
  animation.uc        — Tween system + easing functions
  ecs.uc              — Entity-Component-System
  particles.uc        — Particle emitter system
  pathfind.uc         — A* pathfinding
  scene.uc            — Scene management
  ui.uc               — UI framework
uclang_app_template.cpp — Embedding skeleton (EMBEDDED_SCRIPT placeholder replaced by build_app.bat)
third_party/          — SDL2, SDL2_ttf, SDL2_image, SDL2_mixer VC x64 dev packages + GLM header-only
```

## Key Language Rules
- All identifiers are lowercased by lexer; string literals preserve case
- `run.funcname()` for statement-level function calls
- `input.xxx()` is a namespace call (BUILTIN_INPUT + OP_DOT)
- `GameLoop(update(…). draw(…). )`
- `If(cond then(…). )` — `.` must be inside `then(…)`
- `and`/`or` are binary operators (precedence 2/1), NO short-circuit
- `Environment::set()` is local-only; `setOrUpdate()` walks parent chain
- `list_set` requires valid index < list size; use `list_push` for append

## Native Function Naming
- All native functions registered with lowercase names
- `register_gl_natives` (gl_builtins.cpp) — Window.open_3d, Mesh.*, Texture.*
- `register_shader_natives` (gl_builtins.cpp) — Shader.*, Camera3D.*, Light.*, Draw3D.*
- Both must be called in `registerBuiltins()` (interpreter.cpp)
- `execFuncCall` checks `m_nativeFuncs` first, then `m_functions`

## GameLoop Flow
```
run_sdl_game_loop(sdl_builtins.cpp):
  1. Poll SDL events
  2. Update input state
  3. Call updateFn()
  4. Call drawFn()
  5. Swap buffers (SDL_GL_SwapWindow for 3D, SDL_RenderPresent for 2D)
```
**Draw3D.end() does NOT swap** — the game loop owns the swap. Use Draw3D only inside a GameLoop.

## Build Chain
1. `build_cli.bat` — Compiles all `.cpp` + `uclang_cli.cpp` → `uclang.exe` (/SUBSYSTEM:CONSOLE)
2. `build_app.bat` — Reads template, embeds `.uc` script as string, compiles → `<name>.exe` (/SUBSYSTEM:WINDOWS, WinMain, MessageBoxA)

## GitHub
- Remote: https://github.com/dopekidjonahmoore-debug/UCLang.git
- Branch: master
- Submodule: `third_party/glm` (header-only GLM)

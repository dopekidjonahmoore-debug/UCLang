# UCLang — Agent Guide

## Build Commands
- `build_cli.bat` → `uclang.exe` (console interpreter)
- `build_app.bat <script.uc>` → `<script>.exe` (standalone, no console window, MessageBox output)
- `export_game.bat <game.uc> [output_dir]` → creates standalone game package with DLLs + bundled stdlib
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
uclang_app_template.cpp — Embedding skeleton (FILE_CONTENTS/FILE_CONCAT markers replaced by export pipeline)
embed-bundler.ps1 — PowerShell script that generates per-file raw string literal variables for export
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

## OOP System (Completed)
- Classes: `class Name { fields methods }`, `new ClassName(args.)`, `this`, `super`
- Inheritance: `class Child extends Parent { }`
- Methods: `def name(params.) body. )` — `.` INSIDE parens, body between `.` and `)`, closing `)` on its own line
- `response(expr).` — must use parentheses
- Constructors: `def init(args.)` — called automatically on `new`
- Fields: `name == default.` — inherited fields walk parent chain, child overrides parent
- Objects use `shared_ptr<UCLangObjectData>` in Value variant (reference semantics)
- Classes defined in imported files work with extends (must close class body with `}`, not `)`)

## Native Math Types (Completed)
- `Vec3` (glm::vec3): `.x .y .z`, `+ - * /`, `.normalize() .length() .dot(v) .cross(v)`
- `Quat` (glm::quat): `.x .y .z .w`, `*`, `.normalize() .conjugate() .inverse()`
- `Mat4` (glm::mat4): `*` (with Mat4 or Vec3), `.inverse() .transpose() .translate(x,y,z) .rotate(angle,x,y,z) .scale(x,y,z)`
- Factory functions: `vec3(x,y,z)`, `quat(x,y,z,w)`, `mat4()`, `mat4.translate(x,y,z)`, `mat4.rotate(angle,x,y,z)` etc.
- All registered lowercase in `m_nativeFuncs` (lexer lowercases identifiers)
- `execMemberGet` handles GLM field access; `execMemberCall` handles GLM methods; `execBinaryOp` handles GLM arithmetic

## Coroutines (Completed)
- `yield(val.)` throws `YieldSignal{val}`
- `coro_start(fn_name, args...)` returns Coroutine handle
- `coro_resume(coro, val)` resumes, returns yielded value or final response
- `coro_done(coro)` checks if done
- Coroutine functions check `__yield_step` (auto-incremented) to branch to correct yield point
- `__resume_val` has the value passed to coro_resume
- stdlib/coroutine.uc has `coro_run`, `coro_run_all(dt)`, `coro_wait_seconds(s)` helpers
- `YieldSignal` exception caught in `execFuncCall` propagates up; caught in `coro_resume` callback

## Component Lifecycle (Completed)
- stdlib/component.uc: `Component` class with `on_start()`, `on_update(dt)`, `on_destroy()`, `on_collision(other)`
- `comp_register(comp)` activates component
- `comp_unregister(comp)` destroys and removes
- `comp_update_all(dt)` updates all active components
- Extend Component class and override lifecycle methods

## Event System (Completed)
- stdlib/events.uc: `event_create`, `event_sub`, `event_unsub`, `event_pub`
- Global event bus: `event_bus_sub(name, handler_fn)`, `event_bus_pub(name, args)`, `event_bus_find(name)`
- List mutation caveat: `list_set` returns a copy; must store result back into __events

## UI Anchors (Completed)
- stdlib/ui_canvas.uc: elements now accept an `anchor` parameter: "topleft"(default), "top", "topright", "left", "center", "right", "bottomleft", "bottom", "bottomright", "stretch"
- `ui_apply_anchor(anchor, rx, ry, rw, rh)` converts relative to absolute
- `ui_set_screen_size(w, h)` for responsive layouts
- Backward-compatible `ui_*_old()` helpers provided

## struct (Completed)
- `struct Name { fields methods }` — parsed as `ClassDefNode`, registered in `m_classes` just like classes
- Fields parsed same as classes: `visibility? static? name == default.`
- Methods parsed same as classes: `visibility? static? def name(params.) body. )`
- Structs in UCLang currently use reference semantics (identical to classes at runtime)
- No constructor call needed — `struct` is just a named data container with methods

## interface (Completed)
- `interface Name { method_signatures }` — defines method contracts
- `class Name implements Interface1, Interface2 { }` — must implement all interface methods
- Interface methods have no body (parsed as empty body `FuncDefNode`)
- When a class is defined (`execClassDef`), it checks that all interface methods are implemented
- Interfaces can extend other interfaces (extends tokens are parsed but parent list currently discarded)
- Interface methods are always `public`, not static, not override

## Access Modifiers (Completed)
- `public` — accessible from anywhere (default)
- `private` — accessible only from methods of the **class that defined** the field/method
- `protected` — accessible from the defining class and its subclasses
- Interpreter tracks **two** class names during execution:
  - `m_currentThisClassName` — the runtime class of the `this` object
  - `m_currentDefiningClassName` — the class that **defines** the currently executing method
  - `private` checks use `m_currentDefiningClassName` (not the runtime class), so inherited methods correctly access their own class's private fields
- `checkAccess(fieldDefiningClass, member)` walks the field's inheritance chain to find which class defined it
- `findFieldDefiningClass()` and `findMethodDefiningClass()` walk the parent chain to find the defining class
- Access modifiers are enforced at runtime in `execMemberGet`/`execMemberSet` — private/protected violations throw errors

## Vertex/Mesh Collision (Completed)
- `physics.add_mesh_collider(name, vertex_list, triangle_list?, is_convex?)` — custom mesh collision
  - `vertex_list`: list of `vec3` values defining model vertices
  - `triangle_list` (optional): list of `[i0,i1,i2]` index triples; if omitted, auto-generates fan triangulation
  - `is_convex` (optional): `1`=convex (uses GJK, fastest), `0`=concave (per-triangle collision, slower)
- **GJK (Gilbert-Johnson-Keerthi)** algorithm for convex hull vs any shape collision detection
- **EPA (Expanding Polytope Algorithm)** for collision normal and penetration depth
- Mesh vs sphere collision using per-triangle distance test
- Mesh vs box collision using GJK
- Mesh vs mesh collision using GJK
- `physics.get_mesh_vertices(name)` — get current world-space vertices of a mesh collider
- Mesh world-space transforms updated automatically each physics step
- Auto-create body at origin if name doesn't exist yet (mass=0, static)

## 3D Physics (Completed)
- `physics.set_gravity(gx, gy, gz)` — set 3D gravity vector (default 0,-9.8,0)
- `physics.add_body(name, x, y, z, mass)` — create a physics body (box collider by default)
- `physics.add_box_collider(name, w, h, d)` — set box collider dimensions
- `physics.add_sphere_collider(name, radius)` — set sphere collider
- `physics.add_mesh_collider(name, vertices, triangles?, convex?)` — custom vertex mesh collider (see above)
- `physics.set_velocity(name, vx, vy, vz)` — set velocity
- `physics.set_restitution(bounce)` — set restitution for all bodies (default 0.5)
- `physics.set_friction(drag)` — set friction/drag coefficient (default 0.02)
- `physics.apply_force(name, fx, fy, fz)` — apply a force impulse
- `physics.step(dt)` — step physics simulation (auto-clamped to 0.05 max)
- `physics.collision_test(name1, name2)` — returns 1 if bodies are colliding
- `physics.set_collision_enabled(1|0)` — enable/disable body-body collisions
- `physics.get_body_count()` — returns number of bodies
- `physics.remove_body(name)` — remove a body
- Collision detection: sphere-sphere, box-box, box-sphere
- Collision resolution: impulse-based with restitution, overlap separation
- Ground plane collision at y=0
- Velocity clamping to prevent tunneling

## Asset Pipeline / Serialization (Completed)
- `serialize.to_json(value, pretty?)` — convert UCLang value to JSON string
- `serialize.from_json(json_string)` — parse JSON string into UCLang value (lists, objects, strings, numbers, booleans, null)
- `serialize.save_json(path, value, pretty?)` — save value as JSON to file
- `serialize.load_json(path)` — load JSON from file into UCLang value
- `serialize.save_text(path, text)` — save text to file
- `serialize.load_text(path)` — load text from file
- `serialize.list_files(path)` — list directory entries (returns list of [name, type, size])
- `serialize.file_exists(path)` — check if file exists
- `serialize.copy_file(src, dst)` — copy binary file (returns true/false)
- JSON handles: null, bool, int, float, string, lists, and objects (UCLangObjectData with className="JsonObject")
- Serialization enables saving/loading scenes, configs, and game data

## Editor GUI Features (Completed)
UCLangStudio now includes:
- **Multiple tabs** — open many files, click tabs to switch, `Ctrl+Tab` to cycle, `Ctrl+W` to close, `+` button for new tab
- **Brace/bracket autocomplete** — typing `{`, `(`, `[`, or `"` auto-closes the pair; backspace deletes matching pair
- **Brace matching** — cursor next to a brace highlights its match with an underline
- **Auto-indent** — new lines auto-indent to match previous line; extra indent after `{`
- **Advanced syntax highlighting** — separate colors for keywords, types, builtins, strings, numbers, operators, comments, punctuation (extensive keyword list covers all UCLang constructs)
- **Word count & line count** in status bar
- **Drag-and-drop file opening** with multi-file support
- **Auto-complete suggestions** — `Ctrl+Space` or type 2+ alpha chars

## Important Gotchas
- Class body closes with `}`, method body closes with `)` — easy to confuse!
- `extends` from imported files works (class must be registered in m_classes before use)
- `run_file` / `import` both register functions AND execute statements (including class defs)
- Coroutine step tracking uses `__yield_step` variable set in the function's environment
- Event system requires `ev == event_sub(ev, handler_fn)` to capture the modified event

## GitHub
- Remote: https://github.com/dopekidjonahmoore-debug/UCLang.git
- Branch: master
- Submodule: `third_party/glm` (header-only GLM)

## Scene/Prefab System (New)
- `stdlib/prefab.uc` — Prefab registry, instantiation with overrides
- `prefab_register(name, template_fn)` — register a prefab by name
- `prefab_instantiate(name, overrides)` — create instance with field overrides
- Scene format: JSON with `{"entities": [{"prefab":"name", "overrides":{...}}]}`
- `scene_entity_add(prefab_name, overrides_flat_list)` — add entity to current scene
- `scene_save(path)` / `scene_load(path)` — serialize/deserialize scene with prefab references
- `scene_instantiate()` — instantiate all scene entities

## Skeletal Animation (New)
- `animation_builtins.cpp` — Bone hierarchy, animation clips, animator state machine
- `stdlib/animator.uc` — `Animator` class (extends Component) wrapping native builtins
- `animator_create(bones, clips, states, transitions)` → animator ID (int)
- `animator_update(id, dt)` — advances state machine, interpolates bone transforms
- `animator_set_param(id, name, val)` / `animator_get_param(id, name)` — state machine parameters
- `animator_play(id, state_name)` — force transition to state
- `animator_get_bone_matrices(id)` — returns `[mat4, ...]` world-space bone matrices for skinning
- `animator_get_bone_local(id, index)` — returns `[pos_vec3, rot_quat, scale_vec3]`
- Data helpers: `animator_make_bone()`, `animator_make_keyframe()`, `animator_make_clip()`, `animator_make_state()`, `animator_make_transition()`
- Supports: state machines with parameter-based transitions, blend transitions, looping clips, keyframe interpolation (lerp/slerp), bone hierarchy matrix chain

## Asset GUID + Hot-Reload (New)
- `asset_builtins.cpp` — Stable GUID-based asset references + file watcher
- `asset_register(path, type)` → GUID string (returns existing GUID if path already registered)
- `asset_guid_to_path(guid)` → current resolved path (survives renames)
- `asset_path_to_guid(path)` → GUID
- `asset_rename(old_path, new_path)` — updates registry when asset renamed
- `asset_is_dirty(guid)` → 1 if file changed since last check (consumes the dirty flag)
- `asset_check_all()` → list of all dirty GUIDs
- `asset_watch_start()` / `asset_watch_stop()` — background thread polls file timestamps every 500ms
- `asset_save_registry(path)` / `asset_load_registry(path)` — persist GUID map to JSON
- `asset_list()` → all registered assets

## Export Pipeline (Completed)
- `export_game.bat <game.uc> [output_dir]` — full game packaging
- `embed-bundler.ps1` — PowerShell script called by export_game.bat; reads each stdlib file + game script and generates individual `const char*` variables using `R"uc(...)uc"` raw string literals (avoids MSVC 16380-char limit on string literals)
- `uclang_app_template.cpp` — skeleton with `// FILE_CONTENTS` and `// FILE_CONCAT` markers replaced by embed-bundler.ps1
- Bundles 19 stdlib modules into the executable (no need for `run_file` at runtime)
- Copies assets folder, SDL2 DLLs alongside the .exe
- Creates a self-contained game package ready for distribution
- Individual file size limit: ~16KB per file (MSVC raw string literal limit); all stdlib files are well under this

## Fixed: Particles
- `stdlib/particles.uc` now uses `Draw.circle` instead of `Overlay.circle`
- Particles work in both 2D (`Draw.*` mode) and 3D (`Overlay.*` requires `Overlay.begin()`)
- Game demo particle burst/paint/update all functional

## Reference Files
- `language.txt` — Comprehensive UCLang reference for feeding to other AIs. Lists every token, keyword, operator, built-in function (native + stdlib), syntax rule, and working examples. If this file is ever outdated, regenerate it from all source files.

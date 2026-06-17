# ── UCLang Engine Demo ─────────────────────────────────────
#   Combines all stdlib modules: animation, ecs, particles,
#   ui, pathfind, and scene into one cohesive test.
# ──────────────────────────────────────────────────────────

# Load all modules
run.run_file("stdlib/animation.uc").
run.run_file("stdlib/ecs.uc").
run.run_file("stdlib/pathfind.uc").
run.run_file("stdlib/scene.uc").

Print("=== UCLang Engine Demo ===\n").

# ── 1. Animation System ───────────────────────────────────
Print("─ 1. Animation / Tween System ─\n").

t == tween_create(0, 100, 10, "quad_out").
Print("  tween start: " + int_to_str(list_get(t, 0)) + ", end: " + int_to_str(list_get(t, 1)) + ", dur: " + int_to_str(list_get(t, 2)) + "\n").

t == tween_update(t, 5).
val == tween_value(t).
Print("  t=5s value: " + int_to_str(val) + " (expect ~75 for quad_out)\n").

t == tween_update(t, 5).
done == tween_done(t).
Print("  t=10s done: " + int_to_str(done) + ", val: " + int_to_str(tween_value(t)) + "\n").

# ── 2. ECS System ─────────────────────────────────────────
Print("─ 2. Entity-Component System ─\n").

w == ecs_new().
Print("  world created\n").

result == ecs_entity(w).
w == list_get(result, 0).
eid == list_get(result, 1).
Print("  entity created: " + int_to_str(eid) + "\n").

w == ecs_add(w, eid, "pos", [100, 200]).
w == ecs_add(w, eid, "speed", 5).
Print("  added pos, speed components\n").

hasPos == ecs_has(w, eid, "pos").
Print("  has pos? " + int_to_str(hasPos) + " (expect 1)\n").

pos == ecs_get(w, eid, "pos").
Print("  pos = (" + int_to_str(list_get(pos, 0)) + ", " + int_to_str(list_get(pos, 1)) + ")\n").

result2 == ecs_entity(w).
w == list_get(result2, 0).
eid2 == list_get(result2, 1).
w == ecs_add(w, eid2, "pos", [50, 75]).
Print("  entity " + int_to_str(eid2) + " created with pos\n").

results == ecs_query(w, "pos").
Print("  query(pos) count: " + int_to_str(list_len(results)) + " (expect 2)\n").

# ── 3. Pathfinding ────────────────────────────────────────
Print("─ 3. Grid Pathfinding ─\n").

g == pathfind_grid(8, 6).
g == pathfind_set(g, 3, 2, 1).
g == pathfind_set(g, 3, 3, 1).
g == pathfind_set(g, 3, 4, 1).
Print("  8x6 grid created, wall at col 3 rows 2-4\n").

path == pathfind_find(g, 0, 0, 7, 5).
plen == list_len(path).
Print("  path length: " + int_to_str(plen) + "\n").

i == 0.
While(i < plen then(
  p == list_get(path, i).
  Print("    (" + int_to_str(list_get(p, 0)) + "," + int_to_str(list_get(p, 1)) + ")\n").
  i == i + 1.
). ).

# ── 4. Scene System ──────────────────────────────────────
Print("─ 4. Scene System ─\n").

init_a(.
  Print("  scene A init\n").
).

update_a(.
  Print("  scene A update\n").
).

draw_a(.
  Print("  scene A draw\n").
).

run.scene_register("game", "init_a", "update_a", "draw_a").
Print("  scene 'game' registered\n").

# ── 5. UI Widgets ────────────────────────────────────────
Print("─ 5. UI Widgets ─\n").

btn == list_push([], "Click").
btn == list_push(btn, 10).
btn == list_push(btn, 20).
btn == list_push(btn, 100).
btn == list_push(btn, 30).
btn == list_push(btn, 0).
Print("  button created: " + list_get(btn, 0) + " at (" + int_to_str(list_get(btn, 1)) + "," + int_to_str(list_get(btn, 2)) + ")\n").

label == list_push([], "Hello UCLang").
label == list_push(label, 50).
label == list_push(label, 10).
Print("  label created: " + list_get(label, 0) + "\n").

panel == list_push([], 0).
panel == list_push(panel, 0).
panel == list_push(panel, 200).
panel == list_push(panel, 100).
panel == list_push(panel, "#334466").
Print("  panel created: " + int_to_str(list_get(panel, 2)) + "x" + int_to_str(list_get(panel, 3)) + "\n").

Print("\n=== Demo Complete ===\n").

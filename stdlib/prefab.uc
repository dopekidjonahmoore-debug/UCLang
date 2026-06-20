# ── Prefab & Scene System ──────────────────────────────────────
#  Prefab: a named default-data template (flat key/value list).
#  Scene:  a list of prefab instances with per-instance overrides.
#  Data format: flat list [key1, val1, key2, val2, ...]
# ────────────────────────────────────────────────────────────────

# ─── Prefab Registry ───
__prefabs == [].

def prefab_register(name, defaults.
  __prefabs == list_push(__prefabs, [name, defaults]).
).

def prefab_find(name.
  pfi == 0.
  While(pfi < list_len(__prefabs) then(
    If(list_get(__prefabs, pfi)[0] = name then(
      response(list_get(__prefabs, pfi)[1]).
    ). ).
    pfi == pfi + 1.
  ). ).
  response(0).
).

# ─── Prefab Instance ───
def prefab_instantiate(name, overrides.
  defaults == prefab_find(name).
  If(defaults = 0 then(
    response(0).
  ). ).
  pi_out == [].
  pi_idx == 0.
  While(pi_idx < list_len(defaults) then(
    pi_out == list_push(pi_out, defaults[pi_idx]).
    pi_idx == pi_idx + 1.
  ). ).
  pi_j == 0.
  While(pi_j < list_len(overrides) then(
    pi_key == overrides[pi_j].
    pi_val == overrides[pi_j + 1].
    pi_k == 0.
    While(pi_k < list_len(pi_out) then(
      If(pi_out[pi_k] = pi_key then(
        pi_out == list_set(pi_out, pi_k + 1, pi_val).
      ). ).
      pi_k == pi_k + 2.
    ). ).
    pi_j == pi_j + 2.
  ). ).
  response(pi_out).
).

# ─── Scene ───
__scene == [].

def scene_entity_add(prefab_name, overrides.
  __scene == list_push(__scene, [prefab_name, overrides]).
).

def scene_entity_remove(idx.
  __scene == list_remove(__scene, idx).
).

def scene_clear(.
  __scene == [].
).

def scene_get_entities(.
  response(__scene).
).

# ─── Scene Save/Load ───
def scene_save(path.
  sv_flat == [].
  sv_i == 0.
  While(sv_i < list_len(__scene) then(
    sv_entry == list_get(__scene, sv_i).
    sv_name == sv_entry[0].
    sv_over  == sv_entry[1].
    sv_eflat == [sv_name].
    sv_j == 0.
    While(sv_j < list_len(sv_over) then(
      sv_eflat == list_push(sv_eflat, sv_over[sv_j]).
      sv_j == sv_j + 1.
    ). ).
    sv_flat == list_push(sv_flat, sv_eflat).
    sv_i == sv_i + 1.
  ). ).
  json_str == serialize.to_json(sv_flat, 1).
  serialize.save_text(path, json_str).
).

def scene_load(path.
  json_str == serialize.load_text(path).
  loaded == serialize.from_json(json_str).
  __scene == [].
  sl_i == 0.
  While(sl_i < list_len(loaded) then(
    sl_entry == list_get(loaded, sl_i).
    sl_name == list_get(sl_entry, 0).
    sl_over == [].
    sl_j == 1.
    While(sl_j < list_len(sl_entry) then(
      sl_over == list_push(sl_over, list_get(sl_entry, sl_j)).
      sl_j == sl_j + 1.
    ). ).
    __scene == list_push(__scene, [sl_name, sl_over]).
    sl_i == sl_i + 1.
  ). ).
).

# ─── Scene Instantiation ───
def scene_instantiate(.
  si_acc == [].
  si_i == 0.
  While(si_i < list_len(__scene) then(
    si_entry == list_get(__scene, si_i).
    si_inst == prefab_instantiate(si_entry[0], si_entry[1]).
    si_acc == list_push(si_acc, si_inst).
    si_i == si_i + 1.
  ). ).
  response(si_acc).
).

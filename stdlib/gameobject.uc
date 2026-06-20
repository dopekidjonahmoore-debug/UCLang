def gameobject_create(name, x, y.
  eid == entity_create().
  entity_set(eid, "name", name).
  entity_set(eid, "position", x, y, 0.0).
  entity_set(eid, "rotation", 0.0).
  entity_set(eid, "scale", 1.0, 1.0, 1.0).
  entity_set(eid, "active", 1).
  response(eid).
).

def gameobject_destroy(eid.
  entity_destroy(eid).
).

def gameobject_find(name.
  eids == entity_all().
  i == 0.
  While(i < entity_count() then(
    eid == list_get(eids, i).
    If(entity_has(eid, "name") then(
      n == entity_get(eid, "name").
      If(list_get(n, 0) = name then(response(eid).). ).
    ). ).
    i == i + 1.
  ). ).
  response(-1).
).

def gameobject_find_all(name.
  result == [].
  eids == entity_all().
  i == 0.
  While(i < entity_count() then(
    eid == list_get(eids, i).
    If(entity_has(eid, "name") then(
      n == entity_get(eid, "name").
      If(list_get(n, 0) = name then(
        result == list_push(result, eid).
      ). ).
    ). ).
    i == i + 1.
  ). ).
  response(result).
).

def gameobject_set_active(eid, active.
  entity_set(eid, "active", active).
).

def gameobject_is_active(eid.
  a == entity_get(eid, "active").
  If(list_len(a) = 0 then(response(0).). ).
  response(list_get(a, 0)).
).

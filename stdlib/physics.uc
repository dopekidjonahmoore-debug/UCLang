def physics_update(dt.
  eids == entity_all().
  i == 0.
  While(i < entity_count() then(
    eid == list_get(eids, i).
    If(entity_has(eid, "position") and entity_has(eid, "velocity") then(
      pos == entity_get(eid, "position").
      vel == entity_get(eid, "velocity").
      If(entity_has(eid, "gravity") then(
        g == entity_get(eid, "gravity").
        gy == list_get(g, 0).
        vel == list_set(vel, 1, list_get(vel, 1) + gy * dt).
      ). ).
      pos == list_set(pos, 0, list_get(pos, 0) + list_get(vel, 0) * dt).
      pos == list_set(pos, 1, list_get(pos, 1) + list_get(vel, 1) * dt).
      pos == list_set(pos, 2, list_get(pos, 2) + list_get(vel, 2) * dt).
      If(entity_has(eid, "ground_y") then(
        gycomp == entity_get(eid, "ground_y").
        ground == list_get(gycomp, 0).
        If(list_get(pos, 1) < ground then(
          pos == list_set(pos, 1, ground).
          vel == list_set(vel, 0, 0.0).
          vel == list_set(vel, 1, 0.0).
          vel == list_set(vel, 2, 0.0).
        ). ).
      ). ).
      entity_set(eid, "position", list_get(pos, 0), list_get(pos, 1), list_get(pos, 2)).
      entity_set(eid, "velocity", list_get(vel, 0), list_get(vel, 1), list_get(vel, 2)).
    ). ).
    i == i + 1.
  ). ).
).

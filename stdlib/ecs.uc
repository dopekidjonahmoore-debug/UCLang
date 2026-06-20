def entity_new(.
  response(entity_create()).
).

def entity_add_component(eid, tag.
  entity_set(eid, tag).
).

def entity_get_pos(eid.
  response(entity_get(eid, "position")).
).

def entity_set_pos(eid, x, y, z.
  entity_set(eid, "position", x, y, z).
).

def entity_get_vel(eid.
  response(entity_get(eid, "velocity")).
).

def entity_set_vel(eid, x, y, z.
  entity_set(eid, "velocity", x, y, z).
).

def entity_query(tag.
  result == [].
  eids == entity_all().
  i == 0.
  While(i < entity_count() then(
    eid == list_get(eids, i).
    If(entity_has(eid, tag) then(
      result == list_push(result, eid).
    ). ).
    i == i + 1.
  ). ).
  response(result).
).

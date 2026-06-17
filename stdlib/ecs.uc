ecs_new(.
  response([0, []]).
).

ecs_entity(w.
  nextId == list_get(w, 0).
  entities == list_get(w, 1).
  entities == list_push(entities, [nextId, []]).
  w == list_set(w, 0, nextId + 1).
  w == list_set(w, 1, entities).
  response([w, nextId]).
).

ecs_add(w, eid, comp, val.
  entities == list_get(w, 1).
  i == 0.
  found == 0.
  While(i < list_len(entities) then(
    e == list_get(entities, i).
    If(list_get(e, 0) = eid then(
      comps == list_get(e, 1).
      comps == list_push(comps, val).
      comps == list_push(comps, comp).
      entities == list_set(entities, i, [eid, comps]).
      w == list_set(w, 1, entities).
      found == 1.
      i == list_len(entities).
    ). ).
    i == i + 1.
  ). ).
  response(w).
).

ecs_get(w, eid, comp.
  entities == list_get(w, 1).
  i == 0.
  result == 0.
  While(i < list_len(entities) then(
    e == list_get(entities, i).
    If(list_get(e, 0) = eid then(
      comps == list_get(e, 1).
      ci == 0.
      While(ci < list_len(comps) then(
        If(list_get(comps, ci) = comp then(
          result == list_get(comps, ci - 1).
          ci == list_len(comps).
        ). ).
        ci == ci + 1.
      ). ).
      i == list_len(entities).
    ). ).
    i == i + 1.
  ). ).
  response(result).
).

ecs_has(w, eid, comp.
  entities == list_get(w, 1).
  i == 0.
  found == 0.
  While(i < list_len(entities) then(
    e == list_get(entities, i).
    If(list_get(e, 0) = eid then(
      comps == list_get(e, 1).
      ci == 0.
      While(ci < list_len(comps) then(
        If(list_get(comps, ci) = comp then(
          found == 1.
          ci == list_len(comps).
        ). ).
        ci == ci + 1.
      ). ).
      i == list_len(entities).
    ). ).
    i == i + 1.
  ). ).
  response(found).
).

ecs_remove(w, eid, comp.
  entities == list_get(w, 1).
  i == 0.
  While(i < list_len(entities) then(
    e == list_get(entities, i).
    If(list_get(e, 0) = eid then(
      comps == list_get(e, 1).
      ci == 0.
      While(ci < list_len(comps) then(
        If(list_get(comps, ci) = comp then(
          comps == list_remove(comps, ci).
          comps == list_remove(comps, ci - 1).
          ci == list_len(comps).
        ). ).
        ci == ci + 1.
      ). ).
      entities == list_set(entities, i, [eid, comps]).
      w == list_set(w, 1, entities).
      i == list_len(entities).
    ). ).
    i == i + 1.
  ). ).
  response(w).
).

ecs_destroy(w, eid.
  entities == list_get(w, 1).
  i == 0.
  While(i < list_len(entities) then(
    If(list_get(list_get(entities, i), 0) = eid then(
      entities == list_remove(entities, i).
      w == list_set(w, 1, entities).
      i == list_len(entities).
    ). ).
    i == i + 1.
  ). ).
  response(w).
).

ecs_query(w, comp.
  entities == list_get(w, 1).
  result == [].
  i == 0.
  While(i < list_len(entities) then(
    e == list_get(entities, i).
    comps == list_get(e, 1).
    ci == 0.
    found == 0.
    While(ci < list_len(comps) then(
      If(list_get(comps, ci) = comp then(
        found == 1.
        ci == list_len(comps).
      ). ).
      ci == ci + 1.
    ). ).
    If(found then(
      result == list_push(result, list_get(e, 0)).
    ). ).
    i == i + 1.
  ). ).
  response(result).
).

ecs_clear(w.
  w == list_set(w, 0, 0).
  w == list_set(w, 1, []).
  response(w).
).

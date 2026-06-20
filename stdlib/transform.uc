def transform_set_pos(eid, x, y.
  pos == entity_get(eid, "position").
  pos == list_set(pos, 0, x).
  pos == list_set(pos, 1, y).
  entity_set(eid, "position", list_get(pos, 0), list_get(pos, 1), list_get(pos, 2)).
).

def transform_set_z(eid, z.
  pos == entity_get(eid, "position").
  pos == list_set(pos, 2, z).
  entity_set(eid, "position", list_get(pos, 0), list_get(pos, 1), z).
).

def transform_get_x(eid.
  pos == entity_get(eid, "position").
  response(list_get(pos, 0)).
).

def transform_get_y(eid.
  pos == entity_get(eid, "position").
  response(list_get(pos, 1)).
).

def transform_get_z(eid.
  pos == entity_get(eid, "position").
  response(list_get(pos, 2)).
).

def transform_get_pos(eid.
  response(entity_get(eid, "position")).
).

def transform_set_rotation(eid, angle_deg.
  entity_set(eid, "rotation", angle_deg).
).

def transform_get_rotation(eid.
  rot == entity_get(eid, "rotation").
  response(list_get(rot, 0)).
).

def transform_set_scale(eid, sx, sy.
  entity_set(eid, "scale", sx, sy, 1.0).
).

def transform_get_scale(eid.
  response(entity_get(eid, "scale")).
).

def transform_translate(eid, dx, dy.
  px == transform_get_x(eid) + dx.
  py == transform_get_y(eid) + dy.
  transform_set_pos(eid, px, py).
).

def transform_look_at(from_eid, to_x, to_y.
  fx == transform_get_x(from_eid).
  fy == transform_get_y(from_eid).
  angle == Math.atan2(to_y - fy, to_x - fx) * 57.2958.
  transform_set_rotation(from_eid, angle).
).

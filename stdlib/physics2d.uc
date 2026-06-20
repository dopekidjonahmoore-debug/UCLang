physics2d_gravity_y == -9.8.

def physics2d_set_gravity(gy.
  physics2d_gravity_y == gy.
).

def physics2d_get_gravity(.
  response(physics2d_gravity_y).
).

def physics2d_add_velocity(eid, vx, vy.
  entity_set(eid, "velocity", vx, vy, 0.0).
).

def physics2d_set_ground(eid, ground_y.
  entity_set(eid, "ground_y", ground_y).
).

def physics2d_update(dt.
  eids == entity_all().
  i == 0.
  While(i < entity_count() then(
    eid == list_get(eids, i).
    can_skip == gameobject_is_active(eid).
    If(can_skip = 0 then(i == i + 1. ). else(
      If(entity_has(eid, "position") and entity_has(eid, "velocity") then(
        pos == entity_get(eid, "position").
        vel == entity_get(eid, "velocity").
        has_gravity == entity_has(eid, "gravity").
        If(has_gravity then(
          g == entity_get(eid, "gravity").
          gy == list_get(g, 0).
        ). else(
          gy == physics2d_gravity_y.
        ). ).
        vel == list_set(vel, 1, list_get(vel, 1) + gy * dt).
        pos == list_set(pos, 0, list_get(pos, 0) + list_get(vel, 0) * dt).
        pos == list_set(pos, 1, list_get(pos, 1) + list_get(vel, 1) * dt).
        pos == list_set(pos, 2, list_get(pos, 2) + list_get(vel, 2) * dt).
        If(entity_has(eid, "ground_y") then(
          gycomp == entity_get(eid, "ground_y").
          ground == list_get(gycomp, 0).
          pposy == list_get(pos, 1).
          If(pposy < ground then(
            pos == list_set(pos, 1, ground).
            vel == list_set(vel, 0, list_get(vel, 0) * 0.9).
            vel == list_set(vel, 1, 0.0).
            vel == list_set(vel, 2, 0.0).
          ). ).
        ). ).
        entity_set(eid, "position", list_get(pos, 0), list_get(pos, 1), list_get(pos, 2)).
        entity_set(eid, "velocity", list_get(vel, 0), list_get(vel, 1), list_get(vel, 2)).
      ). ).
      i == i + 1.
    ). ).
  ). ).
).

def physics2d_is_grounded(eid.
  If(entity_has(eid, "ground_y") then(
    gycomp == entity_get(eid, "ground_y").
    ground == list_get(gycomp, 0).
    pos == entity_get(eid, "position").
    py == list_get(pos, 1).
    vel == entity_get(eid, "velocity").
    If(py <= ground and Math.abs(list_get(vel, 1)) < 0.01 then(response(1).). ).
  ). ).
  response(0).
).

def physics2d_circle_collision(x1, y1, r1, x2, y2, r2.
  dx == x2 - x1.
  dy == y2 - y1.
  dist_sq == dx * dx + dy * dy.
  rad_sum == r1 + r2.
  response(dist_sq < rad_sum * rad_sum).
).

def physics2d_rect_collision(ax, ay, aw, ah, bx, by, bw, bh.
  response(ax < bx + bw and ax + aw > bx and ay < by + bh and ay + ah > by).
).

def pathfind_grid(w, h.
  grid == [].
  y == 0.
  While(y < h then(
    row == [].
    x == 0.
    While(x < w then(
      row == list_push(row, 0).
      x == x + 1.
    ). ).
    grid == list_push(grid, row).
    y == y + 1.
  ). ).
  response(grid).
).

def pathfind_set(grid, x, y, val.
  row == list_get(grid, y).
  grid == list_set(grid, y, list_set(row, x, val)).
  response(grid).
).

def pathfind_get(grid, x, y.
  response(list_get(list_get(grid, y), x)).
).

def pathfind_find(grid, sx, sy, ex, ey.
  w == list_len(list_get(grid, 0)).
  h == list_len(grid).
  visited == [].
  y == 0.
  While(y < h then(
    row == [].
    x == 0.
    While(x < w then(
      row == list_push(row, 0).
      x == x + 1.
    ). ).
    visited == list_push(visited, row).
    y == y + 1.
  ). ).
  row_v == list_get(visited, sy).
  visited == list_set(visited, sy, list_set(row_v, sx, 1)).
  parents == [[sx, sy, -1]].
  queue == [[sx, sy]].
  head == 0.
  found == 0.
  While(head < list_len(queue) and found = 0 then(
    cur == list_get(queue, head).
    cx == list_get(cur, 0).
    cy == list_get(cur, 1).
    If(cx > 0 then(
      If(list_get(list_get(grid, cy), cx - 1) = 0 then(
        If(list_get(list_get(visited, cy), cx - 1) = 0 then(
          queue == list_push(queue, [cx - 1, cy]).
          parents == list_push(parents, [cx - 1, cy, head]).
          row_v2 == list_get(visited, cy).
          visited == list_set(visited, cy, list_set(row_v2, cx - 1, 1)).
          If(cx - 1 = ex and cy = ey then(found == 1. ). ).
        ). ).
      ). ).
    ). ).
    If(cx < w - 1 then(
      If(list_get(list_get(grid, cy), cx + 1) = 0 then(
        If(list_get(list_get(visited, cy), cx + 1) = 0 then(
          queue == list_push(queue, [cx + 1, cy]).
          parents == list_push(parents, [cx + 1, cy, head]).
          row_v2 == list_get(visited, cy).
          visited == list_set(visited, cy, list_set(row_v2, cx + 1, 1)).
          If(cx + 1 = ex and cy = ey then(found == 1. ). ).
        ). ).
      ). ).
    ). ).
    If(cy > 0 then(
      If(list_get(list_get(grid, cy - 1), cx) = 0 then(
        If(list_get(list_get(visited, cy - 1), cx) = 0 then(
          queue == list_push(queue, [cx, cy - 1]).
          parents == list_push(parents, [cx, cy - 1, head]).
          row_v2 == list_get(visited, cy - 1).
          visited == list_set(visited, cy - 1, list_set(row_v2, cx, 1)).
          If(cx = ex and cy - 1 = ey then(found == 1. ). ).
        ). ).
      ). ).
    ). ).
    If(cy < h - 1 then(
      If(list_get(list_get(grid, cy + 1), cx) = 0 then(
        If(list_get(list_get(visited, cy + 1), cx) = 0 then(
          queue == list_push(queue, [cx, cy + 1]).
          parents == list_push(parents, [cx, cy + 1, head]).
          row_v2 == list_get(visited, cy + 1).
          visited == list_set(visited, cy + 1, list_set(row_v2, cx, 1)).
          If(cx = ex and cy + 1 = ey then(found == 1. ). ).
        ). ).
      ). ).
    ). ).
    head == head + 1.
  ). ).
  If(found = 0 then(response([]). ). ).
  ci == list_len(parents) - 1.
  path == [].
  While(ci >= 0 then(
    step == list_get(parents, ci).
    path == list_push(path, [list_get(step, 0), list_get(step, 1)]).
    ci == list_get(step, 2).
  ). ).
  rev == [].
  i == list_len(path) - 1.
  While(i >= 0 then(
    rev == list_push(rev, list_get(path, i)).
    i == i - 1.
  ). ).
  response(rev).
).

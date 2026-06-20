def particle_create(x, y, vx, vy, lifetime, color.
  response([x, y, vx, vy, lifetime, lifetime, color]).
).

def particle_update(p, dt.
  px == list_get(p, 0) + list_get(p, 2) * dt.
  py == list_get(p, 1) + list_get(p, 3) * dt.
  p == list_set(p, 0, px).
  p == list_set(p, 1, py).
  remaining == list_get(p, 4) - dt.
  p == list_set(p, 4, remaining).
  response(p).
).

def particle_draw(p.
  alpha == Math.clamp(list_get(p, 4) / list_get(p, 5), 0, 1).
  color == list_get(p, 6).
  Draw.circle(list_get(p, 0), list_get(p, 1), 3, color).
).

def particle_alive(p.
  response(list_get(p, 4) > 0).
).

def particle_burst(x, y, count, speed, lifetime, color.
  result == [].
  i == 0.
  While(i < count then(
    angle == Math.random() * 6.28318.
    vx == Math.cos(angle) * (Math.random() * speed * 0.5 + speed * 0.5).
    vy == Math.sin(angle) * (Math.random() * speed * 0.5 + speed * 0.5).
    p == particle_create(x, y, vx, vy, lifetime, color).
    result == list_push(result, p).
    i == i + 1.
  ). ).
  response(result).
).

def particle_system_new(max_particles.
  response([max_particles, []]).
).

def particle_system_update(ps, dt.
  particles == list_get(ps, 1).
  i == 0.
  count == list_len(particles).
  While(i < count then(
    p == list_get(particles, i).
    p == particle_update(p, dt).
    If(particle_alive(p) then(
      particles == list_set(particles, i, p).
      i == i + 1.
    ). else(
      particles == list_remove(particles, i).
      count == count - 1.
    ). ).
  ). ).
  ps == list_set(ps, 1, particles).
  response(ps).
).

def particle_system_draw(ps.
  particles == list_get(ps, 1).
  i == 0.
  While(i < list_len(particles) then(
    particle_draw(list_get(particles, i)).
    i == i + 1.
  ). ).
).

def particle_system_emit(ps, count, x, y, speed, lifetime, color.
  particles == list_get(ps, 1).
  max_p == list_get(ps, 0).
  burst == particle_burst(x, y, count, speed, lifetime, color).
  i == 0.
  While(i < list_len(burst) and list_len(particles) < max_p then(
    particles == list_push(particles, list_get(burst, i)).
    i == i + 1.
  ). ).
  ps == list_set(ps, 1, particles).
  response(ps).
).

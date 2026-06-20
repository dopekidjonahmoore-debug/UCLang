player == entity_create().
entity_set(player, "transform", 0.0, 1.0, 0.0, 0.0, 0.0, 0.0).
entity_set(player, "physics", 1.0, 0.0, 0.0, 0.0).

speed == 5.0.
dt == 0.016.

tp == entity_get(player, "transform").
px == tp[0].
py == tp[1].
pz == tp[2].

If(input.key_down("w") then(pz == pz - speed * dt). ).
If(input.key_down("s") then(pz == pz + speed * dt). ).

entity_set(player, "transform", px, py, pz, tp[3], tp[4], tp[5]).

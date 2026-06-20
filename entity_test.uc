# Entity system test
# Create entities, attach tagged components, read them back

e1 == entity_create().
print("e1 = " + int_to_str(e1)).
print("exists = " + int_to_str(entity_exists(e1))).

# Attach a transform component: x, y, z, rx, ry, rz
entity_set(e1, "transform", 10.0, 20.0, 30.0, 0.0, 0.0, 0.0).
print("has transform = " + int_to_str(entity_has(e1, "transform"))).

# Attach a physics component: mass, vx, vy, vz
entity_set(e1, "physics", 1.0, 0.0, 0.0, 0.0).

# Read back and verify
t == entity_get(e1, "transform").
print("x = " + int_to_str(t[0])).
print("y = " + int_to_str(t[1])).

# Create a second entity
e2 == entity_create().
entity_set(e2, "transform", 100.0, 200.0, 0.0, 0.0, 0.0, 0.0).

# List all entities
all == entity_all().
print("entity count = " + int_to_str(entity_count())).
print("e2 exists = " + int_to_str(entity_exists(e2))).

# Destroy e1 and verify
entity_destroy(e1).
print("after destroy e1 exists = " + int_to_str(entity_exists(e1))).

entity_clear().
print("after clear count = " + int_to_str(entity_count())).

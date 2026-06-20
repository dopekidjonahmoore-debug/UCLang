# Test Vec3
v == vec3(1, 2, 3).
print(v).
print("\n").
print(v.x).
print(" ").
print(v.y).
print(" ").
print(v.z).
print("\n").

# Test Vec3 arithmetic
v2 == v + vec3(10, 20, 30).
print(v2).
print("\n").

v3 == v * 2.
print(v3).
print("\n").

# Test Vec3 methods
len == v.length().
print("length = ").
print(len).
print("\n").

n == v.normalize().
print("normalized = ").
print(n).
print("\n").

d == v.dot(vec3(1, 0, 0)).
print("dot = ").
print(d).
print("\n").

# Test Quat
q == quat(0, 0, 0, 1).
print(q).
print("\n").
print(q.x).
print(" ").
print(q.y).
print(" ").
print(q.z).
print(" ").
print(q.w).
print("\n").

# Test Mat4
m == mat4().
print(m).
print("\n").

m2 == m.translate(1, 2, 3).
print("translated: ").
print(m2).
print("\n").

# Test Mat4 * Vec3 (transform position)
pos == vec3(1, 0, 0).
tm == mat4().
tm2 == tm.translate(5, 0, 0).
result == tm2 * pos.
print("transformed: ").
print(result).
print("\n").

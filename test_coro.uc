# Test coroutine system
# A coroutine function checks __yield_step to branch to the right yield point
def my_task(x.
  If(__yield_step = 0 then(
    print("step 0: ").
    print(x).
    print("\n").
    yield("first").
  ). ).
  If(__yield_step = 1 then(
    print("step 1: ").
    print(__resume_val).
    print("\n").
    yield("second").
  ). ).
  print("done\n").
  response("finished").
).

c == coro_start("my_task", 42).
print("resume 1\n").
r1 == coro_resume(c, "a").
print("yielded: ").
print(r1).
print("\n").

print("resume 2\n").
r2 == coro_resume(c, "b").
print("yielded: ").
print(r2).
print("\n").

print("resume 3\n").
r3 == coro_resume(c, "c").
print("result: ").
print(r3).
print("\n").

print("done? ").
print(coro_done(c)).
print("\n").

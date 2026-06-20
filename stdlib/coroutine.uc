# High-level coroutine helpers
# A coroutine function checks __yield_step to advance through states.
# Use coro_run_all() in your game loop to advance all active coroutines.

coro_list == [].

def coro_run(fn_name, args.
    c == coro_start(fn_name, args).
    coro_list == list_push(coro_list, [c, 0.0]).
    response(c).
).

def coro_run_all(dt.
    i == 0.
    While(i < list_len(coro_list) then(
        entry == list_get(coro_list, i).
        c == list_get(entry, 0).
        wait == list_get(entry, 1).
        If(wait > 0 then(
            entry == list_set(entry, 1, wait - dt).
        ). else(
            val == coro_resume(c, dt).
            entry == list_set(entry, 1, val).
            If(coro_done(c) then(
                coro_list == list_remove(coro_list, i).
                i == i - 1.
            ). ).
        ). ).
        i == i + 1.
    ). ).
).

def coro_wait_seconds(s.
    response(s).
).

def coro_wait_frames(.
    response(0.0).
).

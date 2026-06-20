# Event system - publish/subscribe pattern
# Usage:
#   ev == event_create("player_died").
#   event_sub(ev, "my_handler").
#   event_pub(ev, "player_1", 100).

def event_create(name.
    response([name, []]).
).

def event_sub(ev, handler_fn.
    handlers == list_get(ev, 1).
    handlers == list_push(handlers, handler_fn).
    ev == list_set(ev, 1, handlers).
    response(ev).
).

def event_unsub(ev, handler_fn.
    handlers == list_get(ev, 1).
    i == 0.
    While(i < list_len(handlers) then(
        If(list_get(handlers, i) = handler_fn then(
            handlers == list_remove(handlers, i).
            i == list_len(handlers).
        ). ).
        i == i + 1.
    ). ).
    ev == list_set(ev, 1, handlers).
    response(ev).
).

def event_pub(ev, args.
    event_name == list_get(ev, 0).
    handlers == list_get(ev, 1).
    i == 0.
    While(i < list_len(handlers) then(
        run.call_func(list_get(handlers, i), event_name, args).
        i == i + 1.
    ). ).
).

# Global event bus
__events == [].

def event_bus_create(name.
    e == event_create(name).
    __events == list_push(__events, e).
    response(e).
).

def event_bus_find(name.
    i == 0.
    While(i < list_len(__events) then(
        If(list_get(list_get(__events, i), 0) = name then(
            response(list_get(__events, i)).
        ). ).
        i == i + 1.
    ). ).
    response([]).
).

def event_bus_sub(name, handler_fn.
    ev == event_bus_find(name).
    If(list_len(ev) = 0 then(
        ev == event_bus_create(name).
    ). ).
    ev == event_sub(ev, handler_fn).
    # Store updated event back in __events
    i == 0.
    While(i < list_len(__events) then(
        If(list_get(list_get(__events, i), 0) = name then(
            __events == list_set(__events, i, ev).
            i == list_len(__events).
        ). ).
        i == i + 1.
    ). ).
).

def event_bus_pub(name, args.
    ev == event_bus_find(name).
    If(list_len(ev) > 0 then(
        event_pub(ev, args).
    ). ).
).

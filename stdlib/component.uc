# Component base class with lifecycle hooks
# Components extend this class and override lifecycle methods:
#   on_start()      - called when component is first activated
#   on_update(dt)   - called every frame with delta time
#   on_collision(other) - called on collision with another entity

class Component {
    entity_id == -1.
    active == 1.
    started == 0.

    def init(eid.
        this.entity_id == eid.
    )

    def start(.
        this.started == 1.
        this.on_start().
    )

    def update(dt.
        If(this.active and this.started then(
            this.on_update(dt).
        ). ).
    )

    def destroy(.
        this.active == 0.
        this.on_destroy().
    )

    def on_start(.)
    def on_update(dt.)
    def on_destroy(.)
    def on_collision(other.)
}

# Component system - manages all component instances
comp_registry == [].

def comp_register(comp.
    comp_registry == list_push(comp_registry, comp).
    comp.start().
    response(comp).
).

def comp_unregister(comp.
    comp.destroy().
    i == 0.
    While(i < list_len(comp_registry) then(
        If(list_get(comp_registry, i) = comp then(
            comp_registry == list_remove(comp_registry, i).
            i == list_len(comp_registry).
        ). ).
        i == i + 1.
    ). ).
).

def comp_update_all(dt.
    i == 0.
    While(i < list_len(comp_registry) then(
        list_get(comp_registry, i).update(dt).
        i == i + 1.
    ). ).
).

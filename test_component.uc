# Test component lifecycle + events
run_file("stdlib/events.uc").
run_file("stdlib/component.uc").

# Event test
print("=== Events ===\n").
def on_player_die(event_name, data.
    print("handler called: ").
    print(event_name).
    print("\n").
).

event_bus_sub("player_died", "on_player_die").
event_bus_pub("player_died", "test_data").
print("event pub done\n").

# Component test
class HealthComp extends Component {
    hp == 100.

    def on_start(.
        print("health comp started, hp=").
        print(this.hp).
        print("\n").
    )

    def on_update(dt.
        print("update dt=").
        print(dt).
        print("\n").
    )

    def take_damage(amount.
        this.hp == this.hp - amount.
    )
}

e == entity_create().
hc == new HealthComp(e).
print("registering component\n").
comp_register(hc).
print("done\n").

# Test entity builtins (existing)
eid == entity_create().
entity_set(eid, "test", 1, 2, 3).
print("entity exists: ").
print(entity_exists(eid)).
print("\n").
print("component count: ").
print(entity_count()).
print("\n").

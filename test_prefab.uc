run_file("stdlib/engine.uc").

prefab_register("Player", ["name", "Hero", "hp", 100, "x", 0, "y", 0]).
prefab_register("Enemy", ["name", "Goblin", "hp", 50, "x", 10, "y", 10]).

# Test scene save/load roundtrip
scene_entity_add("Player", []).
scene_entity_add("Enemy", ["hp", 999]).
scene_entity_add("Player", ["hp", 300, "y", 99]).
print(list_len(scene_get_entities()) + 0). print("\n").

scene_save("test_scene.json").
scene_clear(.
scene_load("test_scene.json").

# Verify restore count
ents == scene_get_entities().
print(list_len(ents) + 0). print("\n").

# Instantiate all
instances == scene_instantiate().
print(list_len(instances) + 0). print("\n").

# Verify values
print(instances[0]). print("\n").
print(instances[1]). print("\n").
print(instances[2]). print("\n").

print("DONE\n").

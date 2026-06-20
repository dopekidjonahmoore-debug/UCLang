# Skeletal Animation test
run_file("stdlib/engine.uc").

# Create bones: [name, parent_idx, pos, rot, scale]
hip_bone == animator_make_bone("hip", -1, vec3(0,0,0), quat(0,0,0,1), vec3(1,1,1)).
leg_bone == animator_make_bone("leg", 0, vec3(0,-1,0), quat(0,0,0,1), vec3(1,1,1)).
foot_bone == animator_make_bone("foot", 1, vec3(0,-1,0), quat(0,0,0,1), vec3(1,1,1)).
bones == [hip_bone, leg_bone, foot_bone].

# Create keyframes: [time, pos, rot, scale]
kf1 == animator_make_keyframe(0, vec3(0,0,0), quat(0,0,0,1), vec3(1,1,1)).
kf2 == animator_make_keyframe(1, vec3(0,0,0), quat.euler(45,0,0), vec3(1,1,1)).
kf3 == animator_make_keyframe(0, vec3(0,-1,0), quat(0,0,0,1), vec3(1,1,1)).
kf4 == animator_make_keyframe(1, vec3(0,-1,0), quat.euler(-45,0,0), vec3(1,1,1)).
kf5 == animator_make_keyframe(0, vec3(0,-1,0), quat(0,0,0,1), vec3(1,1,1)).
kf6 == animator_make_keyframe(1, vec3(0,-1,0), quat.euler(-30,0,0), vec3(1,1,1)).

# Create clips: [name, duration, loop, bone_keyframes]
walk_clip == animator_make_clip("walk", 1, 1, [[kf1, kf2], [kf3, kf4], [kf5, kf6]]).

# Create states: [name, clip_name, speed]
idle_state == animator_make_state("idle", "walk", 1).

# Create transitions: [from, to, param, value, blend]
no_trans == animator_make_transition("", "", "", 0, 0).
transitions == [no_trans].

# Build animator
anim_id == animator_create(bones, [walk_clip], [idle_state], transitions).
print("animator id: "). print(anim_id + 0). print("\n").

# Check initial state
state == animator_get_state(anim_id).
print("state: "). print(state). print("\n").

# Update a few frames
animator_update(anim_id, 0.25).
bones_final == animator_get_bone_matrices(anim_id).
print("bone matrices count: "). print(list_len(bones_final) + 0). print("\n").

# Get local transforms
local0 == animator_get_bone_local(anim_id, 0).
local1 == animator_get_bone_local(anim_id, 1).
print("bone0 local: "). print(local0). print("\n").
print("bone1 local: "). print(local1). print("\n").

# Destroy
animator_destroy(anim_id).
print("ANIMATION TEST PASSED\n").

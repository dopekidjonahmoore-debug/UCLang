# ─── Animator System ──────────────────────────────────────────
#  High-level wrapper around animation builtins.
#  API:
#    animator = Animator(bones, clips, states, transitions.)
#    animator:update(dt.)
#    animator:set_param(name, val.)
#    animator:get_param(name.) -> val
#    animator:get_state(.) -> string
#    animator:play(state_name.)
#    animator:get_bone_matrices(.) -> [mat4, ...]
#    animator:get_bone_local(index.) -> [pos, rot, scale]
#    animator:destroy(.)
#
#  Data format helpers:
#    animator_make_bone(name, parent_idx, pos, rot, scale.)
#    animator_make_keyframe(time, pos, rot, scale.)
#    animator_make_clip(name, duration, loop, bone_keyframes.)
#    animator_make_state(name, clip_name, speed.)
#    animator_make_transition(from, to, param, value, blend.)
# ────────────────────────────────────────────────────────────────

class Animator extends Component {
  __anim_id == 0.
  __bones == [].
  __clips == [].
  __states == [].
  __transitions == [].

  def init(bones, clips, states, transitions.
    super.init().
    __bones == bones.
    __clips == clips.
    __states == states.
    __transitions == transitions.
    __anim_id == animator_create(bones, clips, states, transitions).
  ).

  def update(dt.
    animator_update(__anim_id, dt).
  ).

  def set_param(name, val.
    animator_set_param(__anim_id, name, val).
  ).

  def get_param(name.
    response(animator_get_param(__anim_id, name)).
  ).

  def get_state(.
    response(animator_get_state(__anim_id)).
  ).

  def play(state_name.
    animator_play(__anim_id, state_name).
  ).

  def get_bone_matrices(.
    response(animator_get_bone_matrices(__anim_id)).
  ).

  def get_bone_local(index.
    response(animator_get_bone_local(__anim_id, index)).
  ).

  def destroy(.
    animator_destroy(__anim_id).
  ).
}.

# ─── Data construction helpers ───
def animator_make_bone(name, parent_idx, pos, rot, scale.
  response([name, parent_idx, pos, rot, scale]).
).

def animator_make_keyframe(time, pos, rot, scale.
  response([time, pos, rot, scale]).
).

def animator_make_clip(name, duration, loop, bone_keyframes.
  response([name, duration, loop, bone_keyframes]).
).

def animator_make_state(name, clip_name, speed.
  response([name, clip_name, speed]).
).

def animator_make_transition(from, to, param, value, blend.
  response([from, to, param, value, blend]).
).

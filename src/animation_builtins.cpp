#include "animation_builtins.h"
#include "interpreter.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace UCLang {

// ─── Bone ───
struct BoneData {
    std::string name;
    int parentIndex; // -1 for root
    glm::vec3 localPos;
    glm::quat localRot;
    glm::vec3 localScale;
};

// ─── Keyframe ───
struct Keyframe {
    double time;
    glm::vec3 pos;
    glm::quat rot;
    glm::vec3 scale;
};

// ─── Animation Clip ───
struct AnimClip {
    std::string name;
    double duration;
    std::vector<std::vector<Keyframe>> boneKeyframes; // [boneIndex][keyframeIndex]
    bool loop;
};

// ─── Animator State ───
struct AnimState {
    std::string name;
    std::string clipName;
    double speed;
};

// ─── Animator Transition ───
struct AnimTransition {
    std::string from;
    std::string to;
    std::string conditionParam;
    double conditionValue; // param > value triggers transition
    double blendTime;
};

// ─── Animator ───
struct AnimatorData {
    std::vector<BoneData> bones;
    std::vector<AnimClip> clips;
    std::vector<AnimState> states;
    std::vector<AnimTransition> transitions;
    std::string currentState;
    std::string nextState;
    double stateTime;
    double blendTime;
    double blendProgress;
    std::vector<glm::vec3> currentPoses; // interpolated bone positions
    std::vector<glm::quat> currentRots;  // interpolated bone rotations
    std::vector<glm::vec3> currentScales;
    std::map<std::string, double> parameters;
    bool initialized;
};

static std::map<int, std::shared_ptr<AnimatorData>> g_animators;
static int g_nextAnimatorId = 1;

// ─── Interpolation helpers ───
static Keyframe lerpKeyframe(const Keyframe& a, const Keyframe& b, double t) {
    Keyframe r;
    t = std::max(0.0, std::min(1.0, t));
    r.time = a.time + (b.time - a.time) * t;
    r.pos = a.pos + (b.pos - a.pos) * (float)t;
    r.rot = glm::slerp(a.rot, b.rot, (float)t);
    r.scale = a.scale + (b.scale - a.scale) * (float)t;
    return r;
}

static void sampleClip(const AnimClip& clip, double time, std::vector<glm::vec3>& outPos,
    std::vector<glm::quat>& outRot, std::vector<glm::vec3>& outScale)
{
    double t = clip.loop ? std::fmod(time, clip.duration) : std::min(time, clip.duration);
    for (size_t b = 0; b < clip.boneKeyframes.size(); ++b) {
        const auto& frames = clip.boneKeyframes[b];
        if (frames.empty()) { outPos[b] = {0,0,0}; outRot[b] = glm::quat(1,0,0,0); outScale[b] = {1,1,1}; continue; }
        if (frames.size() == 1) { outPos[b] = frames[0].pos; outRot[b] = frames[0].rot; outScale[b] = frames[0].scale; continue; }
        // Find surrounding keyframes
        size_t i = 0;
        for (; i + 1 < frames.size() && frames[i + 1].time < t; ++i);
        if (i + 1 >= frames.size()) {
            outPos[b] = frames[i].pos; outRot[b] = frames[i].rot; outScale[b] = frames[i].scale;
        } else {
            double segDur = frames[i+1].time - frames[i].time;
            double frac = segDur > 0 ? (t - frames[i].time) / segDur : 0;
            Keyframe kf = lerpKeyframe(frames[i], frames[i+1], frac);
            outPos[b] = kf.pos; outRot[b] = kf.rot; outScale[b] = kf.scale;
        }
    }
}

static void buildBoneMatrices(const std::vector<BoneData>& bones,
    const std::vector<glm::vec3>& posedPos,
    const std::vector<glm::quat>& posedRot,
    const std::vector<glm::vec3>& posedScale,
    std::vector<glm::mat4>& outMatrices)
{
    outMatrices.resize(bones.size());
    for (size_t i = 0; i < bones.size(); ++i) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), posedPos[i]) *
                          glm::mat4_cast(posedRot[i]) *
                          glm::scale(glm::mat4(1.0f), posedScale[i]);
        if (bones[i].parentIndex >= 0 && bones[i].parentIndex < (int)i)
            outMatrices[i] = outMatrices[bones[i].parentIndex] * local;
        else
            outMatrices[i] = local;
    }
}

// ─── Native function helpers ───
static double gv(const Value& v) {
    if (auto* d = std::get_if<double>(&v)) return *d;
    if (auto* i = std::get_if<int64_t>(&v)) return (double)*i;
    return 0.0;
}

static ValueList extractVec3List(const Value& val) {
    ValueList result;
    auto* list = std::get_if<ValueList>(&val);
    if (!list) return result;
    for (const auto& e : *list) {
        if (auto* v = std::get_if<Vec3>(&e)) {
            result.push_back(Value(Vec3(v->x, v->y, v->z)));
        } else {
            result.push_back(e);
        }
    }
    return result;
}

// ─── Register builtins ───
void register_animation_natives(std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m) {
    // animator_create(bones_json, clips_json, states_json, transitions_json) -> id
    // bones_json: [ {name,parent,px,py,pz,rx,ry,rz,rw,sx,sy,sz}, ... ]
    // clips_json: [ {name,duration,loop,boneKeyframes:[[{t,px,py,pz,qx,qy,qz,qw,sx,sy,sz},...],...]}, ... ]
    // states_json: [ {name,clipName,speed}, ... ]
    // transitions_json: [ {from,to,conditionParam,conditionValue,blendTime}, ... ]
    m["animator_create"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 1) throw std::runtime_error("animator_create(bones, clips, states, transitions): need at least bones");
        auto* bonesList = std::get_if<ValueList>(&args[0]);
        if (!bonesList) throw std::runtime_error("animator_create: bones must be a list");

        auto anim = std::make_shared<AnimatorData>();
        anim->initialized = false;

        // Parse bones
        for (const auto& bv : *bonesList) {
            auto* b = std::get_if<ValueList>(&bv);
            if (!b || b->size() < 3) continue;
            BoneData bone;
            auto* name = std::get_if<std::string>(&(*b)[0]);
            bone.name = name ? *name : "bone";
            bone.parentIndex = (int)gv((*b)[1]);
            auto* pos = std::get_if<Vec3>(&(*b)[2]); if (pos) bone.localPos = *pos;
            if (b->size() > 3) { auto* r = std::get_if<Quat>(&(*b)[3]); if (r) bone.localRot = *r; }
            if (b->size() > 4) { auto* s = std::get_if<Vec3>(&(*b)[4]); if (s) bone.localScale = *s; }
            anim->bones.push_back(bone);
        }

        // Parse clips
        if (args.size() > 1) {
            auto* clipsList = std::get_if<ValueList>(&args[1]);
            if (clipsList) {
                for (const auto& cv : *clipsList) {
                    auto* c = std::get_if<ValueList>(&cv);
                    if (!c || c->size() < 4) continue;
                    AnimClip clip;
                    auto* name = std::get_if<std::string>(&(*c)[0]); clip.name = name ? *name : "clip";
                    clip.duration = gv((*c)[1]);
                    clip.loop = (int)gv((*c)[2]) != 0;
                    auto* kfList = std::get_if<ValueList>(&(*c)[3]);
                    if (kfList) {
                        for (const auto& kfv : *kfList) {
                            auto* boneKfs = std::get_if<ValueList>(&kfv);
                            if (!boneKfs) continue;
                            std::vector<Keyframe> frames;
            for (const auto& fv : *boneKfs) {
                                    auto* f = std::get_if<ValueList>(&fv);
                                    if (!f || f->size() < 4) continue;
                                Keyframe kf;
                                kf.time = gv((*f)[0]);
                                auto* p = std::get_if<Vec3>(&(*f)[1]); if (p) kf.pos = *p;
                                auto* q = std::get_if<Quat>(&(*f)[2]); if (q) kf.rot = *q;
                                auto* s = std::get_if<Vec3>(&(*f)[3]); if (s) kf.scale = *s;
                                frames.push_back(kf);
                            }
                            clip.boneKeyframes.push_back(frames);
                        }
                    }
                    anim->clips.push_back(clip);
                }
            }
        }

        // Parse states
        if (args.size() > 2) {
            auto* statesList = std::get_if<ValueList>(&args[2]);
            if (statesList) {
                for (const auto& sv : *statesList) {
                    auto* s = std::get_if<ValueList>(&sv);
                    if (!s || s->size() < 2) continue;
                    AnimState st;
                    auto* name = std::get_if<std::string>(&(*s)[0]); st.name = name ? *name : "state";
                    auto* cn = std::get_if<std::string>(&(*s)[1]); st.clipName = cn ? *cn : "";
                    st.speed = s->size() > 2 ? gv((*s)[2]) : 1.0;
                    anim->states.push_back(st);
                }
            }
        }

        // Parse transitions
        if (args.size() > 3) {
            auto* transList = std::get_if<ValueList>(&args[3]);
            if (transList) {
                for (const auto& tv : *transList) {
                    auto* t = std::get_if<ValueList>(&tv);
                    if (!t || t->size() < 3) continue;
                    AnimTransition tr;
                    auto* fr = std::get_if<std::string>(&(*t)[0]); tr.from = fr ? *fr : "";
                    auto* to = std::get_if<std::string>(&(*t)[1]); tr.to = to ? *to : "";
                    auto* cp = std::get_if<std::string>(&(*t)[2]); tr.conditionParam = cp ? *cp : "";
                    tr.conditionValue = t->size() > 3 ? gv((*t)[3]) : 0.0;
                    tr.blendTime = t->size() > 4 ? gv((*t)[4]) : 0.2;
                    anim->transitions.push_back(tr);
                }
            }
        }

        // Set initial state
        if (!anim->states.empty()) {
            anim->currentState = anim->states[0].name;
            anim->stateTime = 0;
            anim->nextState = "";
            anim->blendTime = 0;
            anim->blendProgress = 1.0;
        }

        int id = g_nextAnimatorId++;
        g_animators[id] = anim;
        return Value((int64_t)id);
    };

    // animator_set_param(id, name, value)
    m["animator_set_param"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 3) throw std::runtime_error("animator_set_param(id,name,value): need 3");
        int id = (int)gv(args[0]);
        auto* name = std::get_if<std::string>(&args[1]);
        if (!name) throw std::runtime_error("animator_set_param: name must be string");
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return std::monostate{};
        it->second->parameters[*name] = gv(args[2]);
        return std::monostate{};
    };

    // animator_get_param(id, name) -> value
    m["animator_get_param"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("animator_get_param(id,name): need 2");
        int id = (int)gv(args[0]);
        auto* name = std::get_if<std::string>(&args[1]);
        if (!name) throw std::runtime_error("animator_get_param: name must be string");
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return Value(0.0);
        auto pit = it->second->parameters.find(*name);
        if (pit == it->second->parameters.end()) return Value(0.0);
        return Value(pit->second);
    };

    // animator_update(id, dt)
    m["animator_update"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("animator_update(id,dt): need 2");
        int id = (int)gv(args[0]);
        double dt = gv(args[1]);
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return std::monostate{};

        auto& anim = it->second;
        if (anim->states.empty()) return std::monostate{};

        // Check transitions
        if (anim->nextState.empty()) {
            for (const auto& tr : anim->transitions) {
                if (tr.from != anim->currentState) continue;
                auto pit = anim->parameters.find(tr.conditionParam);
                if (pit != anim->parameters.end() && pit->second > tr.conditionValue) {
                    anim->nextState = tr.to;
                    anim->blendTime = tr.blendTime;
                    anim->blendProgress = 0.0;
                    break;
                }
            }
        }

        // Advance blend
        if (!anim->nextState.empty()) {
            anim->blendProgress += dt / std::max(anim->blendTime, 0.001);
            if (anim->blendProgress >= 1.0) {
                anim->currentState = anim->nextState;
                anim->nextState = "";
                anim->stateTime = 0;
                anim->blendProgress = 1.0;
            }
        }

        anim->stateTime += dt;

        // Sample current clip
        const AnimClip* curClip = nullptr;
        for (const auto& s : anim->states) {
            if (s.name == anim->currentState) {
                for (const auto& c : anim->clips) {
                    if (c.name == s.clipName) { curClip = &c; break; }
                }
                break;
            }
        }

        const AnimClip* nextClip = nullptr;
        if (!anim->nextState.empty()) {
            for (const auto& s : anim->states) {
                if (s.name == anim->nextState) {
                    for (const auto& c : anim->clips) {
                        if (c.name == s.clipName) { nextClip = &c; break; }
                    }
                    break;
                }
            }
        }

        size_t boneCount = anim->bones.size();
        std::vector<glm::vec3> curPos(boneCount), nxtPos(boneCount);
        std::vector<glm::quat> curRot(boneCount), nxtRot(boneCount);
        std::vector<glm::vec3> curScale(boneCount), nxtScale(boneCount);

        if (curClip) {
            sampleClip(*curClip, anim->stateTime, curPos, curRot, curScale);
        } else {
            for (size_t i = 0; i < boneCount; ++i) {
                curPos[i] = anim->bones[i].localPos;
                curRot[i] = anim->bones[i].localRot;
                curScale[i] = anim->bones[i].localScale;
            }
        }

        if (nextClip) {
            double nextTime = anim->blendProgress < 1.0 ? anim->blendProgress * anim->blendTime : 0;
            sampleClip(*nextClip, nextTime, nxtPos, nxtRot, nxtScale);
        }

        // Blend between current and next
        float blend = (float)std::max(0.0, std::min(1.0, anim->blendProgress));
        anim->currentPoses.resize(boneCount);
        anim->currentRots.resize(boneCount);
        anim->currentScales.resize(boneCount);
        for (size_t i = 0; i < boneCount; ++i) {
            if (nextClip && blend < 1.0f) {
                anim->currentPoses[i] = curPos[i] + (nxtPos[i] - curPos[i]) * blend;
                anim->currentRots[i] = glm::slerp(curRot[i], nxtRot[i], blend);
                anim->currentScales[i] = curScale[i] + (nxtScale[i] - curScale[i]) * blend;
            } else {
                anim->currentPoses[i] = curPos[i];
                anim->currentRots[i] = curRot[i];
                anim->currentScales[i] = curScale[i];
            }
        }

        anim->initialized = true;
        return std::monostate{};
    };

    // animator_get_bone_matrices(id) -> list of mat4 (world-space bone matrices)
    m["animator_get_bone_matrices"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("animator_get_bone_matrices(id): need id");
        int id = (int)gv(args[0]);
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return ValueList{};

        auto& anim = it->second;
        if (!anim->initialized) return ValueList{};

        std::vector<glm::mat4> matrices;
        buildBoneMatrices(anim->bones, anim->currentPoses, anim->currentRots, anim->currentScales, matrices);

        ValueList result;
        for (const auto& m : matrices)
            result.push_back(Value(m));
        return Value(result);
    };

    // animator_get_state(id) -> current state name
    m["animator_get_state"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("animator_get_state(id): need id");
        int id = (int)gv(args[0]);
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return Value(std::string(""));
        return Value(it->second->currentState);
    };

    // animator_play(id, state_name) — force state transition
    m["animator_play"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("animator_play(id,state): need 2");
        int id = (int)gv(args[0]);
        auto* name = std::get_if<std::string>(&args[1]);
        if (!name) throw std::runtime_error("animator_play: state must be string");
        auto it = g_animators.find(id);
        if (it == g_animators.end()) return std::monostate{};
        it->second->nextState = *name;
        it->second->blendTime = 0.1;
        it->second->blendProgress = 0.0;
        return std::monostate{};
    };

    // animator_get_bone_local(id, boneIndex) -> [pos_vec3, rot_quat, scale_vec3]
    m["animator_get_bone_local"] = [](const std::vector<Value>& args) -> Value {
        if (args.size() < 2) throw std::runtime_error("animator_get_bone_local(id, boneIdx): need 2");
        int id = (int)gv(args[0]);
        int boneIdx = (int)gv(args[1]);
        auto it = g_animators.find(id);
        if (it == g_animators.end() || boneIdx < 0 || boneIdx >= (int)it->second->bones.size())
            return ValueList{};
        auto& anim = it->second;
        ValueList r;
        r.push_back(Value(anim->currentPoses[boneIdx]));
        r.push_back(Value(anim->currentRots[boneIdx]));
        r.push_back(Value(anim->currentScales[boneIdx]));
        return Value(r);
    };

    // animator_destroy(id)
    m["animator_destroy"] = [](const std::vector<Value>& args) -> Value {
        if (args.empty()) throw std::runtime_error("animator_destroy(id): need id");
        int id = (int)gv(args[0]);
        g_animators.erase(id);
        return std::monostate{};
    };
}

} // namespace UCLang
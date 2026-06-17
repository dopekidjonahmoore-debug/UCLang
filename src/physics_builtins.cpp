#include "physics_builtins.h"
#include "interpreter.h"
#include <cmath>
#include <map>
#include <string>
#include <stdexcept>

namespace UCLang {

struct PhysicsBody {
    double x,y,z, vx,vy,vz, mass;
    double w,h,d, radius;
    bool isBox;
};
static std::map<std::string, PhysicsBody> g_bodies;
static double g_gravityX=0, g_gravityY=-9.8, g_gravityZ=0;

void register_physics_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    m["physics.set_gravity"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()>=3){
            auto gv=[](const Value& v)->double{if(auto*i=std::get_if<int64_t>(&v))return(double)*i;if(auto*f=std::get_if<double>(&v))return*f;return 0.0;};
            g_gravityX=gv(args[0]); g_gravityY=gv(args[1]); g_gravityZ=gv(args[2]);
        }
        return std::monostate{};
    };
    m["physics.add_body"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.add_body(name,x,y,z,mass): need 5");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_body: name must be string");
        auto gv=[](const Value& v)->double{if(auto*i=std::get_if<int64_t>(&v))return(double)*i;if(auto*f=std::get_if<double>(&v))return*f;return 0.0;};
        PhysicsBody b;
        b.x=gv(args[1]); b.y=gv(args[2]); b.z=args.size()>2?gv(args[2]):0;
        // Actually args are: name, x, y, z, mass
        if(args.size()>=4) b.z=gv(args[3]);
        if(args.size()>=5) b.mass=gv(args[4]); else b.mass=1.0;
        b.vx=b.vy=b.vz=0; b.w=b.h=b.d=1; b.radius=0.5; b.isBox=true;
        g_bodies[*name]=b;
        return std::monostate{};
    };
    m["physics.add_box_collider"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.add_box_collider(name,w,h,d): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_box_collider: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
        auto gv=[](const Value& v)->double{if(auto*i=std::get_if<int64_t>(&v))return(double)*i;if(auto*f=std::get_if<double>(&v))return*f;return 0.0;};
        it->second.w=gv(args[1]); it->second.h=gv(args[2]); it->second.d=args.size()>3?gv(args[3]):gv(args[1]);
        it->second.isBox=true;
        return std::monostate{};
    };
    m["physics.add_sphere_collider"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Physics.add_sphere_collider(name,radius): need 2");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_sphere_collider: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
        auto* rp=std::get_if<double>(&args[1]); if(!rp){auto*ri=std::get_if<int64_t>(&args[1]);if(ri)it->second.radius=(double)*ri;}
        else it->second.radius=*rp;
        it->second.isBox=false;
        return std::monostate{};
    };
    m["physics.set_velocity"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.set_velocity(name,vx,vy,vz): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.set_velocity: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
        auto gv=[](const Value& v)->double{if(auto*i=std::get_if<int64_t>(&v))return(double)*i;if(auto*f=std::get_if<double>(&v))return*f;return 0.0;};
        it->second.vx=gv(args[1]); it->second.vy=gv(args[2]); it->second.vz=gv(args[3]);
        return std::monostate{};
    };
    m["physics.get_pos"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Physics.get_pos(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.get_pos: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()){ValueList vl;vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));return Value(vl);}
        ValueList r1;r1.push_back(Value(it->second.x));r1.push_back(Value(it->second.y));r1.push_back(Value(it->second.z));return Value(r1);
    };
    m["physics.get_velocity"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Physics.get_velocity(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.get_velocity: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()){ValueList vl;vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));return Value(vl);}
        ValueList r2;r2.push_back(Value(it->second.vx));r2.push_back(Value(it->second.vy));r2.push_back(Value(it->second.vz));return Value(r2);
    };
    m["physics.apply_force"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.apply_force(name,fx,fy,fz): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.apply_force: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
        auto gv=[](const Value& v)->double{if(auto*i=std::get_if<int64_t>(&v))return(double)*i;if(auto*f=std::get_if<double>(&v))return*f;return 0.0;};
        double fx=gv(args[1]), fy=gv(args[2]), fz=gv(args[3]);
        if(it->second.mass>0){
            it->second.vx+=fx/it->second.mass;
            it->second.vy+=fy/it->second.mass;
            it->second.vz+=fz/it->second.mass;
        }
        return std::monostate{};
    };
    m["physics.step"] = [](const std::vector<Value>& args) -> Value {
        double dt=0.016;
        if(!args.empty()){auto*i=std::get_if<double>(&args[0]);if(i)dt=*i;else{auto*ii=std::get_if<int64_t>(&args[0]);if(ii)dt=(double)*ii;}}
        for(auto& kv : g_bodies){
            auto& b=kv.second;
            b.vx+=g_gravityX*dt;
            b.vy+=g_gravityY*dt;
            b.vz+=g_gravityZ*dt;
            b.x+=b.vx*dt;
            b.y+=b.vy*dt;
            b.z+=b.vz*dt;
        }
        // Simple ground plane collision
        for(auto& kv : g_bodies){
            auto& b=kv.second;
            if(b.y<0){b.y=0;b.vy=-b.vy*0.5;}
        }
        return std::monostate{};
    };
}

} // namespace UCLang

#include "math_builtins.h"
#include "interpreter.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <stdexcept>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace UCLang {

static std::mt19937& rng() {
    static std::mt19937 gen((unsigned int)time(nullptr));
    return gen;
}

void register_math_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    // ═══ Math ════════════════════════════════════════════════
    m["math.sin"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::sin(v));
    };
    m["math.cos"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::cos(v));
    };
    m["math.tan"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::tan(v));
    };
    m["math.asin"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::asin(v));
    };
    m["math.acos"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::acos(v));
    };
    m["math.atan2"] = [](const std::vector<Value>& args) -> Value {
        double y=0,x=0;
        if(args.size()>=2){
            auto*yp=std::get_if<double>(&args[0]);if(!yp){auto*yi=std::get_if<int64_t>(&args[0]);if(yi)y=(double)*yi;}else y=*yp;
            auto*xp=std::get_if<double>(&args[1]);if(!xp){auto*xi=std::get_if<int64_t>(&args[1]);if(xi)x=(double)*xi;}else x=*xp;
        }
        return Value(std::atan2(y,x));
    };
    m["math.abs"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) return Value((int64_t)0);
        auto*i=std::get_if<int64_t>(&args[0]);
        if(i) return Value(*i<0?-*i:*i);
        auto*f=std::get_if<double>(&args[0]);
        if(f) return Value(*f<0?-*f:*f);
        return Value((int64_t)0);
    };
    m["math.floor"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value((int64_t)std::floor(v));
    };
    m["math.ceil"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value((int64_t)std::ceil(v));
    };
    m["math.round"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value((int64_t)std::round(v));
    };
    m["math.sqrt"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(std::sqrt(v));
    };
    m["math.pow"] = [](const std::vector<Value>& args) -> Value {
        double base=0,exp=0;
        if(args.size()>=2){
            {auto*i=std::get_if<int64_t>(&args[0]);if(i)base=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)base=*f;}}
            {auto*i=std::get_if<int64_t>(&args[1]);if(i)exp=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)exp=*f;}}
        }
        return Value(std::pow(base,exp));
    };
    m["math.clamp"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) return Value((int64_t)0);
        double v=0,min=0,max=0;
        {auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        {auto*i=std::get_if<int64_t>(&args[1]);if(i)min=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)min=*f;}}
        {auto*i=std::get_if<int64_t>(&args[2]);if(i)max=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)max=*f;}}
        if(v<min)v=min;if(v>max)v=max;
        // Preserve int if all inputs were ints
        if(std::get_if<int64_t>(&args[0])&&std::get_if<int64_t>(&args[1])&&std::get_if<int64_t>(&args[2]))
            return Value((int64_t)v);
        return Value(v);
    };
    m["math.lerp"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<3) return Value(0.0);
        double a=0,b=0,t=0;
        {auto*i=std::get_if<int64_t>(&args[0]);if(i)a=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)a=*f;}}
        {auto*i=std::get_if<int64_t>(&args[1]);if(i)b=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)b=*f;}}
        {auto*i=std::get_if<int64_t>(&args[2]);if(i)t=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)t=*f;}}
        return Value(a+(b-a)*t);
    };
    m["math.random"] = [](const std::vector<Value>& args) -> Value {
        double min=0,max=1;
        if(args.size()>=2){
            {auto*i=std::get_if<int64_t>(&args[0]);if(i)min=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)min=*f;}}
            {auto*i=std::get_if<int64_t>(&args[1]);if(i)max=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)max=*f;}}
        }
        std::uniform_real_distribution<double> dist(min,max);
        return Value(dist(rng()));
    };
    m["math.random_int"] = [](const std::vector<Value>& args) -> Value {
        int min=0,max=100;
        if(args.size()>=2){
            auto*mi=std::get_if<int64_t>(&args[0]);if(mi)min=(int)*mi;
            auto*xi=std::get_if<int64_t>(&args[1]);if(xi)max=(int)*xi;
        }
        std::uniform_int_distribution<int> dist(min,max);
        return Value((int64_t)dist(rng()));
    };
    m["math.pi"] = [](const std::vector<Value>&) -> Value {
        return Value(3.14159265358979323846);
    };
    m["math.deg"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(v*180.0/3.14159265358979323846);
    };
    m["math.rad"] = [](const std::vector<Value>& args) -> Value {
        double v=0; if(!args.empty()){auto*i=std::get_if<int64_t>(&args[0]);if(i)v=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)v=*f;}}
        return Value(v*3.14159265358979323846/180.0);
    };
    m["math.distance"] = [](const std::vector<Value>& args) -> Value {
        double x1=0,y1=0,x2=0,y2=0;
        if(args.size()>=4){
            {auto*i=std::get_if<int64_t>(&args[0]);if(i)x1=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x1=*f;}}
            {auto*i=std::get_if<int64_t>(&args[1]);if(i)y1=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y1=*f;}}
            {auto*i=std::get_if<int64_t>(&args[2]);if(i)x2=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)x2=*f;}}
            {auto*i=std::get_if<int64_t>(&args[3]);if(i)y2=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)y2=*f;}}
        }
        double dx=x2-x1, dy=y2-y1;
        return Value(std::sqrt(dx*dx+dy*dy));
    };
    m["math.normalize"] = [](const std::vector<Value>& args) -> Value {
        double x=0,y=0;
        if(args.size()>=2){
            auto*xi=std::get_if<int64_t>(&args[0]);if(xi)x=(double)*xi;else{auto*xf=std::get_if<double>(&args[0]);if(xf)x=*xf;}
            auto*yi=std::get_if<int64_t>(&args[1]);if(yi)y=(double)*yi;else{auto*yf=std::get_if<double>(&args[1]);if(yf)y=*yf;}
        }
        double len=std::sqrt(x*x+y*y);
        if(len==0){ValueList vl;vl.push_back(Value((int64_t)0));vl.push_back(Value((int64_t)0));return Value(vl);}
        ValueList vl;vl.push_back(Value(x/len));vl.push_back(Value(y/len));return Value(vl);
    };
    m["math.dot"] = [](const std::vector<Value>& args) -> Value {
        double x1=0,y1=0,x2=0,y2=0;
        if(args.size()>=4){
            {auto*i=std::get_if<int64_t>(&args[0]);if(i)x1=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x1=*f;}}
            {auto*i=std::get_if<int64_t>(&args[1]);if(i)y1=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y1=*f;}}
            {auto*i=std::get_if<int64_t>(&args[2]);if(i)x2=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)x2=*f;}}
            {auto*i=std::get_if<int64_t>(&args[3]);if(i)y2=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)y2=*f;}}
        }
        return Value(x1*x2+y1*y2);
    };

    // ═══ Collision 2D ═══════════════════════════════════════
    m["rect.overlap"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<8) return Value(false);
        auto gv=[](const Value& v)->int64_t{auto*i=std::get_if<int64_t>(&v);return i?*i:0;};
        int64_t x1=gv(args[0]),y1=gv(args[1]),w1=gv(args[2]),h1=gv(args[3]);
        int64_t x2=gv(args[4]),y2=gv(args[5]),w2=gv(args[6]),h2=gv(args[7]);
        return Value(!(x1+w1<=x2||x2+w2<=x1||y1+h1<=y2||y2+h2<=y1));
    };
    m["rect.contains"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<6) return Value(false);
        auto gv=[](const Value& v)->int64_t{auto*i=std::get_if<int64_t>(&v);return i?*i:0;};
        int64_t rx=gv(args[0]),ry=gv(args[1]),rw=gv(args[2]),rh=gv(args[3]);
        int64_t px=gv(args[4]),py=gv(args[5]);
        return Value(px>=rx&&px<=rx+rw&&py>=ry&&py<=ry+rh);
    };
    m["circle.overlap"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<6) return Value(false);
        auto gv=[](const Value& v)->int64_t{auto*i=std::get_if<int64_t>(&v);return i?*i:0;};
        int64_t x1=gv(args[0]),y1=gv(args[1]),r1=gv(args[2]);
        int64_t x2=gv(args[3]),y2=gv(args[4]),r2=gv(args[5]);
        int64_t dx=x2-x1,dy=y2-y1,rr=r1+r2;
        return Value(dx*dx+dy*dy<=rr*rr);
    };
    m["circle.contains"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) return Value(false);
        auto gv=[](const Value& v)->int64_t{auto*i=std::get_if<int64_t>(&v);return i?*i:0;};
        int64_t cx=gv(args[0]),cy=gv(args[1]),cr=gv(args[2]);
        int64_t px=gv(args[3]),py=gv(args[4]);
        int64_t dx=px-cx,dy=py-cy;
        return Value(dx*dx+dy*dy<=cr*cr);
    };

    // ═══ Native Math Types ═══════════════════════════════════
    m["vec3"] = [](const std::vector<Value>& args) -> Value {
        double x=0,y=0,z=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)x=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)y=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)z=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)z=*f;}}
        return Value(glm::vec3((float)x,(float)y,(float)z));
    };
    m["quat"] = [](const std::vector<Value>& args) -> Value {
        double x=0,y=0,z=0,w=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)x=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)y=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)z=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)z=*f;}}
        if(args.size()>=4){auto*i=std::get_if<int64_t>(&args[3]);if(i)w=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)w=*f;}}
        return Value(glm::quat((float)w,(float)x,(float)y,(float)z));
    };
    m["mat4"] = [](const std::vector<Value>& args) -> Value {
        (void)args;
        return Value(glm::mat4(1.0f));
    };
    m["mat4.translate"] = [](const std::vector<Value>& args) -> Value {
        double x=0,y=0,z=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)x=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)y=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)z=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)z=*f;}}
        return Value(glm::translate(glm::mat4(1.0f), glm::vec3((float)x,(float)y,(float)z)));
    };
    m["mat4.rotate"] = [](const std::vector<Value>& args) -> Value {
        double angle=0,x=0,y=0,z=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)angle=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)angle=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)x=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)x=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)y=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)y=*f;}}
        if(args.size()>=4){auto*i=std::get_if<int64_t>(&args[3]);if(i)z=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)z=*f;}}
        return Value(glm::rotate(glm::mat4(1.0f), glm::radians((float)angle), glm::vec3((float)x,(float)y,(float)z)));
    };
    m["mat4.scale"] = [](const std::vector<Value>& args) -> Value {
        double x=0,y=0,z=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)x=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)x=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)y=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)y=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)z=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)z=*f;}}
        return Value(glm::scale(glm::mat4(1.0f), glm::vec3((float)x,(float)y,(float)z)));
    };
    m["mat4.perspective"] = [](const std::vector<Value>& args) -> Value {
        double fov=45,aspect=1,near=0.1,far=100;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)fov=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)fov=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)aspect=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)aspect=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)near=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)near=*f;}}
        if(args.size()>=4){auto*i=std::get_if<int64_t>(&args[3]);if(i)far=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)far=*f;}}
        return Value(glm::perspective(glm::radians((float)fov), (float)aspect, (float)near, (float)far));
    };
    m["mat4.lookat"] = [](const std::vector<Value>& args) -> Value {
        double ex=0,ey=0,ez=0,tx=0,ty=0,tz=0,ux=0,uy=0,uz=1;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)ex=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)ex=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)ey=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)ey=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)ez=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)ez=*f;}}
        if(args.size()>=4){auto*i=std::get_if<int64_t>(&args[3]);if(i)tx=(double)*i;else{auto*f=std::get_if<double>(&args[3]);if(f)tx=*f;}}
        if(args.size()>=5){auto*i=std::get_if<int64_t>(&args[4]);if(i)ty=(double)*i;else{auto*f=std::get_if<double>(&args[4]);if(f)ty=*f;}}
        if(args.size()>=6){auto*i=std::get_if<int64_t>(&args[5]);if(i)tz=(double)*i;else{auto*f=std::get_if<double>(&args[5]);if(f)tz=*f;}}
        if(args.size()>=7){auto*i=std::get_if<int64_t>(&args[6]);if(i)ux=(double)*i;else{auto*f=std::get_if<double>(&args[6]);if(f)ux=*f;}}
        if(args.size()>=8){auto*i=std::get_if<int64_t>(&args[7]);if(i)uy=(double)*i;else{auto*f=std::get_if<double>(&args[7]);if(f)uy=*f;}}
        if(args.size()>=9){auto*i=std::get_if<int64_t>(&args[8]);if(i)uz=(double)*i;else{auto*f=std::get_if<double>(&args[8]);if(f)uz=*f;}}
        return Value(glm::lookAt(glm::vec3((float)ex,(float)ey,(float)ez), glm::vec3((float)tx,(float)ty,(float)tz), glm::vec3((float)ux,(float)uy,(float)uz)));
    };
    m["quat.euler"] = [](const std::vector<Value>& args) -> Value {
        double pitch=0,yaw=0,roll=0;
        if(args.size()>=1){auto*i=std::get_if<int64_t>(&args[0]);if(i)pitch=(double)*i;else{auto*f=std::get_if<double>(&args[0]);if(f)pitch=*f;}}
        if(args.size()>=2){auto*i=std::get_if<int64_t>(&args[1]);if(i)yaw=(double)*i;else{auto*f=std::get_if<double>(&args[1]);if(f)yaw=*f;}}
        if(args.size()>=3){auto*i=std::get_if<int64_t>(&args[2]);if(i)roll=(double)*i;else{auto*f=std::get_if<double>(&args[2]);if(f)roll=*f;}}
        glm::quat q = glm::quat(glm::vec3((float)pitch,(float)yaw,(float)roll));
        return Value(q);
    };
}

} // namespace UCLang

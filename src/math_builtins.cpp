#include "math_builtins.h"
#include "interpreter.h"
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <stdexcept>

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
}

} // namespace UCLang

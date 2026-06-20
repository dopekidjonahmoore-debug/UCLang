#include "physics_builtins.h"
#include "interpreter.h"
#include <cmath>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <limits>

namespace UCLang {

// ─── 3D vector math helpers ───
struct Vec3d {
    double x,y,z;
    Vec3d() : x(0),y(0),z(0) {}
    Vec3d(double x, double y, double z) : x(x), y(y), z(z) {}
    Vec3d operator+(const Vec3d& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3d operator-(const Vec3d& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3d operator*(double s) const { return {x*s, y*s, z*s}; }
    Vec3d operator/(double s) const { return {x/s, y/s, z/s}; }
    double dot(const Vec3d& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3d cross(const Vec3d& o) const { return {y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x}; }
    double len2() const { return x*x + y*y + z*z; }
    double len() const { return std::sqrt(len2()); }
    Vec3d normalized() const { double l=len(); return l>1e-10 ? (*this)/l : Vec3d{0,0,0}; }
};

// ─── Collision Mesh ───
struct Tri { int i0,i1,i2; };

struct CollisionMesh {
    std::vector<Vec3d> vertices;
    std::vector<Tri> triangles;
    bool isConvex = true;
    // Transformed (world-space) copy stored per-body for quick access
    std::vector<Vec3d> worldVerts;
    void buildWorldVerts(double x, double y, double z) {
        worldVerts.resize(vertices.size());
        for (size_t i = 0; i < vertices.size(); ++i)
            worldVerts[i] = {vertices[i].x + x, vertices[i].y + y, vertices[i].z + z};
    }
};

// ─── Physics Body ───
struct PhysicsBody {
    double x,y,z, vx,vy,vz, mass;
    double w,h,d, radius;
    bool isBox;
    std::shared_ptr<CollisionMesh> mesh; // null if not mesh collider
};

static std::map<std::string, PhysicsBody> g_bodies;
static double g_gravityX=0, g_gravityY=-9.8, g_gravityZ=0;
static double g_restitution=0.5;
static double g_friction=0.02;
static bool g_collisionEnabled=true;

// Forward declarations (PhysicsBody defined above)
static bool sphereSphere(const PhysicsBody& a, const PhysicsBody& b);
static bool boxBox(const PhysicsBody& a, const PhysicsBody& b);
static bool boxSphere(const PhysicsBody& box, const PhysicsBody& sphere);

static double gv(const Value& v) {
    if(auto*i=std::get_if<int64_t>(&v))return(double)*i;
    if(auto*f=std::get_if<double>(&v))return*f;
    return 0.0;
}

// ─── Support function for GJK ───
// Returns the farthest point in direction d
static Vec3d supportBox(const PhysicsBody& b, const Vec3d& d) {
    Vec3d c{b.x, b.y, b.z};
    Vec3d e{b.w/2, b.h/2, b.d/2};
    Vec3d r;
    r.x = d.x > 0 ? c.x + e.x : c.x - e.x;
    r.y = d.y > 0 ? c.y + e.y : c.y - e.y;
    r.z = d.z > 0 ? c.z + e.z : c.z - e.z;
    return r;
}

static Vec3d supportSphere(const PhysicsBody& b, const Vec3d& d) {
    double l = d.len();
    if (l < 1e-10) return {b.x, b.y, b.z};
    return {b.x + d.x/l * b.radius, b.y + d.y/l * b.radius, b.z + d.z/l * b.radius};
}

static Vec3d supportMesh(const PhysicsBody& b, const Vec3d& d) {
    if (!b.mesh || b.mesh->worldVerts.empty()) return {b.x, b.y, b.z};
    double bestDot = -std::numeric_limits<double>::max();
    size_t bestIdx = 0;
    for (size_t i = 0; i < b.mesh->worldVerts.size(); ++i) {
        double dt = b.mesh->worldVerts[i].dot(d);
        if (dt > bestDot) { bestDot = dt; bestIdx = i; }
    }
    return b.mesh->worldVerts[bestIdx];
}

static Vec3d support(const PhysicsBody& b, const Vec3d& d) {
    if (b.mesh) return supportMesh(b, d);
    if (b.isBox) return supportBox(b, d);
    return supportSphere(b, d);
}

// Support difference for GJK: support(A,d) - support(B,-d)
static Vec3d supportDiff(const PhysicsBody& a, const PhysicsBody& b, const Vec3d& d) {
    return support(a, d) - support(b, d * -1.0);
}

// ─── GJK ───
// Returns true if the two bodies intersect
static bool gjk(const PhysicsBody& a, const PhysicsBody& b) {
    // Quick AABB pre-check for boxes
    if (a.isBox && !a.mesh && b.isBox && !b.mesh)
        return boxBox(a,b);

    Vec3d simplex[4];
    int n = 0; // number of points in simplex

    // Initial direction
    Vec3d d = {1, 0, 0};
    Vec3d p = supportDiff(a, b, d);
    simplex[n++] = p;
    d = p * -1.0;

    const int maxIter = 64;
    for (int iter = 0; iter < maxIter; ++iter) {
        p = supportDiff(a, b, d);
        if (p.dot(d) < 0) return false; // no intersection

        simplex[n++] = p;

        if (n == 2) {
            // Line case
            Vec3d ab = simplex[1] - simplex[0];
            Vec3d ao = simplex[0] * -1.0;
            d = ab.cross(ao).cross(ab);
            if (d.len2() < 1e-20) d = {ab.y - ab.z, ab.z - ab.x, ab.x - ab.y};
            if (d.len2() < 1e-20) d = {1, 0, 0};
        } else if (n == 3) {
            // Triangle case
            Vec3d a_ = simplex[0];
            Vec3d b_ = simplex[1];
            Vec3d c_ = simplex[2];
            Vec3d ab = b_ - a_;
            Vec3d ac = c_ - a_;
            Vec3d n_ = ab.cross(ac);
            Vec3d ao = a_ * -1.0;

            if (n_.cross(ac).dot(ao) > 0) {
                // In region of AC
                // Remove B
                simplex[1] = simplex[2];
                n = 2;
                d = ac.cross(ao).cross(ac);
                if (d.len2() < 1e-20) d = ac * -1.0;
            } else if (ab.cross(n_).dot(ao) > 0) {
                // In region of AB
                n = 2;
                d = ab.cross(ao).cross(ab);
                if (d.len2() < 1e-20) d = ab * -1.0;
            } else {
                // In region of triangle, check if above or below
                if (n_.dot(ao) > 0) {
                    d = n_ * -1.0;
                } else {
                    d = n_;
                    // Swap B and C to maintain winding
                    Vec3d tmp = simplex[1];
                    simplex[1] = simplex[2];
                    simplex[2] = tmp;
                }
            }
        } else if (n == 4) {
            // Tetrahedron case — intersection found
            return true;
        }
    }
    return false;
}

// ─── EPA for collision normal and depth ───
struct EPAResult {
    Vec3d normal;
    double depth;
};

static EPAResult epa(const PhysicsBody& a, const PhysicsBody& b) {
    // Build initial tetrahedron from GJK simplex, then expand
    // For simplicity, use the axis of minimum overlap
    Vec3d axes[] = {
        {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
    };
    double bestDepth = -1e10;
    Vec3d bestNormal = {0,1,0};
    for (const auto& axis : axes) {
        Vec3d sa = support(a, axis);
        Vec3d sb = support(b, axis * -1.0);
        double depth = (sa - sb).dot(axis);
        if (depth > bestDepth) {
            bestDepth = depth;
            bestNormal = axis;
        }
    }
    // If no good axis found (shouldn't happen since we know they intersect), fallback
    Vec3d centerDiff = {a.x - b.x, a.y - b.y, a.z - b.z};
    if (bestDepth < 0) {
        bestNormal = centerDiff;
        bestDepth = centerDiff.len();
    }
    if (bestDepth < 1e-10) { bestDepth = 0.1; bestNormal = {0,1,0}; }

    // Use center-to-center vector for normal refinement
    double cdLen = centerDiff.len();
    if (cdLen > 1e-6) {
        Vec3d cdN = centerDiff / cdLen;
        // Blend: use 30% center direction, 70% best axis
        bestNormal = (bestNormal * 0.7 + cdN * 0.3).normalized();
    } else {
        bestNormal = bestNormal.normalized();
    }

    return {bestNormal, bestDepth};
}

// ─── Triangle vs point collision ───
static bool pointInTriangle(const Vec3d& p, const Vec3d& a, const Vec3d& b, const Vec3d& c) {
    Vec3d ab = b - a;
    Vec3d ac = c - a;
    Vec3d ap = p - a;
    double d00 = ab.dot(ab);
    double d01 = ab.dot(ac);
    double d11 = ac.dot(ac);
    double d20 = ap.dot(ab);
    double d21 = ap.dot(ac);
    double denom = d00 * d11 - d01 * d01;
    if (std::abs(denom) < 1e-10) return false;
    double v = (d11 * d20 - d01 * d21) / denom;
    double w = (d00 * d21 - d01 * d20) / denom;
    double u = 1.0 - v - w;
    return u >= -1e-6 && v >= -1e-6 && w >= -1e-6;
}

static double pointTriDist(const Vec3d& p, const Vec3d& a, const Vec3d& b, const Vec3d& c, Vec3d& normal) {
    Vec3d ab = b - a;
    Vec3d ac = c - a;
    normal = ab.cross(ac).normalized();
    double d = (p - a).dot(normal);
    Vec3d proj = p - normal * d;
    if (pointInTriangle(proj, a, b, c)) return std::abs(d);
    return 1e10;
}

// ─── Mesh vs sphere collision ───
static bool meshSphereCollide(const PhysicsBody& mesh, const PhysicsBody& sphere, Vec3d& normal, double& depth) {
    if (!mesh.mesh || mesh.mesh->triangles.empty()) return false;
    Vec3d sPos = {sphere.x, sphere.y, sphere.z};
    double bestDist = 1e10;
    Vec3d bestNorm = {0,1,0};
    bool hit = false;

    for (const auto& tri : mesh.mesh->triangles) {
        const Vec3d& a = mesh.mesh->worldVerts[tri.i0];
        const Vec3d& b = mesh.mesh->worldVerts[tri.i1];
        const Vec3d& c = mesh.mesh->worldVerts[tri.i2];
        Vec3d triNorm;
        double dist = pointTriDist(sPos, a, b, c, triNorm);
        if (dist < sphere.radius + 1e-4 && dist < bestDist) {
            bestDist = dist;
            bestNorm = triNorm;
            hit = true;
        }
    }

    if (hit) {
        double pen = sphere.radius - bestDist;
        if (pen > 0) {
            // Make sure normal points toward sphere
            Vec3d toSphere = sPos - (mesh.mesh->worldVerts[mesh.mesh->triangles[0].i0]);
            if (toSphere.dot(bestNorm) < 0) bestNorm = bestNorm * -1.0;
            normal = bestNorm;
            depth = pen;
            return true;
        }
    }
    return false;
}

// ─── Collision dispatch ───
static bool collision(const PhysicsBody& a, const PhysicsBody& b) {
    // Mesh collisions
    if (a.mesh && b.mesh) return gjk(a, b);
    if (a.mesh && !b.isBox) { // sphere vs mesh
        Vec3d n; double d;
        return meshSphereCollide(a, b, n, d);
    }
    if (b.mesh && !a.isBox) {
        Vec3d n; double d;
        return meshSphereCollide(b, a, n, d);
    }
    if (a.mesh && b.isBox) return gjk(a, b);
    if (b.mesh && a.isBox) return gjk(b, a);

    // Primitive vs primitive
    if(a.isBox && b.isBox) return boxBox(a,b);
    if(!a.isBox && !b.isBox) return sphereSphere(a,b);
    if(a.isBox) return boxSphere(a,b);
    return boxSphere(b,a);
}

static void resolveCollision(PhysicsBody& a, PhysicsBody& b) {
    Vec3d normal;
    double depth = 0;
    Vec3d centerDiff = {a.x - b.x, a.y - b.y, a.z - b.z};

    // Use EPA or center-based normal
    if (a.mesh || b.mesh) {
        // For mesh collisions, use GJK/EPA
        if (a.mesh && !b.isBox && !b.mesh) {
            Vec3d n; double d;
            if (meshSphereCollide(a, b, n, d)) { normal = n; depth = d; }
            else { normal = centerDiff.normalized(); depth = 0.1; }
        } else if (b.mesh && !a.isBox && !a.mesh) {
            Vec3d n; double d;
            if (meshSphereCollide(b, a, n, d)) { normal = n * -1.0; depth = d; }
            else { normal = centerDiff.normalized(); depth = 0.1; }
        } else {
            EPAResult epaRes = epa(a, b);
            normal = epaRes.normal;
            depth = epaRes.depth;
        }
    } else {
        // Primitive collision normal
        double dx=b.x-a.x, dy=b.y-a.y, dz=b.z-a.z;
        double dist=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(dist<0.0001) { dx=0; dy=1; dz=0; dist=1; }
        normal = {dx/dist, dy/dist, dz/dist};

        if(a.isBox && b.isBox) {
            depth = std::max(0.0, (a.w/2+b.w/2)-std::abs(a.x-b.x)) +
                    std::max(0.0, (a.h/2+b.h/2)-std::abs(a.y-b.y)) +
                    std::max(0.0, (a.d/2+b.d/2)-std::abs(a.z-b.z));
        } else if(!a.isBox && !b.isBox) {
            depth = (a.radius+b.radius-dist);
        } else {
            depth = 0.1;
        }
    }

    // Apply impulse
    double relVx = a.vx - b.vx;
    double relVy = a.vy - b.vy;
    double relVz = a.vz - b.vz;
    double relVn = relVx*normal.x + relVy*normal.y + relVz*normal.z;
    if(relVn>0) return;

    double j = -(1+g_restitution)*relVn;
    double invMassSum = (a.mass>0?1.0/a.mass:0) + (b.mass>0?1.0/b.mass:0);
    if(invMassSum<0.0001) return;
    j /= invMassSum;

    if(a.mass>0){ a.vx+=j*normal.x/a.mass; a.vy+=j*normal.y/a.mass; a.vz+=j*normal.z/a.mass; }
    if(b.mass>0){ b.vx-=j*normal.x/b.mass; b.vy-=j*normal.y/b.mass; b.vz-=j*normal.z/b.mass; }

    // Separate overlapping bodies
    double sep = depth*0.5+0.001;
    if(a.mass>0){ a.x-=normal.x*sep; a.y-=normal.y*sep; a.z-=normal.z*sep; }
    if(b.mass>0){ b.x+=normal.x*sep; b.y+=normal.y*sep; b.z+=normal.z*sep; }
}

static bool sphereSphere(const PhysicsBody& a, const PhysicsBody& b) {
    double dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    double dist=std::sqrt(dx*dx+dy*dy+dz*dz);
    return dist<(a.radius+b.radius);
}

static bool boxBox(const PhysicsBody& a, const PhysicsBody& b) {
    bool xOverlap = std::abs(a.x-b.x) < (a.w/2+b.w/2);
    bool yOverlap = std::abs(a.y-b.y) < (a.h/2+b.h/2);
    bool zOverlap = std::abs(a.z-b.z) < (a.d/2+b.d/2);
    return xOverlap && yOverlap && zOverlap;
}

static bool boxSphere(const PhysicsBody& box, const PhysicsBody& sphere) {
    double cx = std::max(box.x-box.w/2, std::min(sphere.x, box.x+box.w/2));
    double cy = std::max(box.y-box.h/2, std::min(sphere.y, box.y+box.h/2));
    double cz = std::max(box.z-box.d/2, std::min(sphere.z, box.z+box.d/2));
    double dx=sphere.x-cx, dy=sphere.y-cy, dz=sphere.z-cz;
    return (dx*dx+dy*dy+dz*dz)<(sphere.radius*sphere.radius);
}

// ─── Extract vec3 list from UCLang value ───
static std::vector<Vec3d> extractVec3List(const Value& val) {
    std::vector<Vec3d> result;
    auto* list = std::get_if<ValueList>(&val);
    if (!list) throw std::runtime_error("Expected a list of vec3 values");
    for (const auto& elem : *list) {
        auto* v3 = std::get_if<Vec3>(&elem);
        if (!v3) throw std::runtime_error("Expected vec3 elements in list");
        result.push_back({(double)(*v3).x, (double)(*v3).y, (double)(*v3).z});
    }
    return result;
}

// ─── Extract triangle list from UCLang value ───
static std::vector<Tri> extractTriList(const Value& val) {
    std::vector<Tri> result;
    auto* list = std::get_if<ValueList>(&val);
    if (!list) return result; // empty triangles = auto-generate convex hull
    for (const auto& elem : *list) {
        auto* triList = std::get_if<ValueList>(&elem);
        if (!triList || triList->size() < 3) continue;
        auto i0 = std::get_if<int64_t>(&(*triList)[0]);
        auto i1 = std::get_if<int64_t>(&(*triList)[1]);
        auto i2 = std::get_if<int64_t>(&(*triList)[2]);
        if (!i0 || !i1 || !i2) continue;
        result.push_back({(int)*i0, (int)*i1, (int)*i2});
    }
    return result;
}

void register_physics_natives(
    std::unordered_map<std::string, std::function<Value(const std::vector<Value>&)>>& m)
{
    m["physics.set_gravity"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()>=3){
            g_gravityX=gv(args[0]); g_gravityY=gv(args[1]); g_gravityZ=gv(args[2]);
        }
        return std::monostate{};
    };
    m["physics.add_body"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.add_body(name,x,y,z,mass): need 5");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_body: name must be string");
        PhysicsBody b;
        b.x=gv(args[1]); b.y=gv(args[2]); b.z=args.size()>2?gv(args[2]):0;
        if(args.size()>=4) b.z=gv(args[3]);
        if(args.size()>=5) b.mass=gv(args[4]); else b.mass=1.0;
        b.vx=b.vy=b.vz=0; b.w=b.h=b.d=1; b.radius=0.5; b.isBox=true;
        b.mesh = nullptr;
        g_bodies[*name]=b;
        return std::monostate{};
    };
    m["physics.add_box_collider"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.add_box_collider(name,w,h,d): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_box_collider: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
        it->second.w=gv(args[1]); it->second.h=gv(args[2]); it->second.d=args.size()>3?gv(args[3]):gv(args[1]);
        it->second.isBox=true;
        it->second.mesh = nullptr;
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
        it->second.mesh = nullptr;
        return std::monostate{};
    };
    // ─── Mesh / vertex collider ───
    // physics.add_mesh_collider(name, vertex_list, triangle_list?, is_convex?)
    // vertex_list: list of vec3 values
    // triangle_list (optional): list of [i0,i1,i2] triples (omit for convex hull auto-triangulation)
    // is_convex (optional): 1 for convex (uses GJK), 0 for concave (uses per-triangle). Default 1.
    m["physics.add_mesh_collider"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Physics.add_mesh_collider(name, vertices, triangles?, convex?): need 2");
        auto* name = std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.add_mesh_collider: name must be string");
        auto it = g_bodies.find(*name);
        if(it == g_bodies.end()){
            // Auto-create body at origin
            PhysicsBody b;
            b.x=0; b.y=0; b.z=0; b.mass=0; b.vx=b.vy=b.vz=0;
            b.w=b.h=b.d=1; b.radius=0.5; b.isBox=false;
            g_bodies[*name] = b;
            it = g_bodies.find(*name);
        }

        std::vector<Vec3d> verts = extractVec3List(args[1]);
        std::vector<Tri> tris;
        bool isConvex = true;

        if(args.size() >= 3) {
            auto* triArg = std::get_if<ValueList>(&args[2]);
            if(triArg && !triArg->empty()) tris = extractTriList(args[2]);
        }
        if(args.size() >= 4) {
            auto* conv = std::get_if<int64_t>(&args[3]);
            if(conv) isConvex = (*conv != 0);
        }

        if(verts.empty()) throw std::runtime_error("Physics.add_mesh_collider: need at least 1 vertex");

        // Auto-generate triangles for convex hull if not provided
        if(tris.empty() && verts.size() >= 3) {
            // Simple fan triangulation from first vertex
            for(size_t i = 1; i + 1 < verts.size(); ++i)
                tris.push_back({0, (int)i, (int)(i+1)});
        }

        auto mesh = std::make_shared<CollisionMesh>();
        mesh->vertices = verts;
        mesh->triangles = tris;
        mesh->isConvex = isConvex;
        mesh->buildWorldVerts(it->second.x, it->second.y, it->second.z);
        it->second.mesh = mesh;
        it->second.isBox = false;
        return std::monostate{};
    };
    m["physics.set_velocity"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<4) throw std::runtime_error("Physics.set_velocity(name,vx,vy,vz): need 4");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.set_velocity: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()) return std::monostate{};
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
        double fx=gv(args[1]), fy=gv(args[2]), fz=gv(args[3]);
        if(it->second.mass>0){
            it->second.vx+=fx/it->second.mass;
            it->second.vy+=fy/it->second.mass;
            it->second.vz+=fz/it->second.mass;
        }
        return std::monostate{};
    };
    m["physics.set_restitution"] = [](const std::vector<Value>& args) -> Value {
        if(!args.empty()) g_restitution=gv(args[0]);
        return std::monostate{};
    };
    m["physics.set_friction"] = [](const std::vector<Value>& args) -> Value {
        if(!args.empty()) g_friction=gv(args[0]);
        return std::monostate{};
    };
    m["physics.get_body_count"] = [](const std::vector<Value>&) -> Value {
        return Value((int64_t)g_bodies.size());
    };
    m["physics.remove_body"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Physics.remove_body(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.remove_body: name must be string");
        g_bodies.erase(*name);
        return std::monostate{};
    };
    m["physics.set_collision_enabled"] = [](const std::vector<Value>& args) -> Value {
        if(!args.empty()){auto*b=std::get_if<int64_t>(&args[0]);if(b)g_collisionEnabled=*b!=0;}
        return std::monostate{};
    };
    m["physics.step"] = [](const std::vector<Value>& args) -> Value {
        double dt=0.016;
        if(!args.empty()) dt=gv(args[0]);
        if(dt>0.05) dt=0.05;

        for(auto& kv : g_bodies){
            auto& b=kv.second;
            b.vx+=g_gravityX*dt;
            b.vy+=g_gravityY*dt;
            b.vz+=g_gravityZ*dt;
        }

        for(auto& kv : g_bodies){
            auto& b=kv.second;
            b.vx*=(1.0-g_friction);
            b.vy*=(1.0-g_friction);
            b.vz*=(1.0-g_friction);
        }

        for(auto& kv : g_bodies){
            auto& b=kv.second;
            b.x+=b.vx*dt;
            b.y+=b.vy*dt;
            b.z+=b.vz*dt;
            // Update mesh world-space vertices
            if(b.mesh) b.mesh->buildWorldVerts(b.x, b.y, b.z);
        }

        // Ground plane collision
        for(auto& kv : g_bodies){
            auto& b=kv.second;
            double floorY = 0;
            if(b.mesh) {
                // Find lowest vertex
                double minY = 1e10;
                for(const auto& v : b.mesh->worldVerts) if(v.y < minY) minY = v.y;
                floorY = -minY + b.y;
                if(b.y < floorY) { b.y = floorY; b.vy = -b.vy * g_restitution; }
                continue;
            }
            if(b.isBox) floorY = b.h/2;
            else floorY = b.radius;
            if(b.y<floorY){
                b.y=floorY;
                b.vy=-b.vy*g_restitution;
                b.vx*=(1.0-g_friction*3);
                b.vz*=(1.0-g_friction*3);
            }
        }

        // Body-body collisions
        if(g_collisionEnabled){
            std::vector<std::pair<std::string,std::string>> pairs;
            for(auto& ka : g_bodies)
                for(auto& kb : g_bodies)
                    if(ka.first<kb.first)
                        pairs.push_back({ka.first,kb.first});
            for(auto& p : pairs){
                auto& a=g_bodies[p.first];
                auto& b=g_bodies[p.second];
                if(a.mass<0.0001 && b.mass<0.0001) continue;
                // Refresh mesh world transforms
                if(a.mesh) a.mesh->buildWorldVerts(a.x, a.y, a.z);
                if(b.mesh) b.mesh->buildWorldVerts(b.x, b.y, b.z);
                if(collision(a,b))
                    resolveCollision(a,b);
            }
        }

        for(auto& kv : g_bodies){
            auto& b=kv.second;
            double maxV=100.0;
            b.vx=std::max(-maxV,std::min(maxV,b.vx));
            b.vy=std::max(-maxV,std::min(maxV,b.vy));
            b.vz=std::max(-maxV,std::min(maxV,b.vz));
        }

        return std::monostate{};
    };
    m["physics.collision_test"] = [](const std::vector<Value>& args) -> Value {
        if(args.size()<2) throw std::runtime_error("Physics.collision_test(name1,name2): need 2");
        auto* na=std::get_if<std::string>(&args[0]);
        auto* nb=std::get_if<std::string>(&args[1]);
        if(!na||!nb) throw std::runtime_error("Physics.collision_test: names must be strings");
        auto ita=g_bodies.find(*na), itb=g_bodies.find(*nb);
        if(ita==g_bodies.end()||itb==g_bodies.end()) return Value((int64_t)0);
        // Refresh mesh transforms for accurate test
        if(ita->second.mesh) ita->second.mesh->buildWorldVerts(ita->second.x, ita->second.y, ita->second.z);
        if(itb->second.mesh) itb->second.mesh->buildWorldVerts(itb->second.x, itb->second.y, itb->second.z);
        return Value((int64_t)(collision(ita->second,itb->second)?1:0));
    };
    m["physics.get_mesh_vertices"] = [](const std::vector<Value>& args) -> Value {
        if(args.empty()) throw std::runtime_error("Physics.get_mesh_vertices(name): need name");
        auto* name=std::get_if<std::string>(&args[0]);
        if(!name) throw std::runtime_error("Physics.get_mesh_vertices: name must be string");
        auto it=g_bodies.find(*name);
        if(it==g_bodies.end()||!it->second.mesh) return ValueList{};
        ValueList result;
        for(const auto& v : it->second.mesh->worldVerts)
            result.push_back(Value(Vec3((float)v.x, (float)v.y, (float)v.z)));
        return Value(result);
    };
}

} // namespace UCLang
#include "geometry/canonical_shapes.h"
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace swc;
struct Sample { Vec3 p,n; };
void require(bool ok,const char* message) { if(!ok) throw std::runtime_error(message); }
bool near(float a,float b) { return std::fabs(a-b)<.0001f; }

template<class Generate>
std::vector<Sample> check(Generate generate,std::size_t count,Vec3 lo,Vec3 hi) {
    std::vector<Sample> vertices;
    generate([&](Vec3 p,Vec3 n) { vertices.push_back({p,n}); });
    require(vertices.size()==count,"Unexpected tessellation size");
    for(const auto& v:vertices) {
        require(std::isfinite(v.p.x)&&std::isfinite(v.p.y)&&std::isfinite(v.p.z),"Non-finite vertex");
        require(near(length(v.n),1),"Normal must have unit length");
        require(v.p.x>=lo.x-.0001f&&v.p.x<=hi.x+.0001f&&
                v.p.y>=lo.y-.0001f&&v.p.y<=hi.y+.0001f&&
                v.p.z>=lo.z-.0001f&&v.p.z<=hi.z+.0001f,"Vertex outside canonical bounds");
    }
    for(std::size_t i=0;i<vertices.size();i+=3) {
        Vec3 geometric=cross(vertices[i+1].p-vertices[i].p,vertices[i+2].p-vertices[i].p);
        if(length(geometric)>.00001f)
            require(dot(geometric,vertices[i].n+vertices[i+1].n+vertices[i+2].n)>0,"Winding disagrees with normals");
    }
    return vertices;
}

int main() {
    try {
        check([](auto out){canonical::cube(out);},36,{-.5f,-.5f,-.5f},{.5f,.5f,.5f});
        auto cylinder=check([](auto out){canonical::cylinder(out);},240,{-1,-.5f,-1},{1,.5f,1});
        for(const auto& v:cylinder) if(v.n.y==0)
            require(near(v.p.x*v.p.x+v.p.z*v.p.z,1),"Cylinder side must be on unit circle");
        auto sphere=check([](auto out){canonical::sphere(out);},672,{-1,-1,-1},{1,1,1});
        for(const auto& v:sphere) require(near(length(v.p),1),"Sphere vertex must have unit radius");
        check([](auto out){canonical::tube(out);},48,{-1,0,-1},{1,1,1});
        check([](auto out){canonical::cone(out);},120,{-1,0,-1},{1,1,1});
        check([](auto out){canonical::frustum(out);},168,{-1,0,-1},{1,1,1});
        auto ring=check([](auto out){canonical::annulus(out);},384,{-1,0,-1},{1,0,1});
        for(const auto& v:ring) require(near(length(v.p),.5f)||near(length(v.p),1),"Incorrect ring radius");
        check([](auto out){canonical::annulus(out,.25f,8,0,PI/2);},48,{0,0,0},{1,0,1});
        check([](auto out){canonical::plane(out);},6,{-.5f,0,-.5f},{.5f,0,.5f});
        check([](auto out){canonical::triangle(out);},3,{0,0,0},{1,1,0});
        std::size_t invalidCount=0;
        auto count=[&](Vec3,Vec3){++invalidCount;};
        canonical::sphere(count,0,14); canonical::cylinder(count,2);
        canonical::frustum(count,-1); canonical::annulus(count,2);
        require(invalidCount==0,"Invalid parameters must not produce geometry");
        // A rotated, non-uniformly scaled instance must preserve perpendicular
        // normals. Check tangents instead of duplicating the matrix formula.
        Mat4 model=trs({3,4,5},37,{2,3,.5f});
        Vec3 normal=normalize(Vec3{1,1,1});
        Vec3 tangent=cross(normal,{0,1,0});
        Vec3 moved=transformPoint(model,tangent)-transformPoint(model,{0,0,0});
        require(std::fabs(dot(normalize(transformNormal(normalMatrix(model),normal)),moved))<.0001f,
                "Non-uniform transform broke normal perpendicularity");
        Vec3 origin=transformPoint(model,{0,0,0});
        require(near(origin.x,3)&&near(origin.y,4)&&near(origin.z,5),"Incorrect instance translation");
        std::cout<<"Canonical geometry checks passed.\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}

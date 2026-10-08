#pragma once

#include "geometry/geometry_math.h"

// Renderer-independent canonical geometry. An emitter accepts (Vec3 position,
// Vec3 normal); every three consecutive calls form a triangle. No OpenGL state,
// materials, global mesh, or heap allocation is required by these generators.
namespace swc { namespace canonical {

template<class Emit>
void quad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 normal,Emit emit) {
    emit(a,normal); emit(b,normal); emit(c,normal);
    emit(a,normal); emit(c,normal); emit(d,normal);
}

template<class Visit>
void cubeFaces(Visit visit) {
    constexpr float lo=-.5f,hi=.5f;
    visit(Vec3{lo,lo,hi},Vec3{hi,lo,hi},Vec3{hi,hi,hi},Vec3{lo,hi,hi},Vec3{0,0,1});
    visit(Vec3{hi,lo,lo},Vec3{lo,lo,lo},Vec3{lo,hi,lo},Vec3{hi,hi,lo},Vec3{0,0,-1});
    visit(Vec3{hi,lo,hi},Vec3{hi,lo,lo},Vec3{hi,hi,lo},Vec3{hi,hi,hi},Vec3{1,0,0});
    visit(Vec3{lo,lo,lo},Vec3{lo,lo,hi},Vec3{lo,hi,hi},Vec3{lo,hi,lo},Vec3{-1,0,0});
    visit(Vec3{lo,hi,hi},Vec3{hi,hi,hi},Vec3{hi,hi,lo},Vec3{lo,hi,lo},Vec3{0,1,0});
    visit(Vec3{lo,lo,lo},Vec3{hi,lo,lo},Vec3{hi,lo,hi},Vec3{lo,lo,hi},Vec3{0,-1,0});
}

// Unit cube: side length 1, centred at the origin.
template<class Emit>
void cube(Emit emit) {
    cubeFaces([&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) { quad(a,b,c,d,n,emit); });
}

template<class Visit>
void circleSectors(int segments,Visit visit) {
    if(segments<3) return;
    for(int i=0;i<segments;++i) {
        float a=2*PI*i/segments,b=2*PI*(i+1)/segments;
        visit(Vec3{std::cos(a),0,std::sin(a)},Vec3{std::cos(b),0,std::sin(b)});
    }
}

// Radius 1, height 1, centred on Y in [-.5,.5], both caps included.
template<class Emit>
void cylinder(Emit emit,int segments=20) {
    circleSectors(segments,[&](Vec3 a,Vec3 b) {
        Vec3 p0{a.x,-.5f,a.z},p1{b.x,-.5f,b.z},p2{b.x,.5f,b.z},p3{a.x,.5f,a.z};
        emit(p1,b); emit(p0,a); emit(p3,a);
        emit(p1,b); emit(p3,a); emit(p2,b);
        emit({0,.5f,0},{0,1,0}); emit(p2,{0,1,0}); emit(p3,{0,1,0});
        emit({0,-.5f,0},{0,-1,0}); emit(p0,{0,-1,0}); emit(p1,{0,-1,0});
    });
}

// Radius 1, Y in [0,1], no caps. Its ends map directly to rod endpoints.
template<class Emit>
void tube(Emit emit,int segments=8) {
    circleSectors(segments,[&](Vec3 a,Vec3 b) {
        Vec3 topA{a.x,1,a.z},topB{b.x,1,b.z};
        emit(a,a); emit(topB,b); emit(b,b);
        emit(a,a); emit(topA,a); emit(topB,b);
    });
}

// Unit sphere: centre (0,0,0), radius 1. Position equals its smooth normal.
template<class Emit>
void sphere(Emit emit,int rings=8,int slices=14) {
    if(rings<2||slices<3) return;
    for(int r=0;r<rings;++r) {
        float p0=-PI/2+PI*r/rings,p1=-PI/2+PI*(r+1)/rings;
        for(int s=0;s<slices;++s) {
            float a0=2*PI*s/slices,a1=2*PI*(s+1)/slices;
            Vec3 n00{std::cos(p0)*std::cos(a0),std::sin(p0),std::cos(p0)*std::sin(a0)};
            Vec3 n01{std::cos(p0)*std::cos(a1),std::sin(p0),std::cos(p0)*std::sin(a1)};
            Vec3 n11{std::cos(p1)*std::cos(a1),std::sin(p1),std::cos(p1)*std::sin(a1)};
            Vec3 n10{std::cos(p1)*std::cos(a0),std::sin(p1),std::cos(p1)*std::sin(a0)};
            emit(n00,n00); emit(n11,n11); emit(n01,n01);
            emit(n00,n00); emit(n10,n10); emit(n11,n11);
        }
    }
}

// Radius 1 at Y=0, top radius topRatio at Y=1. Legacy scene frusta are
// bottomless; standalone callers can request either cap independently.
template<class Emit>
void frustum(Emit emit,float topRatio=.5f,int segments=14,bool bottomCap=true,bool topCap=true) {
    if(topRatio<0) return;
    circleSectors(segments,[&](Vec3 a,Vec3 b) {
        Vec3 p2=b*topRatio+Vec3{0,1,0},p3=a*topRatio+Vec3{0,1,0};
        Vec3 na=normalize({a.x,1-topRatio,a.z}),nb=normalize({b.x,1-topRatio,b.z});
        emit(a,na); emit(p2,nb); emit(b,nb);
        if(topRatio>0) { emit(a,na); emit(p3,na); emit(p2,nb); }
        if(topCap&&topRatio>0) { emit({0,1,0},{0,1,0}); emit(p2,{0,1,0}); emit(p3,{0,1,0}); }
        if(bottomCap) { emit({0,0,0},{0,-1,0}); emit(a,{0,-1,0}); emit(b,{0,-1,0}); }
    });
}

template<class Emit>
void cone(Emit emit,int segments=20) { frustum(emit,0,segments,true,false); }

template<class Visit>
void ringSectors(int segments,float a0,float a1,Visit visit) {
    if(segments<1) return;
    for(int i=0;i<segments;++i) {
        float a=a0+(a1-a0)*i/segments,b=a0+(a1-a0)*(i+1)/segments;
        visit(Vec3{std::cos(a),0,std::sin(a)},Vec3{std::cos(b),0,std::sin(b)});
    }
}

// XZ ring/sector: outer radius 1, normal +Y. Angles are radians.
template<class Emit>
void annulus(Emit emit,float innerRatio=.5f,int segments=64,float a0=0,float a1=2*PI) {
    if(innerRatio<0||innerRatio>1) return;
    ringSectors(segments,a0,a1,[&](Vec3 a,Vec3 b) { quad(a*innerRatio,b*innerRatio,b,a,{0,1,0},emit); });
}

template<class Emit>
void plane(Emit emit) { quad({-.5f,0,.5f},{.5f,0,.5f},{.5f,0,-.5f},{-.5f,0,-.5f},{0,1,0},emit); }

template<class Emit>
void triangle(Emit emit) { emit({0,0,0},{0,0,1}); emit({1,0,0},{0,0,1}); emit({0,1,0},{0,0,1}); }

} } // namespace swc::canonical

#pragma once
#include "geometry/geometry_math.h"
#include <array>
#include <cstdint>
#include <limits>
#include <unordered_map>
#include <vector>

namespace swc {
// Static scene collision, shared by the interactive viewer and headless tests.
// Eye coordinates are used at the API boundary; geometry is in world metres.
class WalkNavigation {
    struct Triangle { Vec3 a,b,c; float low,high; bool floor; };
    std::vector<Triangle> triangles;
    std::unordered_map<std::uint64_t,std::vector<std::size_t>> cells;
    static constexpr float cellSize=2.0f;
    static int cell(float v) { return static_cast<int>(std::floor(v/cellSize)); }
    static std::uint64_t key(int x,int z) {
        return (std::uint64_t(std::uint32_t(x))<<32)|std::uint32_t(z);
    }
    template<class Visit> void nearby(Vec3 p,float radius,Visit visit) const {
        for(int x=cell(p.x-radius);x<=cell(p.x+radius);++x)
            for(int z=cell(p.z-radius);z<=cell(p.z+radius);++z) {
                auto it=cells.find(key(x,z));
                if(it!=cells.end()) for(auto index:it->second) visit(triangles[index]);
            }
    }
    static bool heightAt(const Triangle& t,Vec3 p,float& y) {
        Vec3 u=t.b-t.a,v=t.c-t.a,w=p-t.a;
        float d=u.x*v.z-u.z*v.x;
        if(std::fabs(d)<1e-7f) return false;
        float s=(w.x*v.z-w.z*v.x)/d,r=(u.x*w.z-u.z*w.x)/d;
        if(s<-.0001f||r<-.0001f||s+r>1.0001f) return false;
        y=t.a.y+s*u.y+r*v.y;
        return true;
    }
    // Clip against the body's vertical interval; test its projected polygon
    // against a circle. This also catches thin railings and furniture edges.
    static bool overlaps(const Triangle& t,Vec3 feet) {
        std::array<Vec3,8> polygon{},scratch{};
        polygon[0]=t.a; polygon[1]=t.b; polygon[2]=t.c; int count=3;
        for(int plane=0;plane<2;++plane) {
            float h=feet.y+(plane==0 ? stepHeight+.015f : bodyHeight);
            int out=0;
            for(int i=0;i<count;++i) {
                Vec3 a=polygon[i],b=polygon[(i+1)%count];
                bool ia=plane==0 ? a.y>=h : a.y<=h;
                bool ib=plane==0 ? b.y>=h : b.y<=h;
                if(ia) scratch[out++]=a;
                if(ia!=ib) scratch[out++]=lerp(a,b,(h-a.y)/(b.y-a.y));
            }
            polygon=scratch; count=out;
            if(!count) return false;
        }
        bool positive=false,negative=false; float area=0;
        for(int i=0;i<count;++i) {
            Vec3 a=polygon[i],b=polygon[(i+1)%count];
            float dx=b.x-a.x,dz=b.z-a.z;
            float d=dx*dx+dz*dz;
            float f=d>1e-9f ? limit(((feet.x-a.x)*dx+(feet.z-a.z)*dz)/d,0,1) : 0;
            float ex=feet.x-a.x-f*dx,ez=feet.z-a.z-f*dz;
            if(ex*ex+ez*ez<bodyRadius*bodyRadius) return true;
            float side=dx*(feet.z-a.z)-dz*(feet.x-a.x);
            positive=positive||side>1e-6f; negative=negative||side<-1e-6f;
            area+=a.x*b.z-a.z*b.x;
        }
        return std::fabs(area)>1e-5f && !(positive&&negative);
    }
public:
    static constexpr float eyeHeight=1.6f,bodyHeight=1.78f,bodyRadius=.18f,stepHeight=.24f;
    void clear() { triangles.clear(); cells.clear(); }
    void addTriangle(Vec3 a,Vec3 b,Vec3 c,Vec3 normal) {
        if(length(cross(b-a,c-a))<1e-8f) return;
        std::size_t index=triangles.size();
        triangles.push_back({a,b,c,std::min({a.y,b.y,c.y}),std::max({a.y,b.y,c.y}),normal.y>.65f});
        for(int x=cell(std::min({a.x,b.x,c.x}));x<=cell(std::max({a.x,b.x,c.x}));++x)
            for(int z=cell(std::min({a.z,b.z,c.z}));z<=cell(std::max({a.z,b.z,c.z}));++z)
                cells[key(x,z)].push_back(index);
    }
    bool supported(Vec3 candidate,float previousFloor,Vec3& result) const {
        float floor=-std::numeric_limits<float>::infinity();
        nearby(candidate,bodyRadius,[&](const Triangle& t) {
            if(!t.floor) return;
            // Support the entire foot disc, so descending a plinth edge does
            // not lower the body into the edge while a foot is still on it.
            for(int sample=0;sample<9;++sample) {
                Vec3 point=candidate;
                if(sample) {
                    float angle=(sample-1)*PI/4;
                    point.x+=bodyRadius*std::cos(angle);
                    point.z+=bodyRadius*std::sin(angle);
                }
                float y;
                if(heightAt(t,point,y) && y<=previousFloor+stepHeight+.001f
                   && y>=previousFloor-.65f) floor=std::max(floor,y);
            }
        });
        if(!std::isfinite(floor)) return false; // no falling off balconies/site edges
        Vec3 feet{candidate.x,floor,candidate.z}; bool blocked=false;
        nearby(feet,bodyRadius,[&](const Triangle& t) {
            if(!blocked && t.high>floor+stepHeight+.015f && t.low<floor+bodyHeight)
                blocked=overlaps(t,feet);
        });
        if(blocked) return false;
        result={feet.x,feet.y+eyeHeight,feet.z}; return true;
    }
    Vec3 move(Vec3 eye,Vec3 displacement) const {
        displacement.y=0;
        int steps=std::max(1,static_cast<int>(std::ceil(length(displacement)/.06f)));
        Vec3 step=displacement*(1.0f/steps);
        for(int i=0;i<steps;++i) {
            Vec3 next;
            if(supported(eye+step,eye.y-eyeHeight,next)) eye=next;
            else {
                if(supported(eye+Vec3{step.x,0,0},eye.y-eyeHeight,next)) eye=next;
                if(supported(eye+Vec3{0,0,step.z},eye.y-eyeHeight,next)) eye=next;
            }
        }
        return eye;
    }
};
}

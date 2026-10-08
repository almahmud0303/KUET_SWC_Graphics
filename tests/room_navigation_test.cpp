#include "room_navigation.h"
#include "walk_navigation.h"
#include "geometry/canonical_shapes.h"
#include <iostream>
#include <cstdlib>

void check(bool condition,const char* message) {
    if(!condition) { std::cerr<<message<<'\n'; std::exit(1); }
}
void box(swc::WalkNavigation& world,swc::Vec3 center,swc::Vec3 size) {
    using namespace swc;
    Vec3 vertices[3]; int index=0;
    canonical::cube([&](Vec3 p,Vec3 n) {
        vertices[index++]={center.x+p.x*size.x,center.y+p.y*size.y,center.z+p.z*size.z};
        if(index==3) { world.addTriangle(vertices[0],vertices[1],vertices[2],n); index=0; }
    });
}
int main() {
    using namespace swc;
    WalkNavigation world;
    box(world,{0,-.1f,0},{30,.2f,30});
    // A wall with a 1.4 m opening and a lintel: pass through, not through glass/walls.
    box(world,{-3,1.5f,-2},{4.6f,3,.1f});
    box(world,{3,1.5f,-2},{4.6f,3,.1f});
    box(world,{0,2.8f,-2},{1.4f,.4f,.1f});
    auto entered=world.move({0,1.6f,2},{0,0,-7});
    check(entered.z<-4.9f,"Doorway should connect inside and outside");
    check(world.move(entered,{0,0,7}).z>1.9f,"Must be able to leave again");
    check(world.move({2,1.6f,2},{0,0,-7}).z>-2,"Wall must block even a large movement");
    auto slide=world.move({2,1.6f,-1.6f},{1,0,-1});
    check(slide.x>2.9f && slide.z>-2,"Diagonal movement should slide along walls");
    // Three real treads, followed by a raised landing.
    for(int i=0;i<3;++i) box(world,{7,(i+1)*.075f,1-i*.4f},{2,(i+1)*.15f,.4f});
    box(world,{7,.225f,-1},{2,.45f,2});
    auto upstairs=world.move({7,1.6f,2},{0,0,-3});
    check(upstairs.z<-.9f && std::fabs(upstairs.y-2.05f)<.01f,"Stairs should raise eye height");
    auto downstairs=world.move(upstairs,{0,0,3});
    check(downstairs.z>1.9f && std::fabs(downstairs.y-1.6f)<.01f,"Stairs should descend to ground");
    box(world,{-7,1.7f,0},{3,.15f,3});
    check(world.move({-7,1.6f,3},{0,0,-3}).z>1.5f,"Low ceilings must block entry");
    check(world.move({0,1.6f,14},{0,0,10}).z<=15+WalkNavigation::bodyRadius,"Cannot leave the supported site");
    for(float x:rooms::centers) {
        auto a=rooms::fanTransform(x,0),b=rooms::fanTransform(x,PI/2);
        check(length(transformPoint(a,{.8f,0,0})-transformPoint(b,{.8f,0,0}))>1,"Fan blade must rotate");
        check(length(transformPoint(a,{0,0,0})-transformPoint(b,{0,0,0}))<.001f,"Fan hub must stay mounted");
    }
    std::cout<<"Doors, walls, sliding, stairs, headroom, site bounds and fans passed\n";
}

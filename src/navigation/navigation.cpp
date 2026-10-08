#include "navigation/navigation.h"

#include "geometry/mesh.h"
#include "room_navigation.h"
#include "scene/layout.h"
#include "scene/scene.h"
#include "walk_navigation.h"
#include <cstdlib>
#include <iostream>

namespace swc { namespace app {

WalkNavigation walkNavigation;
bool navigationReady=false;
// Build collision from the same world-space triangles that are rendered.
// Animated overhead fans and the separate base-object gallery are excluded.
void buildNavigation() {
    if(navigationReady) return;
    walkNavigation.clear();
    // Continuous campus ground keeps navigation stable across decorative lawn
    // tessellation gaps; rendered walls and slabs still provide collision.
    walkNavigation.addTriangle({-130,0,-140},{130,0,-140},{130,0,95},{0,1,0});
    walkNavigation.addTriangle({-130,0,-140},{130,0,95},{-130,0,95},{0,1,0});
    auto addRange=[&](std::size_t first,std::size_t count) {
        for(std::size_t i=first;i<first+count;i+=3)
            walkNavigation.addTriangle(mesh[i].position,mesh[i+1].position,mesh[i+2].position,mesh[i].normal);
    };
    addRange(0,opaqueCount);
    for(const auto& pane:roomGlass) addRange(pane.first,6);
    navigationReady=true;
}

int checkWalkRoutes() {
        buildScene(); buildNavigation();
        Vec3 eye{ENTRANCE_AXIS,1.62f,8};
        const Vec3 route[]={{ENTRANCE_AXIS,0,-10},{ENTRANCE_AXIS,0,-18.7f},
                            {11.2f,0,-18.7f},{11.2f,0,-23.3f},{11.2f,0,-18.7f},
                            {37,0,-18.7f},{43,0,-18.7f},{43,0,29},{1.2f,0,29},{1.2f,0,8}};
        for(Vec3 target:route) {
            eye=walkNavigation.move(eye,{target.x-eye.x,0,target.z-eye.z});
            std::cout<<"Walk reached "<<eye.x<<", "<<eye.y<<", "<<eye.z<<'\n';
            if(std::hypot(eye.x-target.x,eye.z-target.z)>.15f) {
                std::cerr<<"Route blocked before "<<target.x<<", "<<target.z<<'\n'; return EXIT_FAILURE;
            }
        }
        auto follow=[&](Vec3& position,Vec3 target) {
            position=walkNavigation.move(position,{target.x-position.x,0,target.z-position.z});
            std::cout<<"Additional route reached "<<position.x<<", "<<position.y<<", "<<position.z<<'\n';
            return std::hypot(position.x-target.x,position.z-target.z)<.15f;
        };
        Vec3 courtyardEye{ENTRANCE_AXIS,2.05f,-18.7f};
        for(Vec3 target: {Vec3{1.2f,0,-24},Vec3{-8,0,-24},Vec3{-8,0,-35},Vec3{-8,0,-24},Vec3{1.2f,0,-24},Vec3{1.2f,0,-18.7f},Vec3{11.2f,0,-18.7f}})
            if(!follow(courtyardEye,target)) return EXIT_FAILURE;
        for(float x:rooms::centers) {
            if(!follow(courtyardEye,{x,0,-18.7f}) || !follow(courtyardEye,{x,0,-23.3f})
               || !follow(courtyardEye,{x,0,-18.7f})) return EXIT_FAILURE;
        }
        Vec3 stairEye{1.2f,2.05f,-12.4f};
        if(!follow(stairEye,{4.1f,0,-12.4f})) return EXIT_FAILURE;
        for(int level=0;level<2;++level) {
            for(Vec3 target: {Vec3{4.1f,0,-16.0f},Vec3{6.1f,0,-16.0f},Vec3{6.1f,0,-12.4f},Vec3{4.1f,0,-12.4f}})
                if(!follow(stairEye,target)) return EXIT_FAILURE;
            if(std::fabs(stairEye.y-(2.05f+(level+1)*STOREY_TALL))>.05f) return EXIT_FAILURE;
            if(!follow(stairEye,{1.2f,0,-12.4f}) || !follow(stairEye,{4.1f,0,-12.4f})) return EXIT_FAILURE;
        }
        for(int level=0;level<2;++level) {
            for(Vec3 target: {Vec3{6.1f,0,-12.4f},Vec3{6.1f,0,-16.0f},Vec3{4.1f,0,-16.0f},Vec3{4.1f,0,-12.4f}})
                if(!follow(stairEye,target)) return EXIT_FAILURE;
        }
        Vec3 outsideStair{35,1.62f,CAFE_WORLD_Z-8.7f};
        if(!follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-8.7f})
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-9.9f})
           || !follow(outsideStair,{22.65f,0,CAFE_WORLD_Z-9.9f}) || outsideStair.y<5.5f
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-9.9f})
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-8.7f})
           || !follow(outsideStair,{35,0,CAFE_WORLD_Z-8.7f}) || outsideStair.y>1.7f) return EXIT_FAILURE;
        std::cout<<"Exterior, entrance, courtyard, all rooms and stair round trips passed\n";
        return EXIT_SUCCESS;
}

}} // namespace swc::app

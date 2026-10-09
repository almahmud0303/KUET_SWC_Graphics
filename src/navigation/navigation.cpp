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
        // The shop faces the entrance hall: in through its doorway, round the stand, and out.
        Vec3 shop{1.2f,2.05f,-9.5f};
        if(!follow(shop,{4.7f,0,-9.5f}) || !follow(shop,{4.7f,0,-12.5f}) || !follow(shop,{4.7f,0,-9.5f})
           || !follow(shop,{1.2f,0,-9.5f})) return EXIT_FAILURE;
        // Its fixed glazing must stop a walker who misses the doorway.
        Vec3 glazing{1.2f,2.05f,-12.0f};
        if(follow(glazing,{5.0f,0,-12.0f})) { std::cerr<<"Shop glazing must stop the walker\n"; return EXIT_FAILURE; }
        // Past the shop, along the corridor to the right, round into the hall behind
        // the curved wall, then the side corridor and the stair: up and back down.
        Vec3 stairEye{1.2f,2.05f,-18.7f};
        for(Vec3 target: {Vec3{23.5f,0,-18.7f},Vec3{23.5f,0,-9.4f},Vec3{16.0f,0,-9.4f}})
            if(!follow(stairEye,target)) return EXIT_FAILURE;
        // Into the stair hall, up the curved flight along the wall to the first floor, and back.
        const float upperEye=2.05f+STOREY_TALL;
        const Vec3 sweep[]={{9.78f,0,.02f},{10.62f,0,1.03f},{11.48f,0,1.72f},{12.44f,0,2.26f},{13.48f,0,2.61f},{14.56f,0,2.79f}};
        if(!follow(stairEye,{15.4f,0,-9.4f}) || !follow(stairEye,{15.4f,0,-1.2f}) || !follow(stairEye,{9.9f,0,-1.2f})) return EXIT_FAILURE;
        for(Vec3 target:sweep) if(!follow(stairEye,target)) return EXIT_FAILURE;
        if(!follow(stairEye,{15.6f,0,2.4f}) || !follow(stairEye,{16.2f,0,.5f}) || !follow(stairEye,{16.2f,0,-5.0f})
           || std::fabs(stairEye.y-upperEye)>.05f || !follow(stairEye,{16.0f,0,-9.4f}) || !follow(stairEye,{5.4f,0,-9.4f})
           || !follow(stairEye,{5.4f,0,-13.6f}) || !follow(stairEye,{1.2f,0,-13.6f})) return EXIT_FAILURE;
        if(!follow(stairEye,{5.4f,0,-13.6f}) || !follow(stairEye,{5.4f,0,-9.4f}) || !follow(stairEye,{16.0f,0,-9.4f})
           || !follow(stairEye,{16.2f,0,-5.0f}) || !follow(stairEye,{16.2f,0,.5f}) || !follow(stairEye,{15.6f,0,2.4f})) return EXIT_FAILURE;
        for(int i=5;i>=0;--i) if(!follow(stairEye,sweep[i])) return EXIT_FAILURE;
        if(!follow(stairEye,{9.9f,0,-1.2f}) || std::fabs(stairEye.y-2.05f)>.05f) return EXIT_FAILURE;
        // The open path from the entrance portico into the stair hall.
        Vec3 porch{6.9f,2.05f,-1.9f};
        if(!follow(porch,{10.5f,0,-1.9f})) return EXIT_FAILURE;
        // Corridor-end stair: both storeys up, then all the way down again.
        Vec3 northEye{1.2f,2.05f,-39.0f};
        if(!follow(northEye,{.675f,0,-42.0f})) return EXIT_FAILURE;
        for(int level=0;level<2;++level) {
            for(Vec3 target: {Vec3{.675f,0,-46.1f},Vec3{2.625f,0,-46.1f},Vec3{2.625f,0,-42.0f},Vec3{.675f,0,-42.0f}})
                if(!follow(northEye,target)) return EXIT_FAILURE;
            if(std::fabs(northEye.y-(2.05f+(level+1)*STOREY_TALL))>.05f) return EXIT_FAILURE;
        }
        for(int level=0;level<2;++level)
            for(Vec3 target: {Vec3{2.625f,0,-42.0f},Vec3{2.625f,0,-46.1f},Vec3{.675f,0,-46.1f},Vec3{.675f,0,-42.0f}})
                if(!follow(northEye,target)) return EXIT_FAILURE;
        if(std::fabs(northEye.y-2.05f)>.05f) return EXIT_FAILURE;
        // The glazed cafeteria wall must stop a walker in the corridor.
        Vec3 glazedWall{1.2f,2.05f,-33.0f};
        if(follow(glazedWall,{6.0f,0,-33.0f})) { std::cerr<<"Cafeteria glazing must stop the walker\n"; return EXIT_FAILURE; }
        Vec3 outsideStair{35,1.62f,CAFE_WORLD_Z-8.7f};
        if(!follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-8.7f})
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-9.9f})
           || !follow(outsideStair,{22.65f,0,CAFE_WORLD_Z-9.9f}) || outsideStair.y<5.5f
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-9.9f})
           || !follow(outsideStair,{30.4f,0,CAFE_WORLD_Z-8.7f})
           || !follow(outsideStair,{35,0,CAFE_WORLD_Z-8.7f}) || outsideStair.y>1.7f) return EXIT_FAILURE;
        // Interior halls from the project_3 walkthrough. The games hall is entered by its side door.
        Vec3 games{-1.9f,2.05f,-4.85f};
        for(Vec3 target: {Vec3{-7.3f,0,-4.85f},Vec3{-22,0,-4.85f},Vec3{-22,0,-9.2f},Vec3{-22,0,-4.85f},Vec3{-1.9f,0,-4.85f}})
            if(!follow(games,target)) return EXIT_FAILURE;
        Vec3 dining{1.2f,2.05f,-39.1f};
        for(Vec3 target: {Vec3{6.4f,0,-39.1f},Vec3{6.4f,0,-42.9f},Vec3{19.0f,0,-42.9f},Vec3{21.6f,0,-42.9f},Vec3{19.0f,0,-42.9f}})
            if(!follow(dining,target)) return EXIT_FAILURE;
        // Upper hall: across it past the two timber doors; the courtyard parapet must stop the walker.
        Vec3 upperHallEye{2.6f,2.05f+STOREY_TALL,-7.2f};
        if(!follow(upperHallEye,{-.4f,0,-8.6f}) || !follow(upperHallEye,{1.2f,0,-13.0f})) return EXIT_FAILURE;
        if(follow(upperHallEye,{-3.0f,0,-13.0f})) { std::cerr<<"Upper hall parapet must stop the walker\n"; return EXIT_FAILURE; }
        // The corner link between the three-storey block and the stage wing, on both levels.
        Vec3 lowerLink{-2.4f,2.05f,-30.0f};
        if(!follow(lowerLink,{-2.4f,0,-42.5f}) || !follow(lowerLink,{-2.4f,0,-46.0f}) || !follow(lowerLink,{-12.0f,0,-46.0f})
           || std::fabs(lowerLink.y-2.05f)>.05f) return EXIT_FAILURE;
        Vec3 upperLink{-2.3f,2.05f+STOREY_TALL,-30.0f};
        if(!follow(upperLink,{-2.3f,0,-42.5f}) || !follow(upperLink,{-2.3f,0,-46.0f}) || !follow(upperLink,{-12.0f,0,-46.0f})
           || std::fabs(upperLink.y-(PLINTH+STOREY+WalkNavigation::eyeHeight))>.05f) return EXIT_FAILURE;
        if(!follow(upperLink,{-2.3f,0,-46.0f}) || !follow(upperLink,{-2.3f,0,-30.0f})
           || std::fabs(upperLink.y-(2.05f+STOREY_TALL))>.05f) return EXIT_FAILURE;
        Vec3 terrace{1.2f,2.05f+STOREY_TALL,-30.0f};
        if(!follow(terrace,{-2.3f,0,-30.0f}) || !follow(terrace,{-2.3f,0,-18.0f})
           || std::fabs(terrace.y-(2.05f+STOREY_TALL))>.05f) return EXIT_FAILURE;
        if(follow(terrace,{-6.0f,0,-18.0f})) { std::cerr<<"Terrace parapet must stop the walker\n"; return EXIT_FAILURE; }
        std::cout<<"Exterior, entrance, courtyard, rooms, halls, terrace and stair round trips passed\n";
        return EXIT_SUCCESS;
}

}} // namespace swc::app

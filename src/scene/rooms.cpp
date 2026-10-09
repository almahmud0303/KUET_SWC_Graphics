#include "scene/rooms.h"

#include "geometry/mesh.h"
#include "room_navigation.h"
#include "scene/components.h"
#include "scene/layout.h"

namespace swc { namespace app {

// Reusable room parts, built from canonical boxes/cylinders. The sliding
// door leaf is parked beside the opening, leaving a real walk-through doorway.
void thaiGlassPanel(float x,float y,float z,float width,float height) {
    const Material aluminium{{.55f,.60f,.60f},.7f,INTERIOR};
    for(float side:{-1.0f,1.0f}) {
        box(x+side*width/2,y,z,.055f,height,.07f,aluminium);
        box(x,y+side*height/2,z,width,.055f,.07f,aluminium);
    }
    roomGlass.push_back({{x,y,z},width-.055f,height-.055f});
}
void ceilingFanMount(float x,float z) { ceilingFan(x,rooms::ceiling,z); }
// Fixed canopy and down-rod; the shared rotor mesh is instanced at the hub.
void ceilingFan(float x,float ceilingY,float z) {
    const Material enamel{{.82f,.82f,.76f},.4f,INTERIOR};
    cylinder(x,ceilingY-.08f,z,.16f,.12f,.16f,enamel);
    cylinder(x,ceilingY-.35f,z,.035f,.5f,.035f,enamel);
    fanPositions.push_back({x,ceilingY-.64f,z});
}
void room(float x,Color accent) {
    using namespace rooms;
    const Material plaster{{.87f,.86f,.78f},.08f,INTERIOR};
    const Material paint{accent,.08f,INTERIOR};
    const Material furniture{{.48f,.30f,.16f},.25f,INTERIOR};
    block(x-halfWidth,0,back,x+halfWidth,floorY,front,walkway);
    block(x-halfWidth,ceiling-.025f,back,x+halfWidth,ceiling+.15f,front,plaster);
    block(x-halfWidth,floorY,back,x+halfWidth,ceiling,back+.12f,paint);
    for(float side:{-1.0f,1.0f})
        box(x+side*(halfWidth-.06f),(floorY+ceiling)/2,(front+back)/2,.12f,ceiling-floorY,front-back,plaster);
    // Fixed glazing on each side of a 1.4 m entrance, with a transom above.
    for(float side:{-1.0f,1.0f}) {
        float px=x+side*1.625f;
        box(px,.72f,front,1.85f,.54f,.12f,plaster);
        thaiGlassPanel(px,1.97f,front,1.85f,1.96f);
    }
    thaiGlassPanel(x,3.36f,front,5.0f,.76f);
    thaiGlassPanel(x-1.43f,1.70f,front-.09f,1.35f,2.45f);
    box(x-.89f,1.6f,front+.005f,.035f,.32f,.07f,metal);
    box(x,2.96f,front,1.48f,.07f,.1f,metal);
    ceilingFanMount(x,-23.55f);
    box(x,3.70f,-25.7f,1.15f,.045f,.40f,tubeLight);
    sceneLights.push_back({{x,3.35f,-24.6f},{1.15f,1.12f,1.02f},7.5f});
    // Desk at the back leaves the doorway and central walking area clear.
    box(x,1.22f,back+1.25f,2.1f,.10f,.85f,furniture);
    for(float dx:{-.9f,.9f}) for(float dz:{-.32f,.32f})
        box(x+dx,.82f,back+1.25f+dz,.065f,.74f,.065f,metalDark);
    box(x,.94f,back+2.05f,.56f,.10f,.55f,paint);
    box(x,1.26f,back+2.30f,.56f,.65f,.07f,paint);
    for(float dx:{-.23f,.23f}) for(float dz:{-.22f,.22f})
        box(x+dx,.69f,back+2.05f+dz,.04f,.48f,.04f,metalDark);
    box(x,2.35f,back+.14f,2.5f,1.05f,.05f,Material{{.14f,.28f,.24f},.1f,INTERIOR});
    box(x,2.35f,back+.18f,2.32f,.88f,.025f,Material{{.88f,.90f,.84f},.1f,INTERIOR});
}
void buildRoomDrawRanges() {
    opaqueCount=static_cast<GLsizei>(mesh.size());
    fanFirst=opaqueCount;
    const Material enamel{{.72f,.72f,.66f},.35f,INTERIOR};
    cylinder(0,0,0,.19f,.16f,.19f,enamel,24);
    for(int i=0;i<3;++i) {
        Place blade(rotationY(radians(i*120.0f)));
        box(.61f,.025f,0,.91f,.035f,.19f,enamel);
    }
    fanCount=static_cast<GLsizei>(mesh.size())-fanFirst;
    const Material glass{{.63f,.85f,.87f},.8f,TRANSPARENT_GLASS};
    for(auto& pane:roomGlass) {
        pane.first=static_cast<GLint>(mesh.size());
        Vec3 c=pane.center; float w=pane.width/2,h=pane.height/2;
        if(pane.alongZ) addQuad({c.x,c.y-h,c.z-w},{c.x,c.y-h,c.z+w},{c.x,c.y+h,c.z+w},{c.x,c.y+h,c.z-w},glass);
        else addQuad({c.x-w,c.y-h,c.z},{c.x+w,c.y-h,c.z},
                     {c.x+w,c.y+h,c.z},{c.x-w,c.y+h,c.z},glass);
    }
}

}} // namespace swc::app

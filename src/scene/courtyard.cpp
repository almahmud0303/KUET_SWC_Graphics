#include "scene/courtyard.h"

#include "geometry/mesh.h"
#include "room_navigation.h"
#include "scene/components.h"
#include "scene/front.h"
#include "scene/layout.h"
#include "scene/lettering.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Courtyard wings, stage and building masses
// ---------------------------------------------------------------------------
// Photo 3.07.59 AM: four framed bays beside three open balcony bays.
void stageCourtyardWing() {
    Place wing(translation({STAGE_WING_LEFT,0,-44.3f}));
    const float length=7*STAGE_BAY,first=PLINTH+STOREY,top=PLINTH+2*STOREY;
    const Material pale{{.79f,.82f,.77f}}, oldBrick{{.43f,.23f,.16f},.04f,BRICK};
    block(-.25f,0,-3.8f,length+.25f,PLINTH,.5f,walkway);
    block(-.3f,first-.30f,-3.8f,length+.3f,first,.35f,pale);
    block(-.4f,top,-4.1f,length+.4f,top+.35f,1.1f,pale);
    for(int b=0;b<7;++b) {
        float x=b*STAGE_BAY;
        { Place edge(translation({0,0,.35f})); balconyPanel(x+.20f,x+STAGE_BAY-.20f,first); }
        if(b<4) {
            // Pale lower wall, narrow barred clerestories and paired brick
            // panels on the upper wall, behind the projecting white fins.
            block(x,PLINTH,-.65f,x+STAGE_BAY,2.40f,-.40f,pale);
            block(x,3.18f,-.65f,x+STAGE_BAY,first-.30f,-.40f,pale);
            for(int w=0;w<4;++w) {
                float a=x+.25f+w*1.27f;
                block(a,2.43f,-.62f,a+1.12f,3.15f,-.58f,glassFor(701+b*4+w));
                for(int bar=0;bar<=8;++bar)
                    box(a+bar*.14f,2.79f,-.36f,.016f,.72f,.025f,metal);
                box(a+.56f,2.79f,-.35f,1.12f,.018f,.025f,metal);
            }
            block(x,first,-3.6f,x+STAGE_BAY,top,-3.3f,pale);
            for(float dx:{.30f,3.25f}) for(float y:{first+.2f,first+1.9f})
                block(x+dx,y,-3.29f,x+dx+1.70f,y+1.33f,-3.22f,oldBrick);
            // Narrow exposed rainwater pipes down the pale ground wall.
            rod({x+.65f,PLINTH,-.28f},{x+.65f,3.55f,-.28f},.047f,oldBrick,10);
        } else {
            // Recessed room fronts leave a continuous open gallery at both levels.
            block(x,PLINTH,-3.8f,x+STAGE_BAY,top,-3.5f,pale);
            for(float floor:{PLINTH,first}) {
                Place back(translation({0,0,-3.2f}));
                windowBay(x+1.9f,floor,3.15f,730+b+floor);
                door(x+4.40f,floor,740+b+floor,1.15f);
            }
        }
        cylinder(x+STAGE_BAY/2,top-.08f,-1.3f,.075f,.15f,.075f,lampBlack,10);
    }
    for(int b=0;b<=7;++b) {
        float x=b*STAGE_BAY;
        box(x,(PLINTH+top)/2,.06f,.34f,top-PLINTH,.42f,pale);
        if(b<=4) {
            // Full-height projecting fins, linked to the rear roof with a beam.
            block(x-.18f,PLINTH,.2f,x+.18f,top,1.05f,pale);
            block(x-.18f,top-.38f,-3.45f,x+.18f,top,1.05f,pale);
        }
    }
    // Side access for the open ground gallery.
    stair(length-2.6f,1.49f,2.8f,3,.15f,.33f,0,trim);
}

void courtyard() {
    // Left side stays open to the lawn, per the corrected site layout.
    // North: stage wing with balcony corridors and projecting frames (13, 14, 21).
    stageCourtyardWing();
    // Shorten the courtyard return so it stops beside the aligned corridor.
    { Place p(trs({COURTYARD_RIGHT,0,-14.5f},180));
      galleryWing({4,(COURTYARD_RIGHT+19.5f)/4,2,STOREY_TALL,3.2f,0,0,0x01u,true,0},51.0f); }
    entranceCorridor();
    // Move corridor, rooms, stairs, and roof details as one complete block.
    { Place aligned(translation(TALL_BLOCK_OFFSET));
      { Place p(trs({17.3f,0,-44.3f},-90));
        galleryWing({6,29.8f/6,3,STOREY_TALL,3.2f,0x15u,0x10u,0x20u,true,0},41.0f); }
      connectedStairwell();
      block(17.3f,0,-55.25f,21.25f,11.7f,-44.3f,concrete);
      // True ground-floor opening on the new doorway axis, through the room mass.
      const float passageBack=ENTRY_BACK_Z-TALL_BLOCK_OFFSET.z;
      const float passageFront=ENTRY_FRONT_Z-TALL_BLOCK_OFFSET.z;
      // Keep the upper floors and rear mass; hollow the ground floor for rooms.
        const float roomBack=rooms::back-TALL_BLOCK_OFFSET.z;
        block(21.25f,0,CAFETERIA_FRONT_Z,25.45f+TALL_EXTRA_WIDTH,11.7f,roomBack,concrete);
        block(21.25f,rooms::ceiling,roomBack,25.45f+TALL_EXTRA_WIDTH,11.7f,passageBack,concrete);
      block(21.25f,0,passageFront,25.45f+TALL_EXTRA_WIDTH,11.7f,-20.0f,concrete);
      block(21.25f,PLINTH+STOREY_TALL-.28f,passageBack,25.45f+TALL_EXTRA_WIDTH,11.7f,passageFront,concrete);
      block(21.25f,0,CAFETERIA_REAR_Z,26.5f+TALL_EXTRA_WIDTH,11.7f,CAFETERIA_FRONT_Z,concrete);
      block(21.25f,0,-55.25f,25.45f+TALL_EXTRA_WIDTH,11.7f,CAFETERIA_REAR_Z,concrete);
      // New front rooms fill the widened area beside the unchanged stairwell.
      block(25.5f,0,-20.0f,TALL_EXTERIOR_X-.75f,11.7f,-14.95f,concrete);
      { Place front(trs({25.5f,0,-14.2f},0));
        windowedWing({3,(TALL_EXTERIOR_X-25.5f)/3,3,STOREY_TALL,3,1.0f,false,false,false,true},121.0f); }
      block(22.0f,11.7f,-40.0f,25.0f,13.6f,-36.0f,concrete);
      block(21.8f,13.6f,-40.2f,25.2f,13.8f,-35.8f,trim);
    }
    block(STAGE_WING_LEFT,0,-55.25f,COURTYARD_RIGHT,7.8f,-48.25f,concrete);
    // Roof parapet edges and rooftop stair/water tank housings (photos 21, 22).
    block(-8.0f,7.8f,-54.0f,-4.0f,10.2f,-50.0f,concrete);
}

void stage() {
    const float cx=STAGE_CENTER;
    block(cx-4.0f,0,-43.1f,cx+4.0f,4.35f,-42.8f,brick);
    block(cx-4.10f,4.35f,-43.15f,cx+4.10f,4.45f,-42.75f,brickDark);
    // Raised platform: circular segment with a low curved brick edge.
    const float chordZ=-42.8f,half=6.8f,sag=1.1f,r=(half*half+sag*sag)/(2*sag),centerZ=chordZ+sag-r;
    const float span=std::asin(half/r),h=.55f; const int n=40;
    for(int i=0;i<n;++i) {
        float a=-span+2*span*i/n,b=-span+2*span*(i+1)/n;
        Vec3 pa{cx+(r-.3f)*std::sin(a),h,centerZ+(r-.3f)*std::cos(a)},pb{cx+(r-.3f)*std::sin(b),h,centerZ+(r-.3f)*std::cos(b)};
        addTriangle({cx,h,chordZ},pb,pa,{0,1,0},{{.80f,.78f,.72f},.1f});
        Vec3 oa{cx+r*std::sin(a),0,centerZ+r*std::cos(a)},ob{cx+r*std::sin(b),0,centerZ+r*std::cos(b)};
        Vec3 ia{cx+(r-.3f)*std::sin(a),0,centerZ+(r-.3f)*std::cos(a)},ib{cx+(r-.3f)*std::sin(b),0,centerZ+(r-.3f)*std::cos(b)};
        Vec3 up{0,h+.06f,0};
        addQuad(oa,ob,ob+up,oa+up,{std::sin((a+b)/2),0,std::cos((a+b)/2)},brickLight);
        addQuad(oa+up,ob+up,ib+up,ia+up,{0,1,0},brickLight);
    }
    block(cx-half,0,chordZ-.1f,cx+half,h,chordZ,{{.80f,.78f,.72f},.1f});
    { Place p(trs({cx+6.8f,0,-42.3f},90)); stair(0,0,1.8f,3,.18f,.30f,0,trim); }
}

}} // namespace swc::app

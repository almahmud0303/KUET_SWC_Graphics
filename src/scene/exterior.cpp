#include "scene/exterior.h"

#include "geometry/mesh.h"
#include "scene/components.h"
#include "scene/layout.h"
#include "scene/lettering.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Exterior elevations, shopfront and cafeteria
// ---------------------------------------------------------------------------
// project_2, 6.42.27--6.42.34: low shop pavilion running back from the
// curved sign. Local +X follows the road toward the cafeteria; +Z faces it.
// The blank upper wall, deep eaves and open end balconies are distinctive.
void rightSideWing() {
    // Shorter, narrower stalls tucked behind the oval; inner edge x=22.34.
    Place wing(trs({27.8f,0,-5.8f},90,{.68f,1,.78f}));
    const float end=22.0f,first=PLINTH+STOREY_TALL,top=PLINTH+2*STOREY_TALL;
    const float roomStart=3.8f,roomEnd=16.0f;
    block(-.35f,0,-7.0f,end+.35f,PLINTH,2.75f,walkway);
    block(-.35f,PLINTH-.12f,2.65f,end+.35f,PLINTH,2.80f,trim);
    block(-.55f,top,-7.2f,end+.65f,top+.36f,3.45f,trim);
    // Slender, double-height columns stand clear of the recessed room wall.
    for(float x : {0.0f,8.0f,16.0f,end}) {
        block(x-.25f,PLINTH,2.05f,x+.25f,top-.22f,2.55f,concrete);
        block(x-.21f,top-.22f,2.09f,x+.21f,top,2.51f,concrete);
        block(x-.29f,PLINTH,2.01f,x+.29f,PLINTH+.17f,2.59f,trim);
    }
    for(float x=1.3f;x<end;x+=3.0f)
        cylinder(x,top-.15f,2.0f,.085f,.15f,.085f,lampBlack,12);

    // Solid upper room volume; two very fine horizontal plaster joints.
    // Reserve the inner strip for the passage beside the taller block.
    block(roomStart,first,-3.2f,roomEnd,top,-.32f,concrete);
    block(roomStart,first-.28f,-3.2f,roomEnd,first,.04f,trim);
    for(float y : {first+.62f,first+1.22f})
        block(roomStart,y,-.325f,roomEnd,y+.014f,-.31f,roofMembrane);
    // End windows face along the road, with brick panels below the grilles.
    { Place endWall(trs({roomStart,0,-.65f},-90));
      windowBay(1.55f,first,2.25f,601.0f);
      block(.2f,first+3.0f,-.35f,3.0f,top,0,concrete); }
    { Place endWall(trs({roomEnd,0,-3.7f},90));
      windowBay(1.55f,first,2.25f,602.0f);
      block(.2f,first+3.0f,-.35f,3.0f,top,0,concrete); }

    // Two counters shifted left, leaving the right-hand front bays open
    // straight through to the corridor. No shutter, waiting seats or bin.
    for(int b=0;b<2;++b) {
        float a=roomStart+b*6.1f,bx=a+6.1f;
        block(a,PLINTH,-.35f,a+.30f,first,.02f,concrete);
        block(a+.3f,PLINTH,-3.0f,bx-.15f,first-.5f,-2.92f,darkGap);
        block(a+.3f,first-.75f,-2.89f,bx-.15f,first-.43f,-2.83f,signDark);
        {
            block(a+.4f,PLINTH,-2.9f,bx-.25f,PLINTH+.85f,-2.2f,wood);
            block(a+.35f,PLINTH+.85f,-2.98f,bx-.20f,PLINTH+.92f,-2.13f,trim);
            for(int shelf=0;shelf<3;++shelf) {
                float y=PLINTH+1.1f+shelf*.52f;
                block(a+.4f,y,-2.90f,bx-.3f,y+.05f,-2.6f,wood);
                for(int k=0;k<11;++k) {
                    Material packet{k%3==0 ? Color{.78f,.16f,.08f} :
                                    (k%3==1 ? Color{.85f,.65f,.15f} : Color{.19f,.48f,.25f}),.15f};
                    block(a+.58f+k*.43f,y+.05f,-2.73f,a+.79f+k*.43f,y+.35f,-2.52f,packet);
                }
            }
        }
        // Small transverse beams behind the shopfront lintel.
        block(a+.1f,first-.45f,-3.0f,a+.22f,first-.28f,-.1f,trim);
    }
    block(roomEnd-.15f,PLINTH,-.35f,roomEnd+.15f,first,.02f,concrete);
    // Broad frontal access, aligned with the clear bay rather than the end gate.
    stair(19.0f,3.65f,4.6f,3,.15f,.30f,0,trim);

    // Open upper balconies at both ends, with brick inset parapets and rails.
    for(int side=0;side<2;++side) {
        float a=side==0 ? 0.0f : roomEnd,b=side==0 ? roomStart : end;
        block(a,first-.28f,-6.5f,b,first,-.05f,concrete);
        { Place edge(translation({0,0,-.05f})); balconyPanel(a+.25f,b-.25f,first); }
        for(float x : {a,b}) block(x-.2f,PLINTH,-.4f,x+.2f,top,0,concrete);
        { Place back(translation({0,0,-3.1f}));
          // Keep the space behind the oval empty at ground level.
          block(a,first,-.4f,b,top,-.34f,concrete);
          windowBay((a+b)/2,first,std::min(3.2f,b-a-.8f),625+side); }
    }
    // Rear balcony of the front shop wing, separate from the rear cafeteria.
    // Only the shop-side return; frontGapCorridor builds the adjoining balcony.
    { Place rear(trs({end,0,-6.5f},90)); balconyPanel(3.3f,6.2f,first); }
    block(end-1.25f,first-.25f,-6.5f,end,first,0,concrete);
    // Broad rear steps open onto the side garden path.
    { Place rear(trs({end+.35f,0,-1.5f},90)); stair(0,1.1f,7.8f,3,.15f,.33f,0,trim); }
    // Short return from the curved brick edge to the pavilion's first balcony.
    block(-.35f,first-.28f,-7.0f,0,first,-.05f,concrete);
}

// Covered passage replacing the grass slot behind the oval/front rooms.
// Its balcony and grilled entrance follow project_2 6.42.32 PM.
void frontGapCorridor() {
    const float left=3.5f,right=25.3f,back=-11.3f,front=-7.0f;
    const float first=PLINTH+STOREY_TALL,roof=PLINTH+2*STOREY_TALL;
    block(left,0,back,right,PLINTH,front,walkway);
    block(left,first-.28f,back,right,first,front,concrete);
    block(left,first,back,right,first+.015f,front,walkway);
    block(left,roof,back,right,roof+.36f,front,trim);
    for(float x=4.0f;x<right;x+=4.4f) {
        block(x-.14f,PLINTH,front-.3f,x+.14f,roof,front,concrete);
        for(float ceiling : {first-.28f,roof}) {
            block(x-.12f,ceiling-.22f,back,x+.12f,ceiling,front,concrete);
            cylinder(x+1.2f,ceiling-.15f,-8.9f,.08f,.15f,.08f,lampBlack,12);
        }
    }
    for(float level : {PLINTH,first}) {
        block(left,level,back+.1f,right,level+.012f,back+.23f,darkTile);
        block(left,level,front-.23f,right,level+.012f,front-.1f,darkTile);
        for(float x=4.8f;x<right;x+=2.4f)
            block(x,level,-9.3f,x+.48f,level+.012f,-8.82f,darkTile);
    }
    // Longitudinal return fills the marked strip beside the shop wing.
    // It meets the transverse passage at z=-11.3 without a wall or step.
    const float inner=21.75f,end=-20.76f,width=right-inner;
    block(inner,0,end,right,PLINTH,back,walkway);
    block(inner,first-.28f,end,right,first,back,concrete);
    block(inner,first,end,right,first+.015f,back,walkway);
    block(inner,roof,end,right,roof+.36f,back,trim);
    for(float level : {PLINTH,first}) {
        for(float x : {inner+.12f,right-.22f})
            block(x,level,end,x+.10f,level+.018f,back,darkTile);
        for(float z=end+.8f;z<back;z+=2.0f)
            block(23.2f,level,z,23.68f,level+.018f,z+.48f,darkTile);
    }
    for(float z=end+.2f;z<back;z+=3.1f) {
        if(z<ENTRY_BACK_Z||z>ENTRY_FRONT_Z)
            block(right-.15f,PLINTH,z-.12f,right,roof,z+.12f,concrete);
        for(float ceiling : {first-.28f,roof}) {
            block(inner,ceiling-.22f,z-.12f,right,ceiling,z+.12f,concrete);
            cylinder(23.5f,ceiling-.15f,z+.7f,.08f,.15f,.08f,lampBlack,12);
        }
    }
    // Rear-facing entrance and balcony at the end of the continuous route.
    block(inner,0,end-5.0f,31.0f,.035f,end-1.05f,paving);
    Place entry(trs({right,0,end},180));
    balconyPanel(.15f,width-.15f,first);
    for(float x : {.12f,width-.12f}) block(x-.12f,PLINTH,-.18f,x+.12f,roof,.16f,trim);
    block(.12f,first-.5f,-.2f,width-.12f,first-.28f,.2f,trim);
    auto grille=[&](float width,float height) {
        for(float x=0;x<=width;x+=.16f) rod({x,PLINTH,0},{x,PLINTH+height,0},.014f,metal,6);
        for(float y=PLINTH;y<=PLINTH+height;y+=.18f) rod({0,y,0},{width,y,0},.014f,metal,6);
    };
    { Place fixed(translation({.26f,0,.06f})); grille(1.35f,2.65f); }
    { Place open(trs({width-.26f,0,.06f},90)); grille(1.3f,2.65f); }
    for(float y=PLINTH+2.8f;y<first-.52f;y+=.15f)
        rod({.26f,y,.06f},{width-.26f,y,.06f},.014f,metal,6);
    stair(width/2,1.05f,width,3,.15f,.3f,0,trim);
}

// A straight route on the relocated door axis, through the removed ground
// window bay and room block, joining the courtyard corridor at the far end.
void straightEntryCorridor() {
    const float x0=COURTYARD_RIGHT-.3f,x1=27.8f;
    const float ceiling=PLINTH+STOREY_TALL-.28f;
    block(x0,0,ENTRY_BACK_Z,x1,PLINTH,ENTRY_FRONT_Z,walkway);
    block(x0,ceiling-.015f,ENTRY_BACK_Z,x1,ceiling+.28f,ENTRY_FRONT_Z,concrete);
    for(float z : {ENTRY_BACK_Z+.12f,ENTRY_FRONT_Z-.22f})
        block(x0,PLINTH,z,x1,PLINTH+.024f,z+.10f,darkTile);
    for(float x=x0+1;x<x1;x+=2.4f)
        block(x,PLINTH,-18.95f,x+.45f,PLINTH+.024f,-18.50f,darkTile);
    for(float x=1.0f;x<26.0f;x+=3.5f) {
        block(x-.1f,ceiling-.2f,ENTRY_BACK_Z,x+.1f,ceiling,ENTRY_FRONT_Z,concrete);
        cylinder(x+1.1f,ceiling-.15f,-18.725f,.08f,.15f,.08f,lampBlack,12);
    }
}

// Street-facing entrance from 6.42.32 PM: open grille doors below the
// brick-inset balcony, with transom bars and diagonal soffit beams.
void photoCorridorEntrance() {
    Place face(trs({27.65f,0,ENTRY_FRONT_Z},90));
    const float width=3.05f,first=PLINTH+STOREY_TALL;
    const Material grilleMetal{{.57f,.59f,.53f},.35f};
    auto grille=[&](float w,float h) {
        for(float x : {0.0f,w}) block(x-.025f,PLINTH,-.025f,x+.025f,PLINTH+h,.025f,grilleMetal);
        for(float y=PLINTH;y<PLINTH+h;y+=.13f)
            block(0,y,-.018f,w,y+.018f,.018f,grilleMetal);
        for(float x=.18f;x<w;x+=.35f)
            block(x,PLINTH,-.02f,x+.018f,PLINTH+h,.02f,grilleMetal);
    };
    { Place fixed(translation({.05f,0,0})); grille(1.20f,2.45f); }
    { Place opened(trs({width-.08f,0,0},90)); grille(1.05f,2.45f); }
    for(float y=PLINTH+2.55f;y<first-.31f;y+=.15f)
        block(.05f,y,-.02f,width-.05f,y+.018f,.02f,grilleMetal);
    block(0,PLINTH+2.48f,-.06f,width,PLINTH+2.55f,.06f,grilleMetal);
    for(float x : {0.0f,width}) block(x-.06f,PLINTH,-.08f,x+.06f,first-.28f,.08f,trim);
    for(float x=0;x<width;x+=1.0f) {
        rod({x,first-.3f,0},{x+.48f,first-.3f,-.8f},.04f,trim,6);
        rod({x+.48f,first-.3f,-.8f},{std::min(x+1.0f,width),first-.3f,0},.04f,trim,6);
    }
    // Small pots flank the clear opening without occupying its threshold.
    for(float x : {-.22f,width+.22f}) {
        frustum({x,PLINTH,.28f},.11f,.16f,.23f,redPaint,12);
        for(int k=0;k<5;++k) {
            float a=2*PI*k/5;
            rod({x,PLINTH+.23f,.28f},{x+.16f*std::cos(a),PLINTH+.62f,.28f+.16f*std::sin(a)},.018f,grass,5);
        }
    }
}

void cafeteria() {
    // Facing the entrance: cafeteria left, external stair and low block right.
    // Local frame: x along the facade in [0,13], +Z outward (photo 12).
    const float f1=PLINTH+STOREY_TALL,f2=PLINTH+2*STOREY_TALL,top=PLINTH+3*STOREY_TALL;
    // Left windowed section with brick infill.
    block(0,PLINTH,-.75f,3.2f,top,-.34f,concrete);
    for(float f : {PLINTH,f1,f2}) {
        windowBay(1.6f,f,1.9f,500.0f+f);
        block(0,f+3.0f,-.42f,3.2f,std::min(top,f+STOREY_TALL),.12f,concrete);
    }
    block(2.9f,0,-.4f,3.5f,top+SLAB,.9f,trim);                         // tall pier left of entrance
    // Recessed entrance under the signed canopy.
    block(3.5f,0,-2.5f,8.8f,PLINTH,.45f,walkway);
    stair(6.15f,1.35f,5.6f,3,.15f,.30f,0,trim);
    block(3.5f,PLINTH,-2.9f,8.8f,f1,-2.5f,concrete);
    block(5.3f,PLINTH,-2.52f,6.9f,PLINTH+2.8f,-2.45f,darkGap);
    block(5.2f,PLINTH,-2.5f,5.3f,PLINTH+2.9f,-2.4f,wood); block(6.9f,PLINTH,-2.5f,7.0f,PLINTH+2.9f,-2.4f,wood);
    { Place p(translation({0,0,-2.15f})); windowBay(4.25f,PLINTH,1.2f,510.0f,false); windowBay(7.95f,PLINTH,1.2f,511.0f,false); }
    block(8.8f,0,-2.9f,9.1f,f1,0,concrete);
    block(3.3f,f1-.7f,-2.5f,9.1f,f1,1.4f,trim);
    flatText(latinGlyphs("CAFETERIA"),6.2f,f1-.55f,1.4f,.075f,.085f,.05f,signDark);
    flatText(bengaliGlyphs({8},23.0f),6.2f,f1+.08f,1.33f,.07f,.07f,.05f,signDark);
    for(float x=3.4f;x<9.05f;x+=1.1f) rod({x,f1,1.3f},{x,f1+1.05f,1.3f},.025f,metal,6);
    block(3.4f,f1+.1f,1.24f,9.0f,f1+1.0f,1.28f,clearGlass);
    rod({3.4f,f1+1.05f,1.3f},{9.0f,f1+1.05f,1.3f},.03f,metal,6);
    // Upper levels above the entrance: windows and the tall central louver.
    block(3.5f,f1,-.75f,8.8f,top,-.34f,concrete);
    for(float f : {f1,f2}) {
        windowBay(4.3f,f,1.3f,520.0f+f); windowBay(7.95f,f,1.3f,530.0f+f);
        block(3.5f,f+3.0f,-.42f,8.8f,std::min(top,f+STOREY_TALL),.12f,concrete);
    }
    block(5.05f,f1+.3f,-.3f,7.15f,top-.1f,.28f,trim);
    block(5.2f,f1+.45f,.28f,7.0f,top-.25f,.29f,darkGap);
    for(float y=f1+.5f;y<top-.3f;y+=.15f) block(5.2f,y,.29f,7.0f,y+.05f,.46f,trim);
    // Straight external staircase with a brick side wall and planter.
    const int steps=24; const float riser=f1/steps,tread=.30f,front=7.4f;
    stair(9.9f,front,1.4f,steps,riser,tread,0,trim);
    const float back=front-steps*tread;
    for(float x : {10.6f,10.8f})
        addQuad({x,0,front},{x,0,back},{x,f1+.70f,back},{x,.70f,front},
                {x<10.7f ? -1.0f : 1.0f,0,0},brickLight);
    addQuad({10.6f,.70f,front},{10.8f,.70f,front},{10.8f,f1+.70f,back},{10.6f,f1+.70f,back},trim);
    block(10.6f,0,front-.03f,10.8f,.70f,front,brickLight);
    for(float h : {.55f,.78f,1.02f})
        rod({9.25f,h,front},{9.25f,f1+h,back},.022f,metal,6);
    for(int i=0;i<=steps;i+=3) {
        float z=front-i*tread,y=i*riser;
        rod({9.25f,y,z},{9.25f,y+1.02f,z},.023f,metal,6);
    }
    block(9.1f,f1-.3f,-.6f,12.4f,f1,.3f,concrete);
    // Upper landing continues sideways into the adjoining low block.
    block(10.6f,f1-.28f,0,12.4f,f1,1.5f,concrete);
    { Place p(trs({12.4f,0,.3f},90)); balconyPanel(0,.9f,f1); }
    // Leave a usable approach between the planter and the first stair tread.
    block(9.3f,0,front+.9f,10.6f,.55f,front+2.0f,trim);
    for(int k=0;k<7;++k) {
        float a=2*PI*k/7;
        rod({9.95f,.55f,front+1.45f},{9.95f+std::cos(a)*.45f,1.15f,front+1.45f+std::sin(a)*.45f},.03f,{{.35f,.55f,.25f}},4);
    }
    // Right windowed block and roof.
    block(10.8f,PLINTH,-.75f,13.0f,top,-.34f,concrete);
    for(float f : {f1,f2}) windowBay(11.9f,f,1.4f,540.0f+f);
    { Place p(translation({0,0,-2.0f})); door(11.9f,PLINTH,545.0f,1.1f,2.4f); }
    block(10.8f,0,-2.5f,13.0f,PLINTH,.45f,walkway);
    block(12.7f,0,-.4f,13.3f,top+SLAB,.6f,trim);
    block(-.3f,top,-2.9f,13.3f,top+SLAB,1.0f,trim);
}

// The return beside the cafeteria, project_2 6.42.34 PM (1) and (2).
// Work in the cafeteria facade frame: +X is the viewer's right, +Z is
// outward across the forecourt. This keeps the stair and balcony connected.
void cafeteriaRightBlock() {
    const float f=PLINTH+STOREY_TALL,roof=PLINTH+2*STOREY_TALL;
    const float left=10.8f,right=23.0f,edge=9.4f;
    block(left,0,-.1f,right,PLINTH,edge,walkway);
    block(left,roof,-.2f,right+.5f,roof+.36f,edge+.65f,trim);
    // Front corner pillar and the recessed porch supports.
    for(float x : {left,right})
        block(x-.25f,PLINTH,edge-.55f,x+.25f,roof,edge-.05f,concrete);
    block(left,PLINTH,5.5f,left+.35f,roof,5.85f,concrete);
    // The first bay is an open porch and balcony next to the stair.
    block(left,f-.28f,0,14.6f,f,6.0f,concrete);
    { Place face(trs({left,0,.15f},-90));
      balconyPanel(1.6f,5.65f,f); // gap at the stair landing
      for(float y=f+1.15f;y<roof-.2f;y+=.19f)
          rod({1.6f,y,-.08f},{5.65f,y,-.08f},.015f,metal,6);
      for(float x=1.6f;x<5.7f;x+=1.0f)
          rod({x,f+.85f,-.08f},{x,roof-.1f,-.08f},.025f,metal,6); }
    { Place face(translation({0,0,6.0f})); balconyPanel(left+.35f,14.5f,f); }
    block(14.3f,PLINTH,0,14.6f,roof,6.0f,concrete);
    { Place back(translation({0,0,.45f}));
      block(left,PLINTH,-.4f,14.6f,roof,-.34f,concrete);
      door(12.4f,PLINTH,650,1.5f,2.5f);
      door(12.4f,f,651,1.4f,2.4f); }
    // Windowed two-storey rooms beyond the open corner, facing the road.
    block(14.6f,0,0,right,roof,5.9f,concrete);
    { Place facade(translation({14.6f,0,6.65f}));
      for(float level : {PLINTH,f}) {
          for(int bay=0;bay<2;++bay) {
              windowBay(bay*4.2f+1.15f,level,1.55f,660+bay+level);
              windowBay(bay*4.2f+3.05f,level,1.55f,670+bay+level);
              block(bay*4.2f+1.93f,level,-.4f,bay*4.2f+2.27f,level+3,0,concrete);
          }
          block(0,level+3,-.4f,8.4f,level+STOREY_TALL,0,concrete);
      }
      for(float x : {0.0f,4.2f,8.4f}) block(x-.2f,PLINTH,-.4f,x+.2f,roof,.1f,concrete);
    }
    { Place endFace(trs({right,0,6.0f},90));
      for(float level : {PLINTH,f}) windowBay(2.9f,level,3.8f,675+level); }
    stair(12.6f,edge+.95f,3.9f,3,.15f,.32f,0,trim);
    for(float x : {12.5f,16.7f,21.0f})
        cylinder(x,roof-.15f,7.9f,.085f,.15f,.085f,lampBlack,12);
    // Open rectangular roof frame above the stair/balcony junction.
    block(left,roof,.3f,left+.3f,roof+1.0f,.65f,trim);
    block(left,roof,5.4f,left+.3f,roof+1.0f,5.75f,trim);
    block(left,roof+.75f,.3f,left+.3f,roof+1.0f,5.75f,trim);
}

void exteriorElevations() {
    // No exterior building wing along the open left side.
    // North exteriors: two-level wing and three-level block.
    { Place p(trs({COURTYARD_RIGHT,0,-56},180)); windowedWing({7,(COURTYARD_RIGHT-STAGE_WING_LEFT)/7,2,STOREY,3,2.2f,false,true,false,true},71.0f); }
    { Place aligned(translation(TALL_BLOCK_OFFSET));
    { Place p(trs({TALL_EXTERIOR_X,0,-56},180)); windowedWing({4,(TALL_EXTERIOR_X-17.3f)/4,3,STOREY_TALL,3,2.2f,false,true,false,true},81.0f); }
    // The area behind the curved sign is the open entranceCorridor, not a solid wing.
    // East exterior: cafeteria and taller gallery block (8-12).
    // Fill the old cafeteria position with continuous gallery bays and keep
    // the rear gallery clear of the relocated entrance, stairs, and signage.
    { Place p(trs({TALL_EXTERIOR_X,0,-20.0f},90));
      windowedWing({1,4.0f,3,STOREY_TALL,3,1.0f,false,false,true,true},131.0f); }
    { Place p(trs({TALL_EXTERIOR_X,0,-24.0f},90));
      windowedWing({2,4.0f,3,STOREY_TALL,3,1.0f,false,false,false,true},134.0f); }
    { Place p(trs({TALL_EXTERIOR_X,0,-32.0f},90));
      galleryWing({2,4.0f,3,STOREY_TALL,2.8f,0,0,0,false,PLINTH+STOREY_TALL-.32f},146.0f); }
    { Place p(trs({TALL_EXTERIOR_X,0,CAFETERIA_FRONT_Z},90)); cafeteria(); cafeteriaRightBlock(); }
    { Place p(trs({TALL_EXTERIOR_X,0,CAFETERIA_REAR_Z},90));
      windowedWing({1,56.0f+CAFETERIA_REAR_Z,3,STOREY_TALL,1,1.0f,false,false,false,true},111.0f); }
    // Angular decorative garden panels (photo 11).
    for(int k=0;k<4;++k) {
        Place p(trs({33.0f+TALL_EXTRA_WIDTH+k*.2f,0,CAFETERIA_FRONT_Z+6.0f+k*1.1f},20.0f+k*35.0f));
        addQuad({-.8f,.3f,0},{.8f,.1f,0},{.6f,1.9f,0},{-.5f,2.2f,0},clearGlass);
        rod({-.8f,0,0},{-.5f,2.2f,0},.03f,metal,5);
    }
    }
}

}} // namespace swc::app

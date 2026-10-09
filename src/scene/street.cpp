#include "scene/street.h"

#include "geometry/mesh.h"
#include "scene/layout.h"
#include "scene/lettering.h"

namespace swc { namespace app {

namespace {

// Everything here lies behind (north of) the SWC, in this order going away
// from the building: boundary wall, footpath, road, divider, stalls, town.
const float WALL_Z=-63.5f,PATH_Z=-66.4f,ROAD_Z=-74.0f,DIVIDER_Z=-74.6f,STALL_FRONT_Z=-77.2f,STALL_BACK_Z=-80.8f;
const float STREET_LEFT=-44.0f,STREET_RIGHT=38.6f;
// The public side (footpath, road, divider, stalls) runs on past the campus gate.
const float STREET_END=64.0f;

// road photo 2.54.52 AM: white piers and coping with red-brick panels between.
void boundaryWall(float x0,float x1) {
    const float bay=3.0f; const int bays=static_cast<int>((x1-x0)/bay+.5f); const float step=(x1-x0)/bays;
    block(x0,0,WALL_Z-.12f,x1,.35f,WALL_Z+.12f,whitePaint);
    for(int b=0;b<=bays;++b) {
        float x=x0+b*step;
        block(x-.18f,0,WALL_Z-.18f,x+.18f,2.25f,WALL_Z+.18f,whitePaint);
        block(x-.22f,2.25f,WALL_Z-.22f,x+.22f,2.33f,WALL_Z+.22f,trim);
        if(b==bays) break;
        block(x+.18f,.35f,WALL_Z-.09f,x+step-.18f,1.85f,WALL_Z+.09f,brickLight);
        block(x+.18f,1.85f,WALL_Z-.12f,x+step-.18f,2.0f,WALL_Z+.12f,whitePaint);
    }
}
// road photo 2.54.53 AM: white steel gate of diagonal lattice, one leaf open.
void latticeGate(float x0,float x1) {
    const Material gateWhite{{.90f,.91f,.89f},.35f};
    for(float x : {x0,x1}) block(x-.22f,0,WALL_Z-.22f,x+.22f,2.6f,WALL_Z+.22f,whitePaint);
    auto leaf=[&](float w) {
        for(float x : {0.0f,w}) block(x-.03f,.08f,-.03f,x+.03f,2.1f,.03f,gateWhite);
        for(float y : {.08f,1.05f,2.1f}) block(0,y-.03f,-.03f,w,y+.03f,.03f,gateWhite);
        for(float s=-2.0f;s<w;s+=.28f) {
            float a=std::max(s,0.0f),b=std::min(s+2.0f,w);
            if(b<=a) continue;
            rod({a,.08f+(a-s),0},{b,.08f+(b-s),0},.012f,gateWhite,5);
            rod({a,2.1f-(a-s),0},{b,2.1f-(b-s),0},.012f,gateWhite,5);
        }
    };
    const float w=(x1-x0-.5f)/2;
    { Place closed(translation({x0+.25f,0,WALL_Z})); leaf(w); }
    { Place open(trs({x1-.25f,0,WALL_Z},-100)); leaf(w); }
}
// Campus gate where the east road meets the boundary wall: weathered white
// piers with globe lamps, two stainless leaves of diagonal bars carrying
// notice boards, a walking gap between them, and brick steps.
void campusGate(float x0,float x1) {
    const Material steelGate{{.66f,.68f,.68f},.6f},pier{{.86f,.87f,.84f},.06f,NATURAL};
    const Material boardWhite{{.95f,.95f,.93f},.2f},boardBlue{{.20f,.50f,.80f},.2f},textRed{{.80f,.12f,.10f},.2f};
    const Material globe{{1.0f,.96f,.84f},.0f,EMISSIVE};
    for(float x : {x0,x1}) {
        block(x-.45f,0,WALL_Z-.35f,x+.45f,2.7f,WALL_Z+.35f,pier);
        block(x-.50f,2.7f,WALL_Z-.40f,x+.50f,2.8f,WALL_Z+.40f,trim);
        cylinder(x,2.88f,WALL_Z,.14f,.16f,.14f,lampBlack,12);
        sphere(x,3.22f,WALL_Z,.30f,globe,8,14);
    }
    // Brick threshold: two shallow steps toward the street.
    block(x0+.45f,0,WALL_Z-1.1f,x1-.45f,.16f,WALL_Z+.6f,brickLight);
    block(x0+.45f,0,WALL_Z-1.5f,x1-.45f,.08f,WALL_Z-1.1f,brickLight);
    const float gap=1.3f,w=(x1-x0-.9f-gap)/2,base=.16f;
    auto leaf=[&](bool banner) {
        for(float x : {0.0f,w}) block(x-.035f,base,-.035f,x+.035f,2.25f,.035f,steelGate);
        for(float y : {base,base+.95f,1.25f,2.25f}) block(0,y-.03f,-.03f,w,y+.03f,.03f,steelGate);
        // Diagonal bars fill the lower panel.
        for(float s=-.95f;s<w;s+=.16f) {
            float a=std::max(s,0.0f),b=std::min(s+.95f,w);
            if(b>a) rod({a,base+.95f-(a-s),0},{b,base+.95f-(b-s),0},.011f,steelGate,5);
        }
        // Notice board with a blue border and two lines of red lettering.
        block(.08f,1.30f,-.05f,w-.08f,2.20f,-.035f,boardBlue);
        block(.12f,1.34f,-.06f,w-.12f,2.16f,-.05f,boardWhite);
        for(float y : {1.86f,1.62f}) block(.25f,y,-.07f,w-.25f,y+.12f,-.06f,textRed);
        block(w*.55f,1.42f,-.07f,w-.25f,1.50f,-.06f,Material{{.12f,.42f,.26f},.2f});
        if(banner) {            // admission notice hung over the lower panel
            block(.05f,base+.05f,-.06f,w*.62f,1.28f,-.045f,boardBlue);
            block(.11f,base+.12f,-.07f,w*.62f-.06f,1.05f,-.06f,boardWhite);
            for(float y=base+.22f;y<1.0f;y+=.09f) block(.17f,y,-.075f,w*.62f-.12f,y+.03f,-.07f,Material{{.35f,.40f,.50f},.2f});
        }
    };
    // Local +X runs along the wall and local -Z faces the street, so the boards face out.
    // Seen from the street the banner hangs on the right-hand leaf.
    { Place left(translation({x0+.45f,0,WALL_Z})); leaf(true); }
    { Place right(translation({x1-.45f-w,0,WALL_Z})); leaf(false); }
    sceneLights.push_back({{(x0+x1)/2,3.0f,WALL_Z-.6f},{1.0f,.95f,.80f},6.0f});
}
// One stall: three painted walls, open front, corrugated lean-to roof,
// signboard, stocked shelves, a counter, and a lamp. The seed picks colours.
void stall(float x0,float x1,float seed) {
    const Color walls[5]={{.80f,.82f,.78f},{.30f,.52f,.72f},{.86f,.74f,.30f},{.42f,.66f,.38f},{.82f,.50f,.56f}};
    const Color signs[5]={{.84f,.10f,.14f},{.92f,.30f,.52f},{.14f,.36f,.72f},{.96f,.78f,.14f},{.16f,.56f,.34f}};
    const Color packets[5]={{.80f,.14f,.10f},{.92f,.74f,.14f},{.16f,.50f,.26f},{.16f,.34f,.70f},{.92f,.90f,.84f}};
    const int wallIndex=static_cast<int>(hash1(seed)*5)%5,signIndex=static_cast<int>(hash1(seed*1.7f+2)*5)%5;
    const Material wall{walls[wallIndex],.1f},tin{{.50f,.53f,.56f},.45f},post{{.34f,.26f,.18f},.1f};
    const float h=2.55f+.35f*hash1(seed*2.3f),zf=STALL_FRONT_Z,zb=STALL_BACK_Z;
    block(x0,0,zb,x1,.22f,zf+.9f,concrete);                                // raised floor and step
    block(x0,.22f,zb,x1,h,zb+.12f,wall);
    block(x0,.22f,zb,x0+.10f,h,zf,wall); block(x1-.10f,.22f,zb,x1,h,zf,wall);
    // Lean-to roof: higher at the back, projecting over the front.
    addQuad({x0-.15f,h+.05f,zf+1.3f},{x1+.15f,h+.05f,zf+1.3f},{x1+.15f,h+.50f,zb-.2f},{x0-.15f,h+.50f,zb-.2f},tin);
    addQuad({x0-.15f,h,zf+1.3f},{x0-.15f,h+.45f,zb-.2f},{x1+.15f,h+.45f,zb-.2f},{x1+.15f,h,zf+1.3f},tin);
    for(float x : {x0+.06f,x1-.06f}) block(x-.05f,.22f,zf+1.15f,x+.05f,h+.05f,zf+1.25f,post);
    // Signboard across the front, lit.
    block(x0,h-.55f,zf,x1,h+.10f,zf+.06f,Material{signs[signIndex],.0f,EMISSIVE});
    block(x0+.3f,h-.42f,zf+.06f,x1-.3f,h-.34f,zf+.07f,Material{{1,1,.95f},.0f,EMISSIVE});
    block(x0+.6f,h-.22f,zf+.06f,x1-.6f,h-.10f,zf+.07f,Material{{1,1,.95f},.0f,EMISSIVE});
    // Shelves of packets on the back wall; hanging strips at the front.
    for(int level=0;level<4;++level) {
        float y=.55f+level*.45f;
        block(x0+.15f,y,zb+.12f,x1-.15f,y+.03f,zb+.45f,post);
        for(float x=x0+.22f;x<x1-.4f;x+=.30f)
            block(x,y+.03f,zb+.16f,x+.22f,y+.03f+.18f+.14f*hash1(x*3.1f+level),zb+.40f,
                  Material{packets[static_cast<int>(hash1(x*7.3f+level*1.9f+seed)*5)%5],.2f});
    }
    for(float x=x0+.5f;x<x1-.4f;x+=.55f)
        block(x,h-1.45f,zf-.12f,x+.10f,h-.60f,zf-.08f,Material{packets[static_cast<int>(hash1(x+seed)*5)%5],.2f});
    block(x0+.4f,.22f,zf-.55f,(x0+x1)/2,1.1f,zf-.15f,Material{{.62f,.48f,.32f},.2f});       // counter
    box((x0+x1)/2+.6f,h-.25f,(zf+zb)/2,.08f,.08f,.08f,tubeLight);
    sceneLights.push_back({{(x0+x1)/2,h-.35f,zf-1.2f},{1.25f,1.10f,.80f},6.5f});
    // Plastic chairs or stools on the apron in front of some stalls.
    if(hash1(seed*4.1f)<.6f)
        for(int k=0;k<3;++k) {
            float x=x0+.6f+k*.75f,z=zf+1.9f+.25f*hash1(seed+k);
            Material plastic{hash1(seed+k*2.2f)<.5f ? Color{.78f,.14f,.12f} : Color{.14f,.42f,.66f},.3f};
            box(x,.36f,z,.40f,.05f,.40f,plastic); box(x,.60f,z+.19f,.40f,.44f,.04f,plastic);
            for(float lx : {-.17f,.17f}) for(float lz : {-.17f,.17f}) box(x+lx,.17f,z+lz,.035f,.34f,.035f,plastic);
        }
}
// Multi-storey town buildings behind the stalls: painted blocks with window
// grids and projecting balconies.
void townBuilding(float x0,float x1,float height,Color colour,float seed) {
    const float z0=-90.0f,z1=-82.0f; const Material body{colour,.08f};
    block(x0,0,z0,x1,height,z1,body);
    block(x0-.15f,height,z0-.15f,x1+.15f,height+.25f,z1+.15f,trim);
    const int floors=static_cast<int>(height/3.1f);
    for(int f=1;f<floors;++f) {
        float y=f*3.1f;
        for(float x=x0+.7f;x<x1-1.4f;x+=2.3f)
            block(x,y+.9f,z1,x+1.3f,y+2.3f,z1+.04f,glassFor(seed+x*1.3f+f*5.0f));
        if(hash1(seed+f)<.6f) {
            block(x0+.5f,y,z1,x1-.5f,y+.12f,z1+1.0f,trim);
            block(x0+.5f,y+.12f,z1+.94f,x1-.5f,y+.95f,z1+1.0f,Material{mix(colour,{1,1,1},.35f),.08f});
        }
    }
}

} // namespace

// ---------------------------------------------------------------------------
// road/ photographs: behind the SWC a brick boundary wall separates the
// campus from a footpath and the public road; a low concrete divider runs
// down the middle of the road, and beyond the road stands a row of stalls.
// ---------------------------------------------------------------------------
void backStreet() {
    const Material asphalt{{.24f,.24f,.25f},.05f,NATURAL},dirt{{.50f,.42f,.34f},.03f,NATURAL};
    const float gate0=6.0f,gate1=10.4f;
    boundaryWall(STREET_LEFT,gate0); boundaryWall(gate1,STREET_RIGHT);
    latticeGate(gate0,gate1);
    // The east campus road (x 41-47) leaves through the main gate in the same wall.
    const float campusGate0=40.9f,campusGate1=47.1f;
    boundaryWall(STREET_RIGHT,campusGate0-.45f); boundaryWall(campusGate1+.45f,STREET_END);
    campusGate(campusGate0,campusGate1);
    // Footpath, road with worn patches, and the apron in front of the stalls.
    addQuad({STREET_LEFT,.03f,WALL_Z-.2f},{STREET_END,.03f,WALL_Z-.2f},{STREET_END,.03f,PATH_Z},{STREET_LEFT,.03f,PATH_Z},{0,1,0},dirt);
    block(STREET_LEFT,0,PATH_Z-.15f,STREET_END,.10f,PATH_Z,concrete);                       // kerb
    addQuad({STREET_LEFT-6,.025f,PATH_Z-.15f},{STREET_END+2.4f,.025f,PATH_Z-.15f},{STREET_END+2.4f,.025f,ROAD_Z},{STREET_LEFT-6,.025f,ROAD_Z},{0,1,0},asphalt);
    for(int k=0;k<14;++k) {
        float x=STREET_LEFT+3+hash1(k*5.3f)*(STREET_END-STREET_LEFT-8),z=ROAD_Z+1.0f+hash1(k*2.9f)*5.5f,r=.7f+hash1(k*1.3f);
        annulus(x,.03f,z,0,r,dirt,14);
    }
    addQuad({STREET_LEFT,.03f,ROAD_Z},{STREET_END,.03f,ROAD_Z},{STREET_END,.03f,STALL_BACK_Z-.6f},{STREET_LEFT,.03f,STALL_BACK_Z-.6f},{0,1,0},dirt);
    // The divider runs down the middle of the road, between the two lanes.
    const float median=(PATH_Z-.15f+ROAD_Z)/2;
    // Low concrete divider with gaps to reach the stalls.
    for(float x=STREET_LEFT;x<STREET_END-1;x+=12.0f) {
        float end=std::min(x+10.2f,STREET_END);
        // Leave the divider open opposite the campus gate.
        for(int part=0;part<2;++part) {
            float a=part==0 ? x : std::max(x,46.6f),b=part==0 ? std::min(end,41.4f) : end;
            if(b-a<.5f) continue;
            block(a,0,median-.3f,b,.62f,median+.3f,concrete);
            block(a,.62f,median-.34f,b,.68f,median+.34f,trim);
        }
    }
    // Stalls of varying width, with an occasional gap between them.
    float x=STREET_LEFT+1.0f; int index=0;
    while(x<STREET_END-4.5f) {
        float width=4.2f+2.2f*hash1(index*3.7f+1);
        stall(x,x+width,index*2.31f+.7f);
        x+=width+(index%4==3 ? 1.6f : .15f); ++index;
    }
    // Town buildings behind the stalls (blue, pink and grey in the photographs).
    const Color town[5]={{.30f,.56f,.74f},{.86f,.56f,.62f},{.72f,.74f,.72f},{.90f,.86f,.72f},{.50f,.66f,.60f}};
    float bx=STREET_LEFT-2.0f; int b=0;
    while(bx<STREET_END-6) {
        float width=9.0f+6.0f*hash1(b*4.9f+3),height=9.3f+3.1f*static_cast<int>(hash1(b*2.1f+5)*3);
        townBuilding(bx,bx+width,height,town[b%5],b*11.0f);
        bx+=width+1.2f+2.0f*hash1(b*1.7f); ++b;
    }
    // Utility poles along the far kerb.
    for(float px=STREET_LEFT+6;px<STREET_END;px+=16.0f) {
        rod({px,0,DIVIDER_Z-.6f},{px,7.2f,DIVIDER_Z-.6f},.09f,concrete,8);
        rod({px-.9f,6.7f,DIVIDER_Z-.6f},{px+.9f,6.7f,DIVIDER_Z-.6f},.04f,metalDark,5);
    }
}

// ---------------------------------------------------------------------------
// Traffic: easybikes (battery three-wheelers), cycle-vans (flatbed
// tricycles) and, now and then, a bus. Bangladesh drives on the left, so the
// lane next to the campus runs toward -X and the lane by the stalls toward +X.
// ---------------------------------------------------------------------------
namespace {

const float ROAD_START_X=STREET_LEFT-6.0f,ROAD_END_X=STREET_END+2.4f;
const Material tyre{{.07f,.07f,.08f},.2f},rim{{.80f,.82f,.84f},.6f};

// A wheel in the XY plane: the canonical cylinder turned so its axis lies along Z.
void wheel(float x,float z,float radius,float width,bool cover=false) {
    Place turned(multiply(translation({x,radius,z}),rotationX(PI/2)));
    cylinder(0,0,0,radius,width,radius,tyre,16);
    cylinder(0,0,0,radius*.55f,width+.02f,radius*.55f,cover ? Material{{.93f,.93f,.90f},.4f} : rim,12);
}
void rider(float x,float y,float z,Color shirt) {
    box(x,y+.32f,z,.26f,.60f,.38f,Material{shirt,.1f});
    sphere(x+.02f,y+.76f,z,.13f,Material{{.55f,.38f,.26f},.1f},6,10);
    box(x+.16f,y-.18f,z,.42f,.14f,.34f,Material{{.20f,.24f,.36f},.1f});
}
// Reference image: red body, tall curved windscreen, canopy on four posts,
// white wheel covers, a driver's seat and two facing passenger benches.
void easybikeMesh() {
    const Material body{{.72f,.10f,.12f},.45f},canopy{{.60f,.16f,.22f},.3f},seat{{.16f,.16f,.18f},.2f};
    wheel(1.15f,0,.22f,.10f); wheel(-.85f,-.52f,.22f,.10f,true); wheel(-.85f,.52f,.22f,.10f,true);
    block(-1.35f,.26f,-.55f,.80f,.40f,.55f,body);                       // floor pan
    block(.80f,.26f,-.32f,1.30f,.95f,.32f,body);                        // nose over the front wheel
    block(-1.35f,.40f,-.55f,-1.28f,1.05f,.55f,body);                    // rear panel
    for(float z : {-.55f,.50f}) block(-1.35f,.40f,z,-.20f,.78f,z+.05f,body);
    addQuad({1.28f,.95f,-.50f},{1.28f,.95f,.50f},{.85f,1.78f,.50f},{.85f,1.78f,-.50f},clearGlass);
    block(-1.40f,1.78f,-.60f,.92f,1.86f,.60f,canopy);
    for(float x : {.85f,-1.33f}) for(float z : {-.53f,.53f}) rod({x,.40f,z},{x,1.78f,z},.025f,rim,6);
    for(float z : {-.53f,.53f}) rod({-.25f,.40f,z},{-.25f,1.78f,z},.025f,rim,6);
    block(.20f,.40f,-.30f,.60f,.72f,.30f,seat); block(.14f,.72f,-.30f,.22f,1.15f,.30f,seat);
    block(-1.26f,.40f,-.48f,-.90f,.74f,.48f,seat); block(-.56f,.40f,-.48f,-.22f,.74f,.48f,seat);
    rod({1.05f,.95f,0},{.72f,1.20f,0},.02f,tyre,6); rod({.72f,1.20f,-.28f},{.72f,1.20f,.28f},.02f,tyre,6);
    box(1.31f,.70f,0,.03f,.14f,.18f,Material{{1,.96f,.80f},.0f,EMISSIVE});
    rider(.42f,.78f,0,{.30f,.42f,.62f});
}
// Reference photograph: a cycle-van. One bicycle wheel and saddle in front,
// a steel flatbed over two wheels behind, loaded with bags and a crate.
void vanMesh() {
    const Material frame{{.24f,.26f,.30f},.5f},bed{{.52f,.52f,.50f},.35f};
    wheel(1.55f,0,.34f,.05f); wheel(-.55f,-.72f,.34f,.05f); wheel(-.55f,.72f,.34f,.05f);
    rod({1.55f,.34f,0},{1.30f,1.05f,0},.022f,frame,6); rod({1.30f,1.05f,-.26f},{1.30f,1.05f,.26f},.02f,frame,6);
    rod({1.30f,.95f,0},{.62f,.62f,0},.025f,frame,6); rod({.62f,.62f,0},{.70f,.96f,0},.022f,frame,6);
    rod({.62f,.62f,0},{.90f,.30f,0},.022f,frame,6);
    box(.70f,.99f,0,.26f,.06f,.18f,tyre);                                // saddle
    block(-1.35f,.66f,-.62f,.45f,.72f,.62f,bed);
    for(float z : {-.62f,.58f}) block(-1.35f,.58f,z,.45f,.66f,z+.04f,frame);
    rod({-.55f,.34f,-.70f},{-.55f,.34f,.70f},.022f,frame,6);
    box(-.75f,.90f,-.18f,.60f,.36f,.42f,Material{{.74f,.12f,.14f},.15f});
    box(-.10f,.86f,.22f,.42f,.28f,.34f,Material{{.90f,.46f,.14f},.2f});
    box(-.95f,.80f,.36f,.34f,.16f,.26f,Material{{.90f,.90f,.86f},.1f});
    rider(.66f,1.02f,0,{.78f,.78f,.72f});
}
void busMesh() {
    const Material body{{.90f,.90f,.84f},.4f},band{{.12f,.42f,.30f},.4f},dark{{.08f,.09f,.10f},.3f};
    for(float x : {2.9f,-2.6f}) for(float z : {-1.08f,1.08f}) wheel(x,z,.48f,.28f);
    block(-4.5f,.42f,-1.2f,4.5f,3.05f,1.2f,body);
    block(-4.52f,.95f,-1.215f,4.52f,1.35f,1.215f,band);
    block(-4.3f,3.05f,-1.05f,4.3f,3.14f,1.05f,body);
    for(float z : {-1.21f,1.19f}) {
        for(float x=-4.1f;x<2.9f;x+=1.15f) block(x,1.62f,z,x+1.0f,2.55f,z+.02f,glassFor(x*3.1f+z));
        block(3.2f,.60f,z,4.1f,2.6f,z+.02f,dark);                         // door
    }
    block(4.5f,1.45f,-1.05f,4.52f,2.65f,1.05f,clearGlass);               // windscreen
    block(-4.52f,1.70f,-.95f,-4.5f,2.55f,.95f,glassFor(77.0f));
    for(float z : {-.85f,.85f}) box(4.52f,.85f,z,.03f,.18f,.30f,Material{{1,.96f,.80f},.0f,EMISSIVE});
    for(float z : {-.90f,.90f}) box(-4.52f,.85f,z,.03f,.16f,.22f,Material{{.85f,.10f,.08f},.0f,EMISSIVE});
    block(4.2f,.42f,-1.22f,4.56f,.62f,1.22f,dark); block(-4.56f,.42f,-1.22f,-4.2f,.62f,1.22f,dark);
}

} // namespace

void buildVehicleMeshes() {
    vehicleMeshes.clear(); vehicles.clear();
    void (*builders[3])()={easybikeMesh,vanMesh,busMesh};
    for(auto build:builders) {
        GLint first=static_cast<GLint>(mesh.size());
        build();
        vehicleMeshes.push_back({first,static_cast<GLsizei>(mesh.size())-first});
    }
    // Vehicles in one lane share a speed and a loop length, so their spacing
    // never changes and they cannot run into each other. The loop is longer
    // than the visible road, which is why the bus is only sometimes in view.
    const float median=(PATH_Z-.15f+ROAD_Z)/2,nearLane=(PATH_Z-.15f+median-.3f)/2,farLane=(median+.3f+ROAD_Z)/2;
    for(float offset : {0.0f,150.0f}) vehicles.push_back({0,farLane,1,4.5f,offset,240});
    vehicles.push_back({1,farLane,1,4.5f,190,240});
    for(float offset : {20.0f,170.0f}) vehicles.push_back({0,nearLane,-1,5.5f,offset,360});
    vehicles.push_back({1,nearLane,-1,5.5f,230,360});
    vehicles.push_back({2,nearLane,-1,5.5f,265,360});
}

bool vehicleTransform(const Vehicle& v,float time,Mat4& transform) {
    // Distance travelled along the loop; only the first stretch lies on the road.
    float s=std::fmod(v.offset+v.speed*time,v.loop);
    if(s>ROAD_END_X-ROAD_START_X) return false;
    float x=v.direction>0 ? ROAD_START_X+s : ROAD_END_X-s;
    transform=multiply(translation({x,.03f,v.laneZ}),rotationY(v.direction>0 ? 0.0f : PI));
    return true;
}

}} // namespace swc::app

#include "scene/interior.h"

#include "geometry/mesh.h"
#include "room_navigation.h"
#include "scene/components.h"
#include "scene/layout.h"
#include "scene/lettering.h"
#include "scene/rooms.h"

namespace swc { namespace app {

namespace {

const float F0=PLINTH,F1=PLINTH+STOREY_TALL;
const float TILE_UNIT=.6f;
const Material greyTile{{.36f,.36f,.38f},.45f};
const Material wallWhite{{.90f,.91f,.88f},.08f};
// The shop ends at SHOP_RIGHT_X; the side corridor and the stair behind the
// curved wall occupy STAIR_LEFT_X..STAIR_RIGHT_X.
const float SHOP_RIGHT_X=14.4f,STAIR_LEFT_X=14.6f,STAIR_RIGHT_X=17.8f;
// The terracotta recess starts at LOBBY_LEFT_X. Its left return is the side wall
// of the games hall, which carries the INDOOR GAMES door centred on GAMES_DOOR_Z.
const float LOBBY_LEFT_X=-4.3f,GAMES_DOOR_Z=-4.85f;

// A light source without new geometry, for bays that already have a fixture.
void glow(float x,float y,float z,float radius=8.0f,float power=1.0f) {
    sceneLights.push_back({{x,y,z},{1.22f*power,1.18f*power,1.08f*power},radius});
}
// Fluorescent batten: an emissive strip plus the light it casts.
void tube(float x,float ceilingY,float z,float length,bool alongX,float radius=8.0f,float power=1.0f) {
    box(x,ceilingY-.035f,z,alongX ? length : .10f,.05f,alongX ? .10f : length,tubeLight);
    glow(x,ceilingY-.38f,z,radius,power);
}
// One accent tile snapped to the 0.6 m floor grid, so its edges meet the joints.
void accentTile(float x,float y,float z,const Material& m) {
    float gx=std::floor(x/TILE_UNIT)*TILE_UNIT,gz=std::floor(z/TILE_UNIT)*TILE_UNIT;
    block(gx,y,gz,gx+TILE_UNIT,y+.006f,gz+TILE_UNIT,m);
}
// Two corner-touching black tiles flanked by grey ones (6.14.10, 6.14.13).
void accentCluster(float x,float y,float z,bool alongZ=false) {
    const int cells[4][2]={{0,0},{1,1},{-2,1},{3,0}};
    for(int i=0;i<4;++i) {
        float a=cells[i][0]*TILE_UNIT,b=cells[i][1]*TILE_UNIT;
        accentTile(x+(alongZ ? b : a),y,z+(alongZ ? a : b),i<2 ? darkTile : greyTile);
    }
}
// Inside face of a barred window: brick sill panel, pale mullions, transom and
// white security grille in front of bright daylight. Local frame: wall plane
// z=0, facing +Z, x in [0,w].
void interiorWindow(float w,float floorY,int panes) {
    const float g0=floorY+.93f,g1=floorY+2.25f,t1=floorY+2.95f;
    block(0,floorY,0,w,floorY+.85f,.10f,brickLight);
    block(-.04f,floorY+.85f,0,w+.04f,floorY+.93f,.14f,trim);
    block(0,g0,0,w,t1,.03f,daylight);
    block(0,g1,0,w,g1+.12f,.08f,whitePaint);
    for(int i=0;i<=panes;++i) { float x=w*i/panes; block(x-.035f,g0,0,x+.035f,t1,.08f,whitePaint); }
    for(float y=g0+.12f;y<g1-.05f;y+=.13f) block(0,y,.045f,w,y+.012f,.06f,whitePaint);
    for(int k=1;k<panes*4;++k) { float x=w*k/(panes*4); block(x-.006f,g0,.045f,x+.006f,g1,.06f,whitePaint); }
}
// Dark laminate table on a steel frame with four slatted chairs (6.14.08 (2)).
void diningSet(float x,float z,bool chairs=true) {
    const Material top{{.20f,.20f,.19f},.40f,INTERIOR},frame{{.30f,.31f,.31f},.5f,INTERIOR};
    const Material seat{{.34f,.17f,.10f},.25f,INTERIOR};
    box(x,F0+.75f,z,1.5f,.04f,.75f,top);
    for(float sx : {-.68f,.68f}) for(float sz : {-.31f,.31f}) box(x+sx,F0+.365f,z+sz,.04f,.73f,.04f,frame);
    box(x,F0+.25f,z,1.36f,.03f,.03f,frame);
    if(!chairs) return;
    for(float sx : {-.38f,.38f}) for(float side : {-1.0f,1.0f}) {
        float cz=z+side*.66f;
        box(x+sx,F0+.44f,cz,.40f,.03f,.38f,seat);
        for(int slat=0;slat<3;++slat) box(x+sx,F0+.62f+slat*.11f,cz+side*.19f,.38f,.06f,.02f,seat);
        for(float lx : {-.18f,.18f}) {
            box(x+sx+lx,F0+.22f,cz-side*.17f,.025f,.44f,.025f,frame);
            box(x+sx+lx,F0+.46f,cz+side*.19f,.025f,.92f,.025f,frame);
        }
    }
}
void tableTennis(float x,float z) {
    const Material top{{.07f,.30f,.22f},.30f,INTERIOR},line{{.92f,.93f,.90f},.2f,INTERIOR};
    box(x,F0+.76f,z,2.4f,.03f,1.3f,top);
    box(x,F0+.778f,z,2.4f,.004f,.02f,line);
    for(float e : {-.64f,.64f}) box(x,F0+.778f,z+e,2.4f,.004f,.02f,line);
    for(float e : {-1.19f,1.19f}) box(x+e,F0+.778f,z,.02f,.004f,1.3f,line);
    box(x,F0+.85f,z,.012f,.15f,1.42f,Material{{.15f,.16f,.17f},.2f,INTERIOR});
    for(float sx : {-.9f,.9f}) for(float sz : {-.5f,.5f}) box(x+sx,F0+.37f,z+sz,.04f,.74f,.04f,steel);
    for(float sx : {-.9f,.9f}) box(x+sx,F0+.30f,z,.03f,.03f,1.0f,steel);
}
void planter(float x,float y,float z,float seed) {
    frustum({x,y,z},.13f,.20f,.26f,potRed,10);
    for(int k=0;k<6;++k) {
        float a=2*PI*k/6+seed;
        rod({x,y+.24f,z},{x+.20f*std::cos(a),y+.55f+.12f*hash1(seed+k),z+.20f*std::sin(a)},.016f,
            leafMaterial(seed+k,.2f),5);
    }
}
// Timber double door with a louvred panel above. Local frame: wall plane z=0, facing +Z.
void timberDoor(float x,float floorY,float w) {
    block(x-w/2-.09f,floorY,0,x+w/2+.09f,floorY+3.05f,.05f,timber);
    for(float side : {-1.0f,1.0f}) {
        float c=x+side*w/4;
        block(c-w/4+.03f,floorY+.04f,.05f,c+w/4-.03f,floorY+2.25f,.08f,Material{{.42f,.21f,.10f},.35f,INTERIOR});
        block(c-.09f,floorY+1.25f,.08f,c+.09f,floorY+1.95f,.09f,glassFor(c*3.1f));
        block(c-side*(w/4-.10f)-.02f,floorY+1.0f,.08f,c-side*(w/4-.10f)+.02f,floorY+1.14f,.12f,steel);
    }
    for(float y=floorY+2.40f;y<floorY+2.98f;y+=.075f)
        block(x-w/2,y,.05f,x+w/2,y+.035f,.09f,Material{{.26f,.12f,.06f},.2f,INTERIOR});
}

} // namespace

// ---------------------------------------------------------------------------
// 6.14.05 PM: the lobby under the entrance canopy. A tall terracotta relief
// wall stands beside the timber doors; the white end wall of the front-left
// wing carries the INDOOR GAMES doorway.
// ---------------------------------------------------------------------------
void entranceLobby() {
    const float roofUnder=PLINTH+2*STOREY,wz=-5.95f,tx0=LOBBY_LEFT_X,tx1=.05f,tTop=6.0f;
    const Material reliefDark{{.50f,.21f,.10f},.05f},reliefLight{{.76f,.38f,.20f},.06f};
    // Blank white bay facing the plaza, in line with the wing's window wall.
    // The INDOOR GAMES door is round the corner, in the side wall (gamesHall()).
    block(-7.8f,PLINTH,-3.35f,tx0,roofUnder,-2.94f,wallWhite);
    // The colonnade roof and its free column continue across this bay.
    block(-7.8f,roofUnder,-3.6f,-2.6f,roofUnder+SLAB,1.9f,trim);
    block(-7.8f,roofUnder-.18f,1.62f,-2.6f,roofUnder,1.9f,trim);
    block(tx0,roofUnder,-6.3f,-2.65f,roofUnder+SLAB,-3.6f,trim);
    box(-3.0f,(PLINTH+roofUnder)/2,1.0f,.56f,roofUnder-PLINTH,.56f,trim);
    cylinder(-5.4f,roofUnder-.07f,-.6f,.08f,.14f,.08f,lampBlack,10);

    // Terracotta relief wall facing the plaza, set back 2.6 m behind the white bay.
    block(tx0,0,-6.3f,-1.1f,PLINTH,-4.6f,walkway);
    block(tx0,PLINTH,-6.3f,tx1,tTop,wz,terracotta);
    block(tx0,tTop,-6.3f,tx1,roofUnder,wz,wallWhite);
    // Square relief panels: recessed joints with a few proud carved tiles.
    for(float y=PLINTH+.55f;y<tTop-.1f;y+=.62f) block(tx0,y,wz,tx1,y+.025f,wz+.02f,reliefDark);
    for(float x=tx0+.30f;x<tx1-.1f;x+=.52f) block(x,PLINTH+2.4f,wz,x+.022f,tTop,wz+.02f,reliefDark);
    for(int k=0;k<30;++k) {
        float x=tx0+.1f+hash1(k*3.7f)*(tx1-tx0-.5f),y=PLINTH+2.5f+hash1(k*5.3f)*(tTop-PLINTH-2.9f);
        block(x,y,wz,x+.26f,y+.26f,wz+.035f,reliefLight);
    }
    block(tx1-.15f,PLINTH+.25f,wz,tx1-.03f,PLINTH+3.3f,wz+.03f,Material{{.92f,.78f,.18f},.15f});
    for(float x : {-3.6f,-2.2f,-.8f}) cylinder(x,roofUnder-.07f,-5.3f,.07f,.14f,.07f,lampBlack,10);
    // Tournament board: white panel, blue heading and coloured department cells.
    const Color cells[4]={{.16f,.52f,.34f},{.74f,.20f,.48f},{.22f,.30f,.62f},{.88f,.52f,.16f}};
    block(-3.70f,PLINTH+.15f,wz+.03f,-1.20f,PLINTH+2.25f,wz+.08f,Material{{.94f,.94f,.90f},.2f});
    block(-3.58f,PLINTH+1.92f,wz+.08f,-1.32f,PLINTH+2.10f,wz+.09f,Material{{.16f,.22f,.42f},.2f});
    block(-3.58f,PLINTH+.28f,wz+.08f,-1.32f,PLINTH+.40f,wz+.09f,Material{{.12f,.42f,.26f},.2f});
    for(int r=0;r<4;++r) for(int c=0;c<6;++c)
        block(-3.58f+c*.38f,PLINTH+.52f+r*.33f,wz+.08f,-3.26f+c*.38f,PLINTH+.78f+r*.33f,wz+.09f,
              Material{cells[(r*3+c)%4],.2f});
    accentCluster(2.0f,PLINTH,-2.2f);
    glow(-1.0f,roofUnder-1.0f,-2.6f,10.0f,.8f); glow(-2.0f,PLINTH+3.1f,-2.4f,9.0f,.8f);
}

// ---------------------------------------------------------------------------
// 6.14.06 PM: the indoor-games hall fills the ground floor of the front-left
// wing. Windows line both long walls; stacked chairs wait beside the door.
// ---------------------------------------------------------------------------
void gamesHall() {
    const float x0=-26.25f,x1=LOBBY_LEFT_X,z0=-10.55f,z1=-3.35f,cy=3.6f,top=7.8f;
    const float dz0=GAMES_DOOR_Z-.75f,dz1=GAMES_DOOR_Z+.75f,dh=PLINTH+2.5f;
    block(x0,0,z0,x1,PLINTH,z1,walkway);
    block(x0,cy,z0,x1-.3f,top,z1,concrete);
    block(x0,cy-.02f,z0,x1-.3f,cy,z1,plaster);            // white ceiling
    block(x0,PLINTH,z0,x1-.3f,cy,z0+.3f,plaster);
    block(x0,PLINTH,z0,x0+.3f,cy,z1,plaster);
    // 6.14.05 PM: the full-height white side wall of the lobby, at a right
    // angle to the terracotta wall, with the INDOOR GAMES doorway near the corner.
    block(x1-.3f,0,z0,x1,top,dz0,wallWhite);
    block(x1-.3f,0,dz1,x1,top,z1,wallWhite);
    block(x1-.3f,dh,dz0,x1,top,dz1,wallWhite);
    for(float z : {dz0-.08f,dz1}) block(x1-.36f,PLINTH,z,x1+.05f,dh,z+.08f,timber);
    block(x1-.36f,dh,dz0-.08f,x1+.05f,dh+.10f,dz1+.08f,timber);
    // Both leaves stand open against the inside wall.
    block(x1-1.02f,PLINTH,dz0-.14f,x1-.3f,dh,dz0-.09f,timber);
    block(x1-1.02f,PLINTH,dz1+.09f,x1-.3f,dh,dz1+.14f,timber);
    block(x1,dh+.22f,dz0-.30f,x1+.035f,dh+1.05f,dz1+.40f,Material{{.93f,.93f,.90f},.15f});
    { Place sign(trs({x1+.036f,0,GAMES_DOOR_Z+.05f},90));
      flatText(latinGlyphs("INDOOR GAMES"),0,dh+.62f,0,.028f,.045f,.012f,signDark);
      block(-.95f,dh+.36f,0,.95f,dh+.42f,.012f,Material{{.16f,.22f,.42f},.2f}); }

    for(int b=0;b<4;++b) {
        float cx=-27.0f+4.8f*b+2.4f;
        { Place south(trs({cx+1.8f,0,z1},180)); interiorWindow(3.6f,PLINTH,3); }
        if(b>0) { Place north(translation({cx-1.8f,0,z0+.3f})); interiorWindow(3.6f,PLINTH,3); }
    }
    for(float x : {-8.0f,-13.0f,-18.0f,-23.0f}) {
        box(x,(PLINTH+cy)/2,-6.0f,.44f,cy-PLINTH,.44f,plaster);
        block(x-.18f,cy-.30f,z0,x+.18f,cy,z1,plaster);
    }
    for(float x : {-10.5f,-15.5f,-20.5f}) tableTennis(x,-8.0f);
    for(float x : {-6.0f,-10.5f,-15.5f,-20.5f}) {
        ceilingFan(x,cy,-8.0f);
        tube(x,cy,-4.9f,1.2f,true,8.5f,1.05f);
    }
    // Stacked plastic chairs to the left of the door, and the attendant's desk.
    for(int stack=0;stack<4;++stack) {
        float x=-6.05f+stack*.36f;
        Color c=stack%2 ? Color{.62f,.16f,.14f} : Color{.36f,.24f,.16f};
        for(int k=0;k<11;++k) box(x,PLINTH+.44f+k*.085f,-3.63f,.30f,.03f,.34f,Material{c,.3f,INTERIOR});
        box(x,PLINTH+.22f,-3.63f,.26f,.44f,.30f,Material{{.20f,.20f,.20f},.3f,INTERIOR});
    }
    box(-5.4f,PLINTH+.72f,-8.6f,.70f,.05f,1.3f,Material{{.74f,.62f,.44f},.25f,INTERIOR});
    box(-5.4f,PLINTH+.35f,-8.6f,.62f,.70f,1.2f,Material{{.68f,.56f,.40f},.2f,INTERIOR});
    accentCluster(-9.4f,PLINTH,-5.0f); accentCluster(-17.0f,PLINTH,-5.0f);
}

// ---------------------------------------------------------------------------
// 6.14.06 PM (1)/(2): beyond the timber doors, the Shwapno shop opens onto
// the corridor behind the curved wall, below a ceiling of diagonal ribs.
// ---------------------------------------------------------------------------
void shopHall() {
    // Images 6.14.06 PM (1) and (2): the shop is on the right of the entrance
    // hall, its long front parallel to the way in, with the timber entrance
    // at its right-hand end. It occupies the bays east of the hall.
    const float fx=3.5f,x1=7.4f,z0=-16.4f,z1=-7.4f,cy=F1-.32f;
    const Material signRed{{.82f,.07f,.06f},.0f,EMISSIVE},signWhite{{1,1,.97f},.0f,EMISSIVE};
    const Material aluminium{{.55f,.60f,.60f},.7f,INTERIOR};
    // The mass behind the curved wall is solid again, apart from the side corridor.
    block(5.5f,0,-7.0f,SHOP_RIGHT_X,7.8f,-3.2f,concrete);
    block(STAIR_RIGHT_X+.2f,0,-7.0f,21.25f,7.8f,-3.2f,concrete);
    block(x1,PLINTH,-11.3f,x1+.2f,cy,-7.0f,plaster);          // back wall toward the hall behind the curve
    block(fx-.1f,3.55f,z0,fx+.1f,cy,-11.3f,plaster);
    // Red fascia: star panel at the far end, Latin name, Bengali name near the entrance.
    block(fx-.25f,2.95f,z0-.1f,fx,3.55f,z1+.1f,signRed);
    { Place sign(trs({fx-.255f,0,z0},-90));
      block(.05f,3.0f,0,.55f,3.5f,.015f,Material{{.05f,.05f,.07f},.3f});
      flatText(latinGlyphs("SHWAPNO"),2.6f,3.08f,0,.07f,.07f,.02f,signWhite);
      flatText(bengaliGlyphs({3},31.0f),6.5f,3.06f,0,.08f,.08f,.02f,signWhite); }
    for(float z : {z0-.1f,-13.4f,-10.3f,-8.7f,z1+.1f}) block(fx-.12f,PLINTH,z-.1f,fx+.1f,2.95f,z+.1f,plaster);
    // Glazed bays in aluminium frames; the doorway is the open bay.
    auto glazedBay=[&](float za,float zb) {
        const float x=fx-.02f,y0=PLINTH+.05f,y1=2.9f;
        for(float y : {y0,y1}) block(x-.035f,y-.03f,za,x+.035f,y+.03f,zb,aluminium);
        for(float z : {za,zb}) block(x-.035f,y0,z-.03f,x+.035f,y1,z+.03f,aluminium);
        roomGlass.push_back({{x,(y0+y1)/2,(za+zb)/2},zb-za-.06f,y1-y0-.06f,0,true});
    };
    glazedBay(z0+.1f,-13.5f); glazedBay(-13.3f,-10.4f); glazedBay(-8.6f,z1-.1f);
    // Shelving along the back wall and a centre gondola, stocked with packets.
    const Color packets[5]={{.80f,.14f,.10f},{.92f,.74f,.14f},{.16f,.50f,.26f},{.16f,.34f,.70f},{.92f,.90f,.84f}};
    const Material shelf{{.86f,.86f,.82f},.3f,INTERIOR};
    for(int level=0;level<5;++level) {
        float y=PLINTH+.25f+level*.48f;
        block(x1-.38f,y,z0+.2f,x1,y+.03f,z1,shelf);
        for(int k=0;k<static_cast<int>((z1-z0-.4f)/.31f);++k) {
            float z=z0+.3f+k*.31f,h=.20f+.16f*hash1(level*9.1f+k);
            block(x1-.30f,y+.03f,z,x1-.06f,y+.03f+h,z+.24f,
                  Material{packets[static_cast<int>(hash1(level*3.3f+k*1.7f)*5)%5],.25f,INTERIOR});
        }
    }
    block(5.2f,PLINTH,-15.4f,5.8f,PLINTH+1.35f,-11.6f,shelf);
    for(int level=0;level<3;++level) for(int k=0;k<12;++k) {
        float z=-15.35f+k*.31f,y=PLINTH+.15f+level*.42f;
        for(float x : {5.13f,5.8f})
            block(x,y,z,x+.07f,y+.30f,z+.24f,Material{packets[(k+level*2)%5],.25f,INTERIOR});
    }
    // "Big sale" stand behind the middle bay, and the bill counter beside the door.
    block(3.9f,PLINTH,-13.1f,4.35f,PLINTH+.85f,-10.7f,Material{{.78f,.10f,.08f},.2f,INTERIOR});
    for(int k=0;k<2;++k)
        block(3.88f,PLINTH+.25f,-12.95f+k*1.15f,3.9f,PLINTH+.62f,-12.0f+k*1.15f,Material{{.96f,.84f,.16f},.2f,INTERIOR});
    for(int k=0;k<6;++k)
        block(3.95f,PLINTH+.85f,-13.0f+k*.38f,4.3f,PLINTH+1.12f,-12.74f+k*.38f,Material{packets[k%5],.25f,INTERIOR});
    block(5.7f,PLINTH,-8.4f,7.0f,PLINTH+1.0f,-7.8f,Material{{.20f,.20f,.22f},.4f,INTERIOR});
    tube(5.4f,cy,-14.2f,1.2f,false,7.0f,1.2f); tube(5.4f,cy,-9.6f,1.2f,false,7.0f,1.2f);
    // Diagonal ribs over the entrance hall in front of the shop, and over
    // each bay of the hall behind the curved wall.
    for(float yaw : {-49.0f,49.0f}) { Place rib(trs({1.2f,0,-8.65f},yaw)); box(0,cy-.11f,0,6.6f,.22f,.13f,trim); }
    const float ribY=F1-.28f;
    for(float x=4.0f;x<21.0f;x+=4.4f) for(float yaw : {-44.3f,44.3f}) {
        Place rib(trs({x+2.2f,0,-9.15f},yaw));
        box(0,ribY-.11f,0,6.0f,.22f,.13f,trim);
    }
    accentCluster(13.6f,PLINTH,-9.6f); accentCluster(.6f,PLINTH,-13.0f,true);
}

// ---------------------------------------------------------------------------
// corridor-with-stair.png and stair-behind-oval.png: a side corridor leaves
// the shop hall at the corner of the Shwapno front and runs toward the curved
// SWC wall. The stair stands inside that curve: one flight up to a landing
// against the white curved wall, and a return flight to the first floor.
// ---------------------------------------------------------------------------
void ovalStairHall() {
    const float x0=STAIR_LEFT_X,x1=STAIR_RIGHT_X,zHall=-7.0f,zStair=-2.0f,cy=F1-.28f,roof=7.8f;
    const float rise=STOREY_TALL/20,run=.28f;
    const Vec3 c{SWC_CURVE_CENTER_X,0,SWC_CURVE_CENTER_Z}; const float rin=SWC_CURVE_RADIUS-.44f;
    auto curveZ=[&](float x) { return c.z+std::sqrt(rin*rin-(x-c.x)*(x-c.x)); };
    // Flat slab from a straight edge out to the curved wall.
    auto toCurve=[&](float y,float zFrom,const Material& m,float up) {
        const int n=8;
        for(int i=0;i<n;++i) {
            float xa=x0+(x1-x0)*i/n,xb=x0+(x1-x0)*(i+1)/n;
            addQuad({xa,y,zFrom},{xb,y,zFrom},{xb,y,curveZ(xb)},{xa,y,curveZ(xa)},{0,up,0},m);
        }
    };
    // Corridor beside the shop: its flank wall carries the roller shutter,
    // the opposite wall a high band of barred windows.
    block(x0,0,zHall,x1,PLINTH,-3.2f,walkway);
    block(SHOP_RIGHT_X,0,zHall,x0,roof,-3.2f,plaster);
    block(x1,0,zHall,x1+.2f,roof,-3.2f,plaster);
    // The stair hall is the double-height space between the building and the
    // curved wall, closed by a partition at each end.
    const float hallLeft=8.6f,hallRight=20.4f;
    const float a0=std::asin((hallLeft-c.x)/rin),a1=std::asin((hallRight-c.x)/rin); const int segments=32;
    auto arc=[&](int i,float y) { float t=a0+(a1-a0)*i/segments; return Vec3{c.x+rin*std::sin(t),y,c.z+rin*std::cos(t)}; };
    // The end wall beside the portico is an open path: the stair hall is
    // entered straight from the entrance lobby.
    { const float wa=-2.85f,wb=curveZ(hallLeft)-.35f,head=PLINTH+2.8f;
      block(hallLeft-.2f,0,-3.2f,hallLeft,8.0f,wa,plaster);
      block(hallLeft-.2f,0,wb,hallLeft,8.0f,curveZ(hallLeft),plaster);
      block(hallLeft-.2f,head,wa,hallLeft,8.0f,wb,plaster);
      block(8.3f,0,wa,hallLeft,PLINTH,wb,walkway); }
    block(hallRight,0,-3.2f,hallRight+.2f,8.0f,curveZ(hallRight),plaster);
    addTriangle({(x0+x1)/2,PLINTH,-3.2f},{hallLeft,PLINTH,-3.2f},arc(0,PLINTH),{0,1,0},walkway);
    addTriangle({(x0+x1)/2,PLINTH,-3.2f},arc(segments,PLINTH),{hallRight,PLINTH,-3.2f},{0,1,0},walkway);
    for(int i=0;i<segments;++i)
        addTriangle({(x0+x1)/2,PLINTH,-3.2f},arc(i,PLINTH),arc(i+1,PLINTH),{0,1,0},walkway);
    // Flat roof over the whole hall, level with the curved wall's own roof.
    addTriangle({(x0+x1)/2,7.96f,-3.2f},{hallLeft,7.96f,-3.2f},arc(0,7.96f),{0,1,0},trim);
    addTriangle({(x0+x1)/2,7.96f,-3.2f},arc(segments,7.96f),{hallRight,7.96f,-3.2f},{0,1,0},trim);
    for(int i=0;i<segments;++i)
        addTriangle({(x0+x1)/2,7.96f,-3.2f},arc(i,7.96f),arc(i+1,7.96f),{0,1,0},trim);
    block(x0,cy,zHall,x1,F1,zStair,concrete);
    block(x0,cy-.02f,zHall,x1,cy,zStair,plaster);
    block(x0,F1,zHall,x1,F1+.012f,zStair,walkway);
    block(SHOP_RIGHT_X,roof-.35f,zHall,x1+.2f,roof,-3.2f,concrete);
    const Material slat{{.62f,.63f,.60f},.35f};
    block(x0,PLINTH,-6.3f,x0+.04f,3.2f,-4.1f,slat);
    for(float y=PLINTH+.04f;y<3.15f;y+=.11f) block(x0+.04f,y,-6.3f,x0+.07f,y+.05f,-4.1f,slat);
    for(float z : {-6.36f,-4.1f}) block(x0,PLINTH,z,x0+.09f,3.25f,z+.06f,metalDark);
    block(x0,3.2f,-6.4f,x0+.14f,3.45f,-4.0f,metal);
    block(x1-.03f,2.35f,-6.6f,x1,3.15f,-3.5f,daylight);
    for(float z=-6.6f;z<-3.45f;z+=.775f) block(x1-.08f,2.35f,z-.03f,x1,3.15f,z+.03f,whitePaint);
    for(float y=2.35f;y<3.2f;y+=.16f) block(x1-.06f,y,-6.6f,x1-.045f,y+.012f,-3.5f,whitePaint);
    cylinder((x0+x1)/2,cy-.09f,-5.0f,.09f,.18f,.09f,lampBlack,12);
    glow((x0+x1)/2,cy-.38f,-5.0f,7.0f,1.0f);
    accentTile(15.7f,PLINTH,-5.4f,greyTile); accentTile(16.9f,PLINTH,-5.4f,greyTile);

    // stair-behind-oval.png: one flight sweeps round the inside of the curved
    // wall, between the wall and a solid white balustrade that rises with it.
    // It starts beside the open path and arrives on a first-floor bridge
    // that crosses the hall to the walkway above the side corridor.
    const float rIn=rin-1.5f,rOut=rin-.04f,rMid=(rIn+rOut)/2;
    const float tTop=std::asin((x0-c.x)/rMid),tSpan=20*run/rMid,tBottom=tTop-tSpan;
    auto polar=[&](float t,float r,float y) { return Vec3{c.x+r*std::sin(t),y,c.z+r*std::cos(t)}; };
    // Solid sector between two angles and two radii.
    auto wedge=[&](float ta,float tb,float ra,float rb,float yb,float yt,const Material& m) {
        Vec3 a=polar(ta,ra,yb),b=polar(ta,rb,yb),e=polar(tb,rb,yb),d=polar(tb,ra,yb),up{0,yt-yb,0};
        addQuad(a+up,b+up,e+up,d+up,{0,1,0},m); addQuad(a,d,e,b,{0,-1,0},m);
        addQuad(a,b,b+up,a+up,m); addQuad(d,d+up,e+up,e,m); addQuad(a,a+up,d+up,d,m); addQuad(b,e,e+up,b+up,m);
    };
    for(int i=0;i<20;++i) {
        float ta=tBottom+tSpan*i/20,tb=tBottom+tSpan*(i+1)/20,y=PLINTH+(i+1)*rise,under=std::max(PLINTH,y-rise-.22f);
        wedge(ta,tb,rIn,rOut,under,y,trim);
        wedge(ta,tb,rIn-.12f,rIn,under,y+.95f,plaster);
        if(i%3==0) rod(polar(ta,rIn+.10f,y),polar(ta,rIn+.10f,y+.95f),.022f,steel,6);
        for(float h : {.50f,.72f,.95f})
            rod(polar(ta,rIn+.10f,y+h-rise),polar(tb,rIn+.10f,y+h),.018f,steel,6);
    }
    // First-floor bridge from the head of the flight to the walkway.
    const float zJoin=polar(tTop,rIn,0).z-.1f;
    block(x0,F1-.22f,zStair,x1,F1,2.0f,trim);
    toCurve(F1,2.0f,trim,1); toCurve(F1-.22f,2.0f,trim,-1);
    for(float h : {.38f,.64f,.90f}) {
        rod({x1-.05f,F1+h,zStair},{x1-.05f,F1+h,curveZ(x1-.05f)},.020f,steel,6);
        rod({x0+.05f,F1+h,zStair},{x0+.05f,F1+h,zJoin},.020f,steel,6);
    }
    for(float z=zStair;z<curveZ(x1-.05f);z+=1.2f) rod({x1-.05f,F1,z},{x1-.05f,F1+.92f,z},.024f,steel,6);
    for(float z=zStair;z<zJoin+.01f;z+=(zJoin-zStair)/3) rod({x0+.05f,F1,z},{x0+.05f,F1+.92f,z},.024f,steel,6);

    // White plaster lining on the inside of the curved brick wall, with the
    // tall mullioned landing window.
    for(int i=0;i<segments;++i) {
        float ta=a0+(a1-a0)*i/segments,tb=a0+(a1-a0)*(i+1)/segments;
        Vec3 pa=arc(i,0),pb=arc(i+1,0);
        Vec3 up{0,8.0f,0},na{-std::sin(ta),0,-std::cos(ta)},nb{-std::sin(tb),0,-std::cos(tb)};
        emit(pa,na,plaster); emit(pb+up,nb,plaster); emit(pb,nb,plaster);
        emit(pa,na,plaster); emit(pa+up,na,plaster); emit(pb+up,nb,plaster);
    }
    { float th=radians(13.0f);
      Place window(multiply(translation({c.x+(rin-.05f)*std::sin(th),0,c.z+(rin-.05f)*std::cos(th)}),rotationY(th+PI)));
      block(-.62f,2.7f,0,.62f,7.3f,.03f,daylight);
      for(float x : {-.62f,0.0f,.62f}) block(x-.03f,2.7f,0,x+.03f,7.3f,.07f,whitePaint);
      for(float y=2.7f;y<7.35f;y+=1.15f) block(-.62f,y-.03f,0,.62f,y+.03f,.07f,whitePaint); }
    cylinder((x0+x1)/2,roof-.44f,-5.0f,.09f,.18f,.09f,lampBlack,12);
    glow((x0+x1)/2,roof-.75f,-5.0f,7.5f,1.0f);
    for(float x : {12.2f,(x0+x1)/2,19.2f}) { glow(x,3.35f,-1.4f,7.0f,.9f); glow(x,7.0f,-1.2f,8.0f,1.0f); }
    accentCluster(11.6f,PLINTH,-1.6f);
}

// ---------------------------------------------------------------------------
// 6.14.08 PM (2): the cafeteria dining hall. Columns and deep beams divide a
// broad tiled room; dark tables fill it and ceiling fans hang between the beams.
// ---------------------------------------------------------------------------
void cafeteriaHall() {
    // L-shaped hall: the bays behind the CAFETERIA sign plus the bays running
    // back along the courtyard corridor as far as the glazed rooms.
    const float x0=3.55f,x1=20.0f,xe=18.95f,z0=CAFE_WORLD_Z-13.0f,zm=CAFE_WORLD_Z,z1=rooms::back,cy=F1-.32f;
    const float westA=-40.5f,westB=-37.7f,stairZ=-41.1f,wallEnd=-26.2f;
    block(x0,0,z0,x1,PLINTH,zm,walkway);    block(x0,0,zm,xe,PLINTH,z1,walkway);
    block(x0,cy,z0,x1,11.7f,zm,concrete);   block(x0,cy,zm,xe,11.7f,z1,concrete);
    block(x0,cy-.02f,z0,x1,cy,zm,plaster);  block(x0,cy-.02f,zm,xe,cy,z1,plaster);
    block(x0,PLINTH,z0,x1,cy,z0+.3f,plaster);
    block(x0,PLINTH,z1-.3f,xe,cy,z1,plaster);
    block(xe-.35f,PLINTH,zm,xe,cy,z1,plaster);
    block(xe-.35f,PLINTH,zm-.3f,x1,cy,zm,plaster);
    block(x0,PLINTH,z0,x0+.35f,cy,stairZ,plaster);          // solid beside the corridor-end stair
    // East wall either side of the recessed entrance built by cafeteria().
    block(x1-.4f,PLINTH,z0,x1,cy,CAFE_WORLD_Z-8.8f,plaster);
    block(x1-.4f,PLINTH,CAFE_WORLD_Z-3.5f,x1,cy,zm,plaster);

    // 6.14.08 PM (1), 6.14.09 PM: the corridor wall is glazed, so the tables
    // are seen from the corridor. Brick panel, barred window, transom, top light.
    const float wx0=3.25f,wx1=3.55f;
    block(2.8f,0,stairZ,x0,PLINTH,wallEnd,walkway);
    block(2.8f,PLINTH+3.0f,stairZ,x0,F1,wallEnd,concrete);
    block(wx0,PLINTH,stairZ,wx1,PLINTH+3.0f,westA,wallWhite);
    block(wx0,PLINTH+2.6f,westA,wx1,PLINTH+3.0f,westB,wallWhite);
    block(wx0,PLINTH,z1,wx1,PLINTH+3.0f,wallEnd,wallWhite);
    const int modules=4; const float mw=(z1-westB)/modules;
    for(int m=0;m<=modules;++m) {
        float a=westB+m*mw,b=a+mw;
        block(wx0-.05f,PLINTH,a-.15f,wx1,PLINTH+3.0f,a+.15f,wallWhite);
        if(m==modules) break;
        block(wx0,PLINTH,a+.15f,wx1,PLINTH+.85f,b-.15f,brickLight);
        block(wx0-.05f,PLINTH+.85f,a+.15f,wx1+.03f,PLINTH+.93f,b-.15f,trim);
        block(wx0,PLINTH+2.25f,a+.15f,wx1,PLINTH+2.45f,b-.15f,wallWhite);
        block(wx0,PLINTH+.93f,(a+b)/2-.03f,wx1,PLINTH+3.0f,(a+b)/2+.03f,whitePaint);
        roomGlass.push_back({{(wx0+wx1)/2,PLINTH+1.59f,(a+b)/2},mw-.3f,1.32f,0,true});
        roomGlass.push_back({{(wx0+wx1)/2,PLINTH+2.725f,(a+b)/2},mw-.3f,.55f,0,true});
        for(float y=PLINTH+1.08f;y<PLINTH+2.2f;y+=.165f) block(wx0+.02f,y,a+.15f,wx0+.035f,y+.012f,b-.15f,whitePaint);
        for(int k=1;k<10;++k) { float z=a+.15f+(mw-.3f)*k/10; block(wx0+.02f,PLINTH+.93f,z-.006f,wx0+.035f,PLINTH+2.25f,z+.006f,whitePaint); }
    }

    const float rows[5]={-45.4f,-41.2f,-37.0f,-32.8f,-28.9f};
    for(float x : {9.5f,15.0f}) {
        block(x-.2f,cy-.35f,z0,x+.2f,cy,z1,plaster);
        for(float z : rows) box(x,(PLINTH+cy)/2,z,.5f,cy-PLINTH,.5f,plaster);
    }
    for(float z : rows) block(x0,cy-.28f,z-.16f,z>zm ? xe : x1,cy,z+.16f,plaster);
    for(int k=0;k<5;++k) { Place south(translation({4.6f+k*3.0f,0,z0+.3f})); interiorWindow(2.6f,PLINTH,3); }
    { Place east(trs({x1-.4f,0,z0+.6f},-90)); interiorWindow(3.0f,PLINTH,3); }
    { Place east(trs({x1-.4f,0,zm-3.3f},-90)); interiorWindow(2.6f,PLINTH,3); }
    for(int k=0;k<3;++k) { Place east(trs({xe-.35f,0,zm+.7f+k*3.1f},-90)); interiorWindow(2.6f,PLINTH,3); }
    { Place kitchen(trs({11.0f,0,z1-.3f},180)); timberDoor(0,PLINTH,1.7f); }
    // Tables keep the door axis and the corridor opening clear.
    for(float x : {9.9f,11.9f,13.4f,16.2f,18.0f}) diningSet(x,-48.0f);
    for(float x : {5.6f,7.6f,11.4f,13.2f,17.0f}) diningSet(x,-45.9f);
    for(float x : {8.0f,11.4f,13.2f,17.0f}) diningSet(x,-39.3f);
    for(float z : {-35.0f,-30.8f}) for(float x : {5.6f,7.6f,11.4f,13.2f,17.0f}) diningSet(x,z);
    // Service counter with a menu band above it.
    const Material counter{{.72f,.60f,.44f},.3f,INTERIOR};
    block(4.2f,PLINTH,-49.3f,8.6f,PLINTH+1.0f,-48.6f,counter);
    block(4.1f,PLINTH+1.0f,-49.35f,8.7f,PLINTH+1.05f,-48.5f,Material{{.30f,.30f,.30f},.5f,INTERIOR});
    block(4.2f,PLINTH+2.25f,-49.2f,8.6f,PLINTH+2.9f,-49.15f,Material{{.62f,.12f,.08f},.0f,EMISSIVE});
    for(int k=0;k<5;++k)
        block(4.4f+k*.85f,PLINTH+2.35f,-49.15f,5.0f+k*.85f,PLINTH+2.8f,-49.14f,Material{{.96f,.94f,.86f},.0f,EMISSIVE});
    for(float x : {6.6f,12.2f,17.6f}) for(float z : {-47.1f,-43.3f,-39.2f}) {
        if(z>-40.0f&&x<7.0f) continue;
        ceilingFan(x,cy,z);
    }
    for(float x : {6.6f,12.2f,17.0f}) for(float z : {-34.9f,-30.7f}) ceilingFan(x,cy,z);
    for(float x : {6.6f,12.2f,17.6f}) for(float z : {-45.0f,-40.6f}) tube(x,cy,z,1.2f,true,8.5f,1.1f);
    for(float x : {6.6f,12.2f,17.0f}) for(float z : {-36.3f,-29.6f}) tube(x,cy,z,1.2f,true,8.5f,1.1f);
    tube(12.2f,cy,-48.6f,1.2f,true,8.0f,.9f);
    accentCluster(12.0f,PLINTH,-43.3f); accentCluster(5.6f,PLINTH,-39.4f,true); accentCluster(11.6f,PLINTH,-33.0f);
}

// ---------------------------------------------------------------------------
// 6.14.11 PM, 6.14.11 PM (2), 6.14.12 PM: the first-floor terrace beside the
// gallery. Deep white portal frames span it; a brick-inset parapet with a
// steel rail overlooks the lawn; red planters stand in two rows.
// ---------------------------------------------------------------------------
// Joins the three-storey block to the stage wing at the courtyard corner, on
// both levels: a paved link at ground level and a bridge from the terrace to
// the stage-wing gallery above it. The stage wing floor is 0.15 lower at the
// upper level, which is one shallow step.
void wingLink() {
    const float xe=TERRACE_EDGE_X,xr=COURTYARD_RIGHT,zTerrace=-41.1f,zStageLower=-43.8f,zStageUpper=-43.95f;
    block(xe,0,zStageLower,xr,PLINTH,zTerrace,walkway);
    block(xe,PLINTH,zStageLower,xe+.14f,PLINTH+.004f,zTerrace,darkTile);
    block(xe,F1-.32f,zStageUpper,xr,F1,zTerrace,concrete);
    block(xe,F1,zStageUpper,xr,F1+.012f,zTerrace,Material{{.84f,.85f,.82f},.30f,TILE});
    // Parapet on the courtyard side of the bridge, continuing the terrace parapet.
    { Place edge(trs({xe,0,zStageUpper},-90)); balconyPanel(.10f,zTerrace-zStageUpper-.30f,F1); }
    cylinder((xe+xr)/2,F1-.41f,(zTerrace+zStageLower)/2,.09f,.18f,.09f,lampBlack,12);
    glow((xe+xr)/2,F1-.72f,(zTerrace+zStageLower)/2,7.0f,.9f);
}

void courtyardTerrace() {
    const float xe=TERRACE_EDGE_X,xi=COURTYARD_RIGHT-.30f,bay=29.8f/6,z0=-41.1f,z1=z0+5*bay;
    const float beamY=F1+2.95f,frameTop=F1+3.75f;
    const Material terraceTile{{.84f,.85f,.82f},.30f,TILE};
    // Ground level: the corridor becomes two bays wide (6.14.11 PM (1)).
    block(xe,0,z0,xi-.15f,PLINTH,z1,walkway);
    block(xe,PLINTH,z0,xe+.14f,PLINTH+.004f,z1,darkTile);
    block(xe,F1-.32f,z0,xi,F1,z1,concrete);
    block(xe,F1,z0,xi,F1+.012f,z1,terraceTile);
    for(int k=0;k<=5;++k) {
        float z=z0+k*bay;
        block(xe,PLINTH,z-.28f,xe+.56f,frameTop,z+.28f,trim);
        block(xe,beamY,z-.22f,COURTYARD_RIGHT,frameTop,z+.22f,trim);
        block(xe,F1-.62f,z-.14f,xi,F1-.32f,z+.14f,concrete);
        if(k<5) {
            Place edge(trs({xe,0,z},-90));
            balconyPanel(.30f,bay-.30f,F1);
            cylinder(bay/2,F1-.41f,-1.6f,.09f,.18f,.09f,lampBlack,12);
        }
    }
    // The north end is open: wingLink() carries the terrace across to the stage wing.
    wingLink();
    { Place end(translation({xe+.56f,0,z1})); balconyPanel(0,xi-xe-.56f,F1); }
    // Planter rows along both edges, a blue bench, and grey accent tiles.
    for(int k=0;k<10;++k) {
        float z=z0+1.4f+k*2.45f;
        if(z<-22.2f||z>-19.9f) planter(xe+.85f,F1+.012f,z,k*1.3f);
        if(k%2==0) planter(xi-.25f,F1+.012f,z+1.1f,k*2.1f+7);
    }
    for(float z : {-19.4f,-28.6f,-37.2f}) accentTile(-2.6f,F1+.012f,z,greyTile);
    for(int s=0;s<3;++s) {
        float z=-21.6f+s*.56f;
        box(xe+1.05f,F1+.46f,z,.46f,.07f,.50f,seatBlue);
        box(xe+.84f,F1+.72f,z,.06f,.46f,.50f,seatBlue);
    }
    box(xe+1.05f,F1+.38f,-21.04f,.08f,.08f,1.7f,steel);
    for(float z : {-21.7f,-20.4f}) box(xe+1.05f,F1+.19f,z,.42f,.38f,.05f,steel);
    for(float z : {-19.0f,-30.2f,-37.4f}) accentCluster(-2.6f,PLINTH,z,true);
}

// ---------------------------------------------------------------------------
// 6.14.13 PM, (1), (2) and 6.14.14 PM: the open first-floor hall. Timber
// double doors with louvred heads, tandem waiting seats and a waste bin.
// ---------------------------------------------------------------------------
void upperHall() {
    // 6.14.13 PM (1): the open upper hall beside the entrance corridor. Its white
    // west wall, the side of the front-left wing, carries two timber double
    // doors with louvred heads and a room label; a mat lies before the first.
    for(float z : {-9.6f,-7.55f}) { Place wall(trs({-1.1f,0,z},90)); timberDoor(0,F1,1.7f); }
    block(-1.1f,F1+1.9f,-8.72f,-1.07f,F1+2.1f,-8.42f,Material{{.93f,.93f,.90f},.2f});
    block(-1.0f,F1,-10.4f,.3f,F1+.012f,-8.8f,Material{{.62f,.42f,.28f},.15f});
    // The door wall runs on past its old corner, so the hall is wider, and the
    // two pillars that stood at that corner are left out at this level. Tandem
    // seats and the green waste bin stand against the wall.
    block(-1.3f,F1,-14.8f,-1.1f,PLINTH+2*STOREY_TALL-.32f,-10.55f,wallWhite);
    for(int s=0;s<3;++s) {
        float z=-13.9f+s*.56f;
        box(-.72f,F1+.46f,z,.46f,.07f,.50f,seatBlue);
        box(-.93f,F1+.72f,z,.06f,.46f,.50f,seatBlue);
    }
    box(-.72f,F1+.38f,-13.34f,.08f,.08f,1.7f,steel);
    for(float z : {-14.0f,-12.7f}) box(-.72f,F1+.19f,z,.42f,.38f,.05f,steel);
    box(-.72f,F1+.52f,-11.4f,.55f,1.0f,.55f,Material{{.12f,.36f,.32f},.3f,INTERIOR});
    // Beyond the wall the hall is open to the courtyard behind a railed parapet.
    { Place edge(trs({COURTYARD_RIGHT-.30f,0,-16.0f},-90)); balconyPanel(0,1.15f,F1); }
    accentCluster(1.4f,F1,-12.6f);
    // Farther along, the hall above the shops opens onto the bridge from the curved stair.
    for(int s=0;s<3;++s) {
        float x=18.6f+s*.56f;
        box(x,F1+.46f,-7.62f,.50f,.07f,.46f,seatBlue);
        box(x,F1+.72f,-7.38f,.50f,.46f,.06f,seatBlue);
    }
    box(19.16f,F1+.38f,-7.62f,1.7f,.08f,.08f,steel);
    for(float x : {18.5f,19.8f}) box(x,F1+.19f,-7.62f,.05f,.38f,.42f,steel);
    box(21.0f,F1+.52f,-7.55f,.55f,1.0f,.55f,Material{{.12f,.36f,.32f},.3f,INTERIOR});
    accentCluster(12.6f,F1+.016f,-9.6f); accentCluster(6.6f,F1+.016f,-9.0f);
    for(float x : {6.2f,12.8f,19.4f}) planter(x,F1+.015f,-11.0f,x);
}

// ---------------------------------------------------------------------------
// 6.14.07--6.14.11 PM: ground corridors and the link hall.
// ---------------------------------------------------------------------------
void corridorDetails() {
    // Black-and-grey tile groups along the corridors and in the link hall.
    accentCluster(.6f,PLINTH,-27.0f,true); accentCluster(.6f,PLINTH,-34.4f,true);
    accentCluster(9.0f,PLINTH,-19.2f); accentCluster(19.2f,PLINTH,-19.2f);
    accentCluster(.6f,F1,-24.0f,true); accentCluster(.6f,F1,-33.0f,true);
    accentCluster(5.0f,F1,-14.2f);
    // corridor-with-door.png: roller shutter on the shop's flank, in the corridor
    // that runs from the far end of the shop to the right-hand entrance.
    const Material slat{{.62f,.63f,.60f},.35f};
    block(4.2f,PLINTH,-17.25f,7.0f,3.25f,-17.2f,slat);
    for(float y=PLINTH+.04f;y<3.2f;y+=.11f) block(4.2f,y,-17.28f,7.0f,y+.05f,-17.25f,slat);
    for(float x : {4.14f,7.0f}) block(x,PLINTH,-17.30f,x+.06f,3.3f,-17.2f,metalDark);
    block(4.14f,3.25f,-17.34f,7.06f,3.5f,-17.2f,metal);
}

// Ceiling fixtures of the covered spaces, as point lights for the shader and
// the ray tracer. The fixtures themselves are built with each wing.
void interiorLighting() {
    const float c0=F1-.32f-.38f,c1=PLINTH+2*STOREY_TALL-.32f-.38f,c2=PLINTH+3*STOREY_TALL-.38f;
    const float bay=29.8f/6;
    for(int b=0;b<6;++b) {
        float z=-41.1f+(b+.5f)*bay;
        glow(1.2f,c0,z); glow(1.2f,c1,z);
        if(b%2==0) glow(1.2f,c2,z);
        if(b<5) glow(-2.4f,c0,z,7.5f,.9f);
    }
    for(float z : {-8.5f,-10.6f}) glow(ENTRANCE_AXIS,c0,z,7.0f,.9f);
    for(float x : {6.0f,11.2f,16.3f,21.5f,25.6f}) glow(x,c0,-18.7f);
    for(float x : {6.0f,10.5f,15.0f,19.5f}) { glow(x,c0,-9.1f); glow(x,c1+.3f,-9.1f); }
    for(float z : {-13.5f,-17.5f}) { glow(23.5f,c0,z,7.0f,.9f); glow(23.5f,c1+.3f,z,7.0f,.9f); }
    for(float y : {c0,c1,c2}) { glow(5.2f,y,-13.4f,7.0f,.9f); glow(1.6f,y,-43.4f,7.0f,.9f); }
    for(float x : {-17.0f,-12.2f,-7.4f,-2.8f}) { glow(x,c0,-12.9f,7.0f,.9f); glow(x,c1,-12.9f,7.0f,.9f); }
    // Stage-wing galleries and the shop arcade.
    for(int b=0;b<7;b+=2) glow(STAGE_WING_LEFT+(b+.5f)*STAGE_BAY,PLINTH+STOREY-.7f,-45.9f,7.5f,.8f);
    for(float z : {-9.0f,-13.5f,-18.0f}) glow(28.6f,c0,z,7.0f,.9f);
}

void buildInteriors() {
    entranceLobby();
    gamesHall();
    shopHall();
    ovalStairHall();
    cafeteriaHall();
    courtyardTerrace();
    upperHall();
    corridorDetails();
    interiorLighting();
}

}} // namespace swc::app

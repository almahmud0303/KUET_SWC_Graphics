#include "scene/front.h"

#include "geometry/mesh.h"
#include "scene/components.h"
#include "scene/layout.h"
#include "scene/lettering.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Front: long colonnade, main entrance portico and balcony return
// ---------------------------------------------------------------------------
void frontWing() {
    // Continuous raised plinth and broad steps along the whole front.
    block(-27.5f,0,-4.6f,8.3f,PLINTH,2.2f,walkway);
    block(-27.5f,0,2.05f,8.3f,PLINTH+.004f,2.2f,darkTile);
    stair(-9.6f,3.2f,35.8f,3,.15f,.33f,0,trim);
    // Front-left windowed wing behind double-height free columns (photos 23, 24, 27).
    { Place p(trs({-27,0,-2.6f},0));
      windowedWing({5,4.8f,2,STOREY,3,4.5f,true,true,false,false},11.0f); }
    // Entrance canopy continues to the curved brick volume.
    const float roofUnder=PLINTH+2*STOREY;
    block(-2.65f,roofUnder,-10.55f,8.3f,roofUnder+SLAB,1.9f,trim);
    block(-2.65f,roofUnder-.18f,1.62f,8.3f,roofUnder,1.9f,trim);
    for(float x : {.2f,3.4f,6.6f}) box(x,(PLINTH+roofUnder)/2,1.0f,.56f,roofUnder-PLINTH,.56f,trim);
    for(float x : {.2f,3.4f}) box(x,(PLINTH+roofUnder)/2,-3.0f,.50f,roofUnder-PLINTH,.50f,trim);
    for(float x=-1.5f;x<8.0f;x+=2.4f) for(float z : {-2.0f,.6f}) cylinder(x,roofUnder-.07f,z,.08f,.14f,.08f,lampBlack,10);
    // Tall warm-brick wall left of the doors.
    block(-3.0f,PLINTH,-4.8f,-1.1f,roofUnder,-4.3f,warmPanel);
    // Recessed timber entrance: open centre leads through to the rear corridor.
    const float dz=-6.0f;
    const float landing=PLINTH+STOREY_TALL;
    block(-1.1f,PLINTH+3.3f,dz-.3f,3.5f,landing,dz+.1f,concrete);
    for(int leaf=0;leaf<4;++leaf) {
        float x0=-1.1f+leaf*1.15f,x1=x0+1.15f;
        if(leaf!=2) block(x0,PLINTH,dz-.2f,x0+.06f,PLINTH+3.3f,dz+.05f,trim);
        if(leaf==0||leaf==3) {
            block(x0+.06f,PLINTH,dz-.2f,x1-.03f,PLINTH+2.6f,dz-.05f,wood);
            for(float gx : {.3f,.57f,.84f}) block(x0+gx,PLINTH+.1f,dz-.05f,x0+gx+.03f,PLINTH+2.5f,dz-.03f,woodGroove);
        }
        block(x0+.06f,PLINTH+2.6f,dz-.2f,x1-.03f,PLINTH+2.66f,dz,trim);
        for(int s=0;s<8;++s) block(x0+.08f,PLINTH+2.70f+s*.075f,dz-.15f,x1-.05f,PLINTH+2.73f+s*.075f,dz-.02f,wood);
    }
    block(-1.1f,0,-11.3f,3.5f,PLINTH,-4.6f,walkway);
    // Upper floor above the doors: the rear balcony edge seen through the portico.
    block(-1.1f,landing-.32f,-9.0f,3.5f,landing,dz+.1f,concrete);
    { Place p(translation({0,0,dz+.1f})); balconyPanel(-1.1f,3.5f,landing); }
    block(3.5f,PLINTH,-6.3f,5.5f,PLINTH+3.95f,-5.8f,warmPanel);
    // Balcony return between the entrance and the curved wall (photos 26, 30).
    block(5.5f,PLINTH+3.55f,-3.2f,8.3f,PLINTH+3.95f,-.3f,concrete);
    { Place p(translation({0,0,-.3f})); balconyPanel(5.5f,8.3f,PLINTH+3.95f); }
    { Place p(translation({0,0,-3.2f}));
      windowBay(6.9f,PLINTH,1.6f,301.0f); windowBay(6.9f,PLINTH+STOREY,1.6f,302.0f); }
    // Building masses behind the facades (roofs are the tops of these blocks).
    block(-26.25f,0,-10.55f,-3.0f,7.8f,-3.35f,concrete);
    block(-3.0f,0,-10.55f,-1.1f,7.8f,-4.8f,concrete);
    // Leave the strip behind these rooms hollow for the connecting corridor.
    block(3.5f,0,-7.0f,5.5f,7.8f,-6.3f,concrete);
    block(5.5f,0,-7.0f,21.25f,7.8f,-3.2f,concrete);
}

// Straight passage from the entrance door into the relocated three-storey
// corridor. Its centre stays on ENTRANCE_AXIS all the way through the building.
void entranceCorridor() {
    const float first=PLINTH+STOREY_TALL,roof=PLINTH+2*STOREY_TALL;
    const float rear=-14.5f+TALL_BLOCK_OFFSET.z;
    block(-1.1f,0,rear,3.5f,PLINTH,-6.0f,walkway);
    block(-1.1f,first-.32f,rear,3.5f,first,-9.0f,concrete);
    block(-1.1f,roof,rear,3.5f,roof+SLAB,-6.0f,trim);
    // Open side doors into the new transverse corridor on both levels.
    block(3.5f,PLINTH,rear,3.75f,roof,-10.8f,concrete);
    block(3.5f,PLINTH,-7.5f,3.75f,roof,-7.0f,concrete);
    block(3.5f,PLINTH+2.8f,-10.8f,3.75f,first,-7.5f,concrete);
    block(3.5f,first+2.8f,-10.8f,3.75f,roof,-7.5f,concrete);
    for(float floor : {PLINTH,first}) {
        // Longitudinal tile borders emphasize the straight route through the door.
        for(float x : {COURTYARD_RIGHT+.15f,ENTRANCE_AXIS+1.45f})
            block(x,floor,rear,x+.10f,floor+.008f,-6.35f,darkTile);
        for(int i=0;i<3;++i) {
            float x=i%2 ? 1.65f : .25f,z=-7.1f-i*1.4f;
            block(x,floor,z,x+.55f,floor+.008f,z+.55f,darkTile);
        }
        const float ceiling=floor==PLINTH ? first-.32f : roof;
        for(float z : {-8.1f,-10.5f}) {
            block(-1.1f,ceiling-.30f,z-.12f,3.5f,ceiling,z+.12f,concrete);
            cylinder(ENTRANCE_AXIS,ceiling-.1f,z+.5f,.09f,.18f,.09f,lampBlack,12);
            block(2.9f,ceiling-.07f,z-.55f,3.0f,ceiling-.02f,z+.55f,lampWhite);
        }
    }
}

// Two return flights per storey, with open stairwells and corridor-level landings.
void connectedStairwell() {
    const float top=PLINTH+3*STOREY_TALL;
    block(20.5f,0,-20.0f,25.5f,PLINTH,-14.2f,walkway);
    block(25.1f,PLINTH,-20.0f,25.5f,top,-14.2f,concrete);
    block(20.5f,PLINTH,-20.0f,25.5f,top,-19.75f,concrete);
    block(20.5f,top,-20.0f,25.5f,top+SLAB,-14.2f,trim);
    for(int level=0;level<3;++level) {
        float f=PLINTH+level*STOREY_TALL;
        if(level>0) block(20.5f,f-.22f,-15.9f,25.1f,f,-14.2f,concrete);
        { Place edge(translation({0,0,-14.2f})); balconyPanel(20.8f,25.1f,f); }
        if(level==2) continue;
        const float rise=STOREY_TALL/20,run=.29f;
        block(21.1f,f+STOREY_TALL/2-.22f,-19.75f,24.7f,f+STOREY_TALL/2,-18.8f,concrete);
        for(int i=0;i<10;++i) {
            float y=f+(i+1)*rise,z=-15.9f-i*run;
            block(21.1f,y-rise,z-run,22.65f,y,z,trim);
            float yr=f+STOREY_TALL/2+(i+1)*rise,zr=-18.8f+i*run;
            block(23.05f,yr-rise,zr,24.6f,yr,zr+run,trim);
            if(i%2==0) {
                rod({22.65f,y,z-run/2},{22.65f,y+.9f,z-run/2},.022f,metal,6);
                rod({23.05f,yr,zr+run/2},{23.05f,yr+.9f,zr+run/2},.022f,metal,6);
            }
        }
        rod({22.65f,f+rise+.9f,-15.9f},{22.65f,f+STOREY_TALL/2+.9f,-18.8f},.028f,metal,8);
        rod({23.05f,f+STOREY_TALL/2+rise+.9f,-18.8f},{23.05f,f+STOREY_TALL+.9f,-15.9f},.028f,metal,8);
    }
}

// ---------------------------------------------------------------------------
// Curved brick SWC facade (section 4): partial arc with a flared left edge,
// pale bands, louver strips with fins and bilingual lettering.
// ---------------------------------------------------------------------------
void curvedBrickFacade() {
    const Vec3 c{SWC_CURVE_CENTER_X,0,-4.5f};
    const float R=SWC_CURVE_RADIUS,thick=.40f,thetaL=radians(-68.0f),thetaR=radians(95.0f),flare=radians(12.0f);
    const float hL=9.4f,hR=8.5f; const int N=60,M=14;
    auto theta=[&](float u,float y) { float start=thetaL-flare*(y/hL); return start+(thetaR-start)*u; };
    auto topAt=[&](float u) { return hL+(hR-hL)*u; };
    auto at=[&](float th,float y,float r) { return Vec3{c.x+r*std::sin(th),y,c.z+r*std::cos(th)}; };
    auto radial=[](float th) { return Vec3{std::sin(th),0,std::cos(th)}; };
    auto surface=[&](float r,float sign,const Material& m,float yFrom,float yTo) {
        for(int i=0;i<N;++i) for(int j=0;j<M;++j) {
            float u0=static_cast<float>(i)/N,u1=static_cast<float>(i+1)/N;
            float ya0=yFrom+(std::min(yTo,topAt(u0))-yFrom)*j/M,ya1=yFrom+(std::min(yTo,topAt(u0))-yFrom)*(j+1)/M;
            float yb0=yFrom+(std::min(yTo,topAt(u1))-yFrom)*j/M,yb1=yFrom+(std::min(yTo,topAt(u1))-yFrom)*(j+1)/M;
            float t00=theta(u0,ya0),t01=theta(u0,ya1),t10=theta(u1,yb0),t11=theta(u1,yb1);
            emit(at(t00,ya0,r),radial(t00)*sign,m); emit(at(t10,yb0,r),radial(t10)*sign,m); emit(at(t11,yb1,r),radial(t11)*sign,m);
            emit(at(t00,ya0,r),radial(t00)*sign,m); emit(at(t11,yb1,r),radial(t11)*sign,m); emit(at(t01,ya1,r),radial(t01)*sign,m);
        }
    };
    surface(R,1,brick,0,99); surface(R-thick,-1,brick,0,99);
    surface(R+.035f,1,brickDark,0,.55f);
    for(int i=0;i<N;++i) {
        float u0=static_cast<float>(i)/N,u1=static_cast<float>(i+1)/N,y0=topAt(u0),y1=topAt(u1);
        float a=theta(u0,y0),b=theta(u1,y1);
        addQuad(at(a,y0,R),at(b,y1,R),at(b,y1,R-thick),at(a,y0,R-thick),{0,1,0},trim);
    }
    for(int j=0;j<M;++j) {
        float y0=hL*j/M,y1=hL*(j+1)/M,a=theta(0,y0),b=theta(0,y1);
        addQuad(at(a,y0,R-thick),at(a,y0,R),at(b,y1,R),at(b,y1,R-thick),{-std::cos(a),0,std::sin(a)},brick);
        float r0=hR*j/M,r1=hR*(j+1)/M;
        addQuad(at(thetaR,r0,R),at(thetaR,r0,R-thick),at(thetaR,r1,R-thick),at(thetaR,r1,R),{std::cos(thetaR),0,-std::sin(thetaR)},brick);
    }
    // Pale horizontal bands wrapping the curve.
    for(float yb : {1.0f,3.0f,4.9f,6.9f,8.25f}) for(int i=0;i<N;++i) {
        float u0=static_cast<float>(i)/N,u1=static_cast<float>(i+1)/N;
        if(yb+.07f>std::min(topAt(u0),topAt(u1))) continue;
        float a=theta(u0,yb),b=theta(u1,yb),ro=R+.06f,ri=R-.01f;
        addQuad(at(a,yb-.07f,ro),at(b,yb-.07f,ro),at(b,yb+.07f,ro),at(a,yb+.07f,ro),radial((a+b)/2),trim);
        addQuad(at(a,yb+.07f,ro),at(b,yb+.07f,ro),at(b,yb+.07f,ri),at(a,yb+.07f,ri),{0,1,0},trim);
        addQuad(at(a,yb-.07f,ri),at(b,yb-.07f,ri),at(b,yb-.07f,ro),at(a,yb-.07f,ro),{0,-1,0},trim);
    }
    // Tall louver strips with projecting pale fins.
    for(float deg : {-31.0f,-27.0f,19.0f,62.0f}) {
        float th=radians(deg);
        Place p(multiply(translation(at(th,0,R)),rotationY(th)));
        block(-.30f,1.07f,-.06f,.30f,6.83f,.02f,darkGap);
        for(float y=1.12f;y<6.8f;y+=.13f) block(-.28f,y,-.02f,.28f,y+.045f,.10f,trim);
        block(-.40f,.95f,-.02f,-.30f,6.95f,.34f,trim); block(.30f,.95f,-.02f,.40f,6.95f,.34f,trim);
    }
    // Bilingual lettering following the arc.
    auto arcText=[&](const std::vector<std::string>& glyphs,float thetaMid,float y,float sx,float sy,const Material& m) {
        float total=(glyphs.size()*6-1)*sx;
        for(std::size_t k=0;k<glyphs.size();++k)
            for(int col=0;col<5;++col) {
                float s=-total/2+(k*6+col)*sx+sx*.43f,th=thetaMid+s/R;
                Place p(multiply(translation(at(th,0,R)),rotationY(th)));
                for(int row=0;row<5;++row)
                    if(glyphs[k][row*5+col]=='1')
                        block(-sx*.43f,y+(4-row)*sy,.0f,sx*.43f,y+(4-row)*sy+sy*.86f,.06f,m);
            }
    };
    const Material letters{{.90f,.90f,.86f},.45f};
    arcText(latinGlyphs("STUDENT WELFARE CENTER"),radians(-4.0f),5.30f,.036f,.064f,letters);
    arcText(bengaliGlyphs({6,6,5},9.0f),radians(-4.0f),5.95f,.052f,.078f,letters);
    // Flat roof hidden inside the curved parapet.
    for(int i=0;i<N;++i) {
        float a=theta(static_cast<float>(i)/N,8.0f),b=theta(static_cast<float>(i+1)/N,8.0f);
        addTriangle({c.x,8.0f,c.z},at(b,8.0f,R-thick),at(a,8.0f,R-thick),{0,1,0},roofMembrane);
    }
    // The right-side canopy belongs to the continuous two-storey shop wing.
}

}} // namespace swc::app

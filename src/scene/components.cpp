#include "scene/components.h"

#include "geometry/mesh.h"
#include "scene/layout.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Reusable components (all in a local facade frame: facade plane z=0,
// outward +Z, running along +X, y measured from the ground).
// ---------------------------------------------------------------------------

// WindowBay: brick panel, projecting brick sill, framed glazing with dense
// security bars, a concrete transom band and a short transom light above.
void windowBay(float x,float floorY,float w,float seed,bool transom) {
    float x0=x-w/2,x1=x+w/2;
    block(x0,floorY,-.34f,x1,floorY+.80f,-.12f,brickLight);
    block(x0-.05f,floorY+.80f,-.30f,x1+.05f,floorY+.90f,.06f,brick);
    float g0=floorY+.90f,g1=floorY+2.36f;
    int panes=w>1.55f ? 3 : 2;
    for(int i=0;i<panes;++i)
        block(x0+w*i/panes,g0,-.31f,x0+w*(i+1)/panes,g1,-.27f,glassFor(seed+i*7.3f));
    block(x0,g0,-.27f,x1,g0+.05f,-.21f,metal); block(x0,g1-.05f,-.27f,x1,g1,-.21f,metal);
    for(int i=0;i<=panes;++i) { float mx=x0+w*i/panes; block(mx-.03f,g0,-.27f,mx+.03f,g1,-.21f,metal); }
    int bars=std::max(4,static_cast<int>(w/.11f));
    for(int k=1;k<bars;++k) { float bx=x0+w*k/bars; block(bx-.009f,g0,-.20f,bx+.009f,g1,-.18f,metal); }
    for(float hy : {g0+.36f,g0+.73f,g0+1.10f}) block(x0,hy-.012f,-.20f,x1,hy+.012f,-.18f,metal);
    block(x0,g1,-.36f,x1,g1+.16f,-.04f,concrete);
    if(!transom) return;
    float t0=g1+.16f,t1=floorY+3.0f;
    for(int i=0;i<panes;++i)
        block(x0+w*i/panes,t0,-.31f,x0+w*(i+1)/panes,t1,-.27f,glassFor(seed+i*3.7f+40.0f));
    for(int i=0;i<=panes;++i) { float mx=x0+w*i/panes; block(mx-.03f,t0,-.27f,mx+.03f,t1,-.21f,metal); }
    for(int k=1;k<bars;k+=2) { float bx=x0+w*k/bars; block(bx-.009f,t0,-.20f,bx+.009f,t1,-.18f,metal); }
}

// ACUnit: outdoor compressor on steel brackets with a refrigerant pipe.
void acUnit(float x,float y,float z) {
    block(x-.42f,y,z,x+.42f,y+.58f,z+.30f,acBody);
    { Place p(multiply(translation({x-.08f,y+.29f,z+.305f}),rotationX(PI/2)));
      cylinder(0,0,0,.21f,.015f,.21f,metalDark,16); }
    block(x+.26f,y+.08f,z+.30f,x+.38f,y+.50f,z+.31f,metalDark);
    for(float bx : {x-.34f,x+.34f}) block(bx-.02f,y-.05f,z-.25f,bx+.02f,y,z+.34f,metalDark);
    rod({x+.30f,y+.40f,z+.05f},{x+.48f,y+1.55f,z-.10f},.018f,trim,6);
}

void door(float x,float floorY,float seed,float w,float h) {
    block(x-w/2-.08f,floorY,-.36f,x-w/2,floorY+h+.08f,-.26f,trim);
    block(x+w/2,floorY,-.36f,x+w/2+.08f,floorY+h+.08f,-.26f,trim);
    block(x-w/2,floorY+h,-.36f,x+w/2,floorY+h+.08f,-.26f,trim);
    block(x-w/2,floorY,-.34f,x+w/2,floorY+h,-.29f,wood);
    for(int k=1;k<3;++k) block(x-w/2,floorY+h*k/3-.02f,-.29f,x+w/2,floorY+h*k/3+.02f,-.28f,woodGroove);
    block(x+w/2-.16f,floorY+1.0f,-.29f,x+w/2-.10f,floorY+1.1f,-.25f,metal);
    block(x-w/2,floorY+h+.08f,-.31f,x+w/2,floorY+2.95f,-.28f,glassFor(seed+11.0f));
}

// BalconyPanel: pale solid parapet, long brick inset and two thin metal rails.
void balconyPanel(float x0,float x1,float floorY) {
    block(x0,floorY,-.22f,x1,floorY+.80f,0,concrete);
    block(x0+.32f,floorY+.22f,-.06f,x1-.32f,floorY+.60f,.02f,brickLight);
    block(x0-.02f,floorY+.80f,-.25f,x1+.02f,floorY+.86f,.03f,trim);
    for(float y : {floorY+1.0f,floorY+1.12f}) rod({x0,y,-.1f},{x1,y,-.1f},.018f,metal,6);
    int posts=std::max(2,static_cast<int>((x1-x0)/1.2f)+1);
    for(int i=0;i<posts;++i) {
        float x=x0+.1f+(x1-x0-.2f)*i/(posts-1);
        rod({x,floorY+.86f,-.1f},{x,floorY+1.14f,-.1f},.02f,metal,6);
    }
}

// Projecting rectangular concrete frame in front of an upper corridor.
void openFrame(float x0,float x1,float y0,float y1,float front) {
    const float t=.42f;
    block(x0,y0,front-t,x0+t,y1,front,trim); block(x1-t,y0,front-t,x1,y1,front,trim);
    block(x0,y1-.5f,-.3f,x0+t,y1,front,trim); block(x1-t,y1-.5f,-.3f,x1,y1,front,trim);
    block(x0,y1-.5f,front-t,x1,y1,front,trim);
    block(x0,y0,front-t,x1,y0+.32f,front,trim);
    block(x0,y0,.3f,x0+t,y0+.32f,front,trim); block(x1-t,y0,.3f,x1,y0+.32f,front,trim);
}

// Windowed wing: structural bays with large piers, several windows per bay,
// floor beams, a deep projecting roof slab and a raised walkway.
void windowedWing(const WindowWing& s,float seed) {
    const float length=s.bays*s.bay,top=PLINTH+s.levels*s.storey;
    block(0,s.skipGround ? PLINTH+s.storey : PLINTH,-.75f,length,top,-.34f,concrete);
    if(s.walkway) {
        block(-.3f,0,-.75f,length+.3f,PLINTH,s.overhang-.5f,walkway);
        block(-.3f,0,s.overhang-.5f,length+.3f,PLINTH+.004f,s.overhang-.35f,darkTile);
        for(int b : {1,s.bays-2}) if(b>=0&&b<s.bays)
            stair(b*s.bay+s.bay/2,s.overhang-.35f+.9f,2.4f,3,.15f,.30f,0,trim);
    }
    const float w=(s.bay-.6f-(s.perBay-1)*.22f)/s.perBay;
    for(int l=0;l<s.levels;++l) {
        float f=PLINTH+l*s.storey,bandTop=l==s.levels-1 ? top : f+s.storey;
        block(0,f+3.0f,-.42f,length,bandTop,.12f,concrete);
        for(int b=0;b<s.bays;++b) for(int k=0;k<s.perBay;++k) {
            float cx=b*s.bay+.3f+w/2+k*(w+.22f);
            if(s.skipGround&&l==0) continue;
            if(k>0) block(cx-w/2-.22f,f,-.42f,cx-w/2,f+3.0f,.04f,concrete);
            float id=seed+l*37.0f+b*5.0f+k;
            windowBay(cx,f,w,id);
            if(s.acUnits&&l==s.levels-1&&hash1(id*.7f)<.5f)
                acUnit(cx+(hash1(id+.5f)<.5f ? -.25f : .25f),f+.12f,-.10f);
        }
    }
    for(int b=0;b<=s.bays;++b) block(b*s.bay-.3f,0,-.45f,b*s.bay+.3f,top,.38f,concrete);
    block(-.4f,top,-1.0f,length+.4f,top+SLAB,s.overhang,trim);
    block(-.4f,top-.18f,s.overhang-.28f,length+.4f,top,s.overhang,trim);
    for(int b=0;b<s.bays;++b) cylinder(b*s.bay+s.bay/2,top-.07f,s.overhang*.55f,.08f,.14f,.08f,lampBlack,10);
    if(s.freeColumns)
        for(int b=0;b<=s.bays;++b) box(b*s.bay,(PLINTH+top)/2,s.overhang-.9f,.56f,top-PLINTH,.56f,trim);
}

// Gallery wing: open ground corridor and upper balcony corridors in front of
// the room wall (courtyard frontage of the stage wing and the taller block).
void galleryWing(const Gallery& s,float seed) {
    const float length=s.bays*s.bay,top=PLINTH+s.levels*s.storey;
    block(-.3f,0,-s.depth-.1f,length+.3f,PLINTH,.45f,walkway);
    block(-.3f,0,.30f,length+.3f,PLINTH+.004f,.45f,darkTile);
    if(s.tileAccents)
        for(int i=0;i<static_cast<int>(length/2.2f);++i) {
            float x=hash1(seed+i*1.7f)*(length-.6f),z=-hash1(seed+i*2.9f)*(s.depth-.6f)-.6f;
            block(x,PLINTH,z,x+.58f,PLINTH+.004f,z+.58f,darkTile);
        }
    {   Place back(translation({0,0,-s.depth}));
        for(int l=0;l<s.levels;++l) for(int b=0;b<s.bays;++b) {
            float f=PLINTH+l*s.storey,bx=b*s.bay;
            if((s.voids>>b&1u)||(l==0&&(s.passages>>b&1u))) continue;
            block(bx,f,-.75f,bx+s.bay,f+s.storey,-.34f,concrete);
            block(bx,f+3.0f,-.42f,bx+s.bay,f+s.storey,0,concrete);
            block(bx-.18f,f,-.42f,bx+.18f,f+3.0f,.02f,concrete);
            door(bx+.95f,f,seed+b+l*9.0f);
            float w=std::min(2.2f,s.bay-2.3f);
            block(bx+1.52f,f,-.42f,bx+1.75f,f+3.0f,.02f,concrete);
            windowBay(bx+1.75f+(s.bay-1.75f)/2,f,w,seed+b*3.0f+l*13.0f);
        }
    }
    for(int l=1;l<s.levels;++l) {
        float f=PLINTH+l*s.storey;
        block(-.3f,f-.32f,-s.depth,length+.3f,f,.30f,concrete);
        Place p(translation({0,0,.30f}));
        for(int b=0;b<s.bays;++b)
            if(!(l==1&&(s.terrace>>b&1u))) balconyPanel(b*s.bay+.3f,(b+1)*s.bay-.3f,f);
    }
    for(int l=0;l<s.levels;++l) {
        float ceiling=l==s.levels-1 ? top : PLINTH+(l+1)*s.storey-.32f;
        for(int b=0;b<=s.bays;++b) block(b*s.bay-.14f,ceiling-.42f,-s.depth,b*s.bay+.14f,ceiling,.2f,concrete);
        for(int b=0;b<s.bays;++b) {
            float cx=b*s.bay+s.bay/2;
            cylinder(cx,ceiling-.1f,-s.depth*.45f,.09f,.18f,.09f,lampBlack,12);
            block(cx-.6f,ceiling-.05f,-s.depth+.35f,cx+.6f,ceiling,-s.depth+.43f,lampWhite);
        }
    }
    for(int b=0;b<=s.bays;++b) {
        if(s.hallColumns>>b&1u) {   // column interrupted through the first floor
            block(b*s.bay-.28f,PLINTH,-.28f,b*s.bay+.28f,PLINTH+s.storey,.28f,concrete);
            if(s.levels>2) block(b*s.bay-.28f,PLINTH+2*s.storey-.32f,-.28f,b*s.bay+.28f,top,.28f,concrete);
        } else block(b*s.bay-.28f,PLINTH,-.28f,b*s.bay+.28f,top,.28f,concrete);
    }
    block(-.4f,top,-s.depth-1.0f,length+.4f,top+SLAB,1.0f,trim);
    for(int b=0;b<s.bays;++b)
        if(s.frames>>b&1u) openFrame(b*s.bay+.15f,(b+1)*s.bay-.15f,s.frameBase,top+.9f,1.7f);
}

}} // namespace swc::app

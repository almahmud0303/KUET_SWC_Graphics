#include "scene/site.h"

#include "geometry/mesh.h"
#include "scene/components.h"
#include "scene/layout.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Site: lawn, plaza with alpona artwork, roads, paths and drains
// ---------------------------------------------------------------------------
void petalRing(float cx,float cz,float y,float r0,float r1,int count,float halfAngle,const Material& a,const Material& b,float phase) {
    for(int k=0;k<count;++k) {
        float c=phase+2*PI*k/count; const Material& m=k%2 ? b : a;
        const int n=6;
        for(int i=0;i<n;++i) {
            float t0=static_cast<float>(i)/n,t1=static_cast<float>(i+1)/n;
            float w0=halfAngle*std::sin(PI*(t0*.9f+.05f)),w1=halfAngle*std::sin(PI*(t1*.9f+.05f));
            float ra=r0+(r1-r0)*t0,rb=r0+(r1-r0)*t1;
            Vec3 p0{cx+ra*std::cos(c-w0),y,cz+ra*std::sin(c-w0)},p1{cx+ra*std::cos(c+w0),y,cz+ra*std::sin(c+w0)};
            Vec3 p2{cx+rb*std::cos(c+w1),y,cz+rb*std::sin(c+w1)},p3{cx+rb*std::cos(c-w1),y,cz+rb*std::sin(c-w1)};
            addQuad(p0,p1,p2,p3,{0,1,0},m);
        }
    }
}
// Circular multicoloured alpona in front of the entrance (photos 23, 26, 30).
void alpona(float cx,float cz) {
    const float y=.028f;
    const Material white{{.93f,.92f,.86f}},green{{.30f,.60f,.30f}},yellow{{.93f,.80f,.22f}},blue{{.20f,.42f,.78f}},
                   red{{.80f,.22f,.18f}},pink{{.90f,.50f,.55f}},orange{{.92f,.52f,.18f}},cream{{.90f,.86f,.66f}};
    annulus(cx,y,cz,5.75f,6.05f,cream,96);
    for(int k=0;k<40;++k) { float a=2*PI*k/40; annulus(cx+6.35f*std::cos(a),y,cz+6.35f*std::sin(a),.12f,.20f,white,12); }
    annulus(cx,y,cz,4.9f,5.75f,green,96);
    petalRing(cx,cz,y+.004f,4.95f,5.7f,24,.10f,yellow,blue);
    annulus(cx,y,cz,4.45f,4.9f,red,96);
    annulus(cx,y,cz,3.5f,4.45f,yellow,96);
    petalRing(cx,cz,y+.004f,3.55f,4.4f,20,.13f,orange,red,.08f);
    annulus(cx,y,cz,3.1f,3.5f,blue,96);
    annulus(cx,y,cz,2.1f,3.1f,cream,80);
    petalRing(cx,cz,y+.004f,2.15f,3.05f,16,.17f,red,yellow);
    annulus(cx,y,cz,1.25f,2.1f,green,64);
    petalRing(cx,cz,y+.004f,1.3f,2.05f,12,.22f,white,yellow,.2f);
    annulus(cx,y,cz,0,1.25f,pink,48);
    petalRing(cx,cz,y+.004f,.15f,1.2f,8,.35f,red,orange);
}
void site() {
    addQuad({-130,0,95},{130,0,95},{130,0,-140},{-130,0,-140},{0,1,0},grass);
    // Front plaza: terracotta pavers with small tan square groups.
    addQuad({-8,.02f,27},{9.5f,.02f,27},{9.5f,.02f,3.2f},{-8,.02f,3.2f},{0,1,0},paving);
    addQuad({-28,.02f,5},{-8,.02f,5},{-8,.02f,3.2f},{-28,.02f,3.2f},{0,1,0},paving);
    for(int g=0;g<80;++g) {
        float gx=-7.5f+hash1(g*3.1f)*16.5f,gz=3.6f+hash1(g*5.7f)*23.0f;
        float dx=gx-1.2f,dz=gz-12.5f; if(dx*dx+dz*dz<6.9f*6.9f) continue;
        int type=static_cast<int>(hash1(g*7.3f)*3);
        for(int i=0;i<4;++i) {
            float ox=type==0 ? (i%2)*.3f : (type==1 ? i*.3f : (i<3 ? i*.3f : 0.0f));
            float oz=type==0 ? (i/2)*.3f : (type==1 ? 0.0f : (i<3 ? 0.0f : .3f));
            addQuad({gx+ox,.03f,gz+oz+.28f},{gx+ox+.28f,.03f,gz+oz+.28f},{gx+ox+.28f,.03f,gz+oz},{gx+ox,.03f,gz+oz},{0,1,0},tanPaver);
        }
    }
    alpona(1.2f,12.5f);
    // Low brick bed edge between plaza and the curved-facade lawn.
    block(9.5f,0,3.2f,9.75f,.28f,15.0f,brick);
    // Campus roads (front E-W and east N-S), concrete with hedges and drains.
    addQuad({-80,.015f,33},{47,.015f,33},{47,.015f,27},{-80,.015f,27},{0,1,0},road);
    addQuad({41,.016f,33},{47,.016f,33},{47,.016f,-95},{41,.016f,-95},{0,1,0},road);
    for(float z=-94.0f;z<26.0f;z+=.35f) {
        if(z>CAFE_WORLD_Z-15.0f&&z<CAFE_WORLD_Z+.6f) continue;
        if(z>-24.3f&&z<1.0f) continue;
        if(z>14.4f&&z<17.1f) continue;
        addQuad({40.25f,.02f,z+.25f},{40.75f,.02f,z+.25f},{40.75f,.02f,z},{40.25f,.02f,z},{0,1,0},metalDark);
    }
    // Red paver paths: plaza to road, and the cafeteria forecourt.
    addQuad({9.5f,.022f,17.0f},{41,.022f,17.0f},{41,.022f,14.5f},{9.5f,.022f,14.5f},{0,1,0},paving);
    { Place aligned(translation({TALL_BLOCK_OFFSET.x+TALL_EXTRA_WIDTH,0,TALL_BLOCK_OFFSET.z-CAFETERIA_SETBACK}));
    addQuad({29,.022f,-19.5f},{41,.022f,-19.5f},{41,.022f,-33.5f},{29,.022f,-33.5f},{0,1,0},paving);
    for(int g=0;g<30;++g) {
        float gx=30+hash1(g*9.1f)*10,gz=-33+hash1(g*4.3f)*13;
        addQuad({gx,.03f,gz+.3f},{gx+.6f,.03f,gz+.3f},{gx+.6f,.03f,gz},{gx,.03f,gz},{0,1,0},tanPaver);
    }
    }
    // Reconnect the moved cafeteria forecourt to the existing campus road.
    addQuad({41+TALL_BLOCK_OFFSET.x+TALL_EXTRA_WIDTH,.022f,-23.5f-CAFETERIA_SETBACK},{41,.022f,-23.5f-CAFETERIA_SETBACK},
            {41,.022f,-25.0f-CAFETERIA_SETBACK},{41+TALL_BLOCK_OFFSET.x+TALL_EXTRA_WIDTH,.022f,-25.0f-CAFETERIA_SETBACK},{0,1,0},paving);
    addQuad({-28,.021f,-58},{-24,.021f,-58},{-24,.021f,-62},{-28,.021f,-62},{0,1,0},paving);
    // Close roadside path and rear forecourt seen beside the new shop wing.
    addQuad({36,.025f,1},{41,.025f,1},{41,.025f,-24.3f},{36,.025f,-24.3f},{0,1,0},road);
    // Straight paved approach through the hedge opening to the stall-front corridor.
    block(30.6f,0,ENTRY_BACK_Z-.25f,36.0f,.035f,ENTRY_FRONT_Z+.25f,paving);
    addQuad({29,.025f,CAFE_WORLD_Z+.5f},{41,.025f,CAFE_WORLD_Z+.5f},
            {41,.025f,CAFE_WORLD_Z-15},{29,.025f,CAFE_WORLD_Z-15},{0,1,0},paving);
}

}} // namespace swc::app

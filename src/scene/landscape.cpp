#include "scene/landscape.h"

#include "geometry/mesh.h"
#include "scene/components.h"
#include "scene/layout.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Landscape components
// ---------------------------------------------------------------------------
void paintedBand(Vec3 base,float radius) {
    frustum(base,radius+.012f,radius*.96f+.012f,1.0f,whitePaint,12);
    frustum(base+Vec3{0,1.0f,0},radius*.96f+.014f,radius*.95f+.014f,.15f,redPaint,12);
}
void broadleafTree(float x,float z,float height,float spread,float seed,bool painted) {
    const float trunk=height*.5f,r=.10f+.018f*height;
    frustum({x,0,z},r,r*.7f,trunk+.3f,bark,12);
    if(painted) paintedBand({x,0,z},r);
    int branches=3+static_cast<int>(hash1(seed)*3);
    for(int k=0;k<branches;++k) {
        float a=2*PI*(k+hash1(seed+k))/branches;
        Vec3 tip{x+std::cos(a)*spread*.5f,trunk+spread*(.35f+.3f*hash1(seed+k*3.0f)),z+std::sin(a)*spread*.5f};
        rod({x,trunk*.85f,z},tip,r*.42f,bark,6);
    }
    int blobs=8+static_cast<int>(hash1(seed+1)*5);
    for(int k=0;k<blobs;++k) {
        float a=hash1(seed+k*4.1f)*2*PI,d=hash1(seed+k*5.3f)*spread*.55f;
        float y=trunk+spread*(.45f+.45f*hash1(seed+k*6.7f));
        float rad=spread*(.28f+.18f*hash1(seed+k*7.9f));
        ellipsoid({x+std::cos(a)*d,y,z+std::sin(a)*d},{rad,rad*.72f,rad},leafMaterial(seed+k),6,11);
    }
}
// Palm frond: arched rachis with narrow drooping leaflet triangles.
void frond(Vec3 start,float azimuth,float len,float lift,float leaflet,const Material& m) {
    Vec3 h{std::cos(azimuth),0,std::sin(azimuth)},side{-std::sin(azimuth),0,std::cos(azimuth)};
    auto at=[&](float t) { return start+h*(len*t)+Vec3{0,len*(lift*t-(lift+.55f)*t*t),0}; };
    const int n=10;
    for(int i=0;i<n;++i) {
        float t0=static_cast<float>(i)/n,t1=static_cast<float>(i+1)/n;
        Vec3 p0=at(t0),p1=at(t1),mid=lerp(p0,p1,.5f);
        float w=leaflet*(.35f+.65f*std::sin(PI*(t0*.8f+.15f)));
        Vec3 along=(p1-p0)*1.3f,droop{0,-w*.55f,0};
        for(float sgn : {-1.0f,1.0f}) {
            addTriangle(p0,mid,p0+side*(w*sgn)+along+droop,m);
            addTriangle(mid,p1,mid+side*(w*sgn)+along+droop,m);
        }
        rod(p0,p1,.025f*(1.2f-t0),m,4);
    }
}
void coconutPalm(float x,float z,float height,float leanDeg,float leanDir,float seed,bool painted) {
    Vec3 base{x,0,z},dir{std::cos(leanDir),0,std::sin(leanDir)};
    float lean=std::tan(radians(leanDeg))*height;
    auto trunkAt=[&](float t) { return base+Vec3{0,height*t,0}+dir*(lean*t*t); };
    const int segs=10;
    for(int i=0;i<segs;++i) {
        float t=static_cast<float>(i)/segs;
        rod(trunkAt(t),trunkAt(t+1.0f/segs),.19f-.07f*t,palmBark,9);
    }
    if(painted) {
        rod(trunkAt(0),trunkAt(1.0f/height),.205f,whitePaint,10);
        rod(trunkAt(1.0f/height),trunkAt(1.15f/height),.207f,redPaint,10);
    }
    Vec3 top=trunkAt(1);
    sphere(top.x,top.y-.1f,top.z,.28f,{{.30f,.32f,.12f}},6,10);
    for(int k=0;k<5;++k) {
        float a=2*PI*k/5;
        sphere(top.x+std::cos(a)*.25f,top.y-.35f,top.z+std::sin(a)*.25f,.13f,{{.42f,.40f,.12f},.2f},5,8);
    }
    const int fronds=15;
    for(int k=0;k<fronds;++k) {
        float az=2*PI*(k+hash1(seed+k)*.6f)/fronds;
        float lift=k%3==0 ? .75f : (k%3==1 ? .35f : .05f);
        frond(top,az,3.0f+hash1(seed+k*2.0f)*1.2f,lift,.75f,leafMaterial(seed+k*1.3f,.1f));
    }
}
// Fan-leaved palm: petioles carrying pleated circular fans.
void fanPalm(float x,float z,float height,float seed) {
    frustum({x,0,z},.24f,.20f,height,palmBark,12);
    paintedBand({x,0,z},.24f);
    Vec3 top{x,height,z};
    const int leaves=14;
    for(int k=0;k<leaves;++k) {
        float az=2*PI*(k+hash1(seed+k))/leaves,el=radians(-10.0f+60.0f*hash1(seed+k*3.0f));
        Vec3 h{std::cos(az)*std::cos(el),std::sin(el),std::sin(az)*std::cos(el)};
        Vec3 c=top+h*1.4f;
        rod(top,c,.03f,palmBark,4);
        Vec3 side=normalize(cross(h,{0,1,0})),up=normalize(cross(side,h));
        Material leaf=leafMaterial(seed+k*.9f,.05f);
        const int ribs=12; const float radius=1.25f;
        for(int i=0;i<ribs;++i) {
            float a0=-1.2f+2.4f*i/ribs,a1=-1.2f+2.4f*(i+1)/ribs;
            Vec3 d0=h*std::cos(a0)+side*std::sin(a0),d1=h*std::cos(a1)+side*std::sin(a1);
            Vec3 pleat=up*(i%2 ? .08f : -.08f);
            addTriangle(c,c+d0*radius+pleat-up*.25f,c+d1*radius-pleat-up*.25f,leaf);
        }
    }
}
void arecaPalm(float x,float z,float seed) {
    for(int s=0;s<4;++s) {
        float a=2*PI*s/4+hash1(seed+s),h=2.0f+hash1(seed+s*3.0f)*1.2f;
        Vec3 b{x+std::cos(a)*.15f,0,z+std::sin(a)*.15f},t=b+Vec3{std::cos(a)*.35f,h,std::sin(a)*.35f};
        rod(b,t,.045f,{{.52f,.55f,.30f}},6);
        for(int k=0;k<6;++k)
            frond(t,2*PI*k/6+a,1.5f,.9f,.35f,leafMaterial(seed+s*5.0f+k,.2f));
    }
}
void araucaria(float x,float z,float height,float seed) {
    frustum({x,0,z},.16f,.06f,height,bark,10);
    paintedBand({x,0,z},.16f);
    const int tiers=9;
    for(int t=0;t<tiers;++t) {
        float y=1.6f+t*(height-1.9f)/tiers,r=1.5f*(1.0f-static_cast<float>(t)/tiers)+.25f;
        for(int k=0;k<5;++k) {
            float a=2*PI*(k+hash1(seed+t*5.0f+k))/5;
            ellipsoid({x+std::cos(a)*r*.55f,y,z+std::sin(a)*r*.55f},{r*.55f,.22f,r*.55f},leafMaterial(seed+t+k,-.2f),5,9);
        }
    }
}
void shrub(float x,float z,float radius,float seed,bool flowering) {
    for(int k=0;k<3;++k) {
        float a=2*PI*k/3+hash1(seed+k);
        ellipsoid({x+std::cos(a)*radius*.35f,radius*.8f,z+std::sin(a)*radius*.35f},{radius*.75f,radius*.7f,radius*.75f},leafMaterial(seed+k,.05f),6,10);
    }
    if(!flowering) return;
    for(int k=0;k<9;++k) {
        float a=hash1(seed+k*1.1f)*2*PI,e=hash1(seed+k*2.3f)*1.2f;
        sphere(x+std::cos(a)*std::cos(e)*radius*1.05f,radius*.8f+std::sin(e)*radius*.9f,z+std::sin(a)*std::cos(e)*radius*1.05f,.10f,{{.86f,.12f,.10f},.2f},4,6);
    }
}
// Croton-like ornamental foliage (red, orange and yellow leaves).
void croton(float x,float z,float seed) {
    rod({x,0,z},{x,.5f,z},.03f,bark,5);
    const Color tones[3]={{.70f,.14f,.07f},{.82f,.40f,.08f},{.78f,.62f,.12f}};
    for(int k=0;k<5;++k) {
        float a=2*PI*k/5+hash1(seed+k);
        ellipsoid({x+std::cos(a)*.22f,.55f+hash1(seed+k*2.0f)*.35f,z+std::sin(a)*.22f},{.30f,.18f,.30f},{tones[(k+static_cast<int>(seed))%3],.2f,NATURAL},5,8);
    }
}
void hedge(Vec3 a,Vec3 b,float height,float width,float seed) {
    float len=length(b-a); int n=std::max(1,static_cast<int>(len/.8f));
    for(int i=0;i<=n;++i) {
        Vec3 p=lerp(a,b,static_cast<float>(i)/n);
        float h=height*(.85f+.3f*hash1(seed+i));
        ellipsoid({p.x,h*.55f,p.z},{width*.6f,h*.55f,width*.6f},leafMaterial(seed+i*.37f,.25f),5,9);
    }
}

// Gazebo: bamboo posts, low woven panels, lattice band and hipped thatch roof.
void gazebo(float x,float z,float yaw) {
    Place p(trs({x,0,z},yaw));
    const float hx=2.3f,hz=1.8f;
    block(-hx-.2f,0,-hz-.2f,hx+.2f,.30f,hz+.2f,brickLight);
    for(float px : {-hx,-hx/3,hx/3,hx}) for(float pz : {-hz,hz}) {
        rod({px-.05f,.3f,pz},{px-.05f,2.45f,pz},.055f,bamboo,6);
        rod({px+.06f,.3f,pz},{px+.06f,2.45f,pz},.05f,bamboo,6);
    }
    for(float px : {-hx,hx}) rod({px,.3f,0},{px,2.45f,0},.055f,bamboo,6);
    auto side=[&](Vec3 a,Vec3 b,bool entrance) {
        Vec3 d=b-a; float len=length(d); Vec3 u=d*(1.0f/len);
        for(int seg=0;seg<3;++seg) {
            if(entrance&&seg==1) continue;
            Vec3 s0=a+u*(len*seg/3),s1=a+u*(len*(seg+1)/3);
            addQuad(s0+Vec3{0,.3f,0},s1+Vec3{0,.3f,0},s1+Vec3{0,.95f,0},s0+Vec3{0,.95f,0},{{.70f,.55f,.32f},.05f,THATCH});
            for(int k=0;k<7;++k) {
                Vec3 q0=lerp(s0,s1,k/7.0f),q1=lerp(s0,s1,(k+1)/7.0f);
                rod(q0+Vec3{0,.95f,0},q1+Vec3{0,1.25f,0},.018f,bamboo,4);
                rod(q1+Vec3{0,.95f,0},q0+Vec3{0,1.25f,0},.018f,bamboo,4);
            }
            rod(s0+Vec3{0,1.27f,0},s1+Vec3{0,1.27f,0},.035f,bamboo,6);
            rod(s0+Vec3{0,.95f,0},s1+Vec3{0,.95f,0},.03f,bamboo,6);
        }
    };
    side({-hx,0,hz},{hx,0,hz},true); side({hx,0,-hz},{-hx,0,-hz},false);
    side({-hx,0,-hz},{-hx,0,hz},false); side({hx,0,hz},{hx,0,-hz},false);
    for(float y : {2.40f,2.30f}) {
        rod({-hx,y,-hz},{hx,y,-hz},.05f,bamboo,6); rod({-hx,y,hz},{hx,y,hz},.05f,bamboo,6);
        rod({-hx,y,-hz},{-hx,y,hz},.05f,bamboo,6); rod({hx,y,-hz},{hx,y,hz},.05f,bamboo,6);
    }
    // Hipped thatch roof in three slightly offset layers for a shaggy edge.
    for(int layer=0;layer<3;++layer) {
        float o=layer*.12f,ey=2.35f+layer*.10f,ex=hx+.85f-o,ez=hz+.85f-o,ry=3.95f,rx=1.05f;
        Material m{mix(thatch.color,{.50f,.37f,.22f},layer*.35f),.02f,THATCH};
        Vec3 a{-ex,ey,ez},b{ex,ey,ez},c{ex,ey,-ez},d{-ex,ey,-ez},r0{-rx,ry,0},r1{rx,ry,0};
        addQuad(a,b,r1,r0,m); addQuad(c,d,r0,r1,m);
        addTriangle(b,c,r1,m); addTriangle(d,a,r0,m);
    }
    block(-hx-.95f,2.25f,hz+.80f,hx+.95f,2.40f,hz+.95f,thatch);
    block(-hx-.95f,2.25f,-hz-.95f,hx+.95f,2.40f,-hz-.80f,thatch);
    block(-hx-.95f,2.25f,-hz-.95f,-hx-.80f,2.40f,hz+.95f,thatch);
    block(hx+.80f,2.25f,-hz-.95f,hx+.95f,2.40f,hz+.95f,thatch);
    rod({-1.1f,3.97f,0},{1.1f,3.97f,0},.08f,{{.45f,.34f,.20f}},6);
}

// ---------------------------------------------------------------------------
// Landscaping (placed after the architecture, per section 15)
// ---------------------------------------------------------------------------
void landscaping() {
    // Front left lawn: palms and broadleaf trees behind shrubs and crotons (23, 24, 27).
    coconutPalm(-23.5f,8.5f,9.5f,6,.4f,1); coconutPalm(-31.0f,5.0f,10.5f,4,2.0f,2);
    coconutPalm(-13.5f,10.0f,8.0f,8,-.6f,3);
    broadleafTree(-19.0f,13.0f,7.5f,4.2f,4); broadleafTree(-30.0f,15.5f,8.5f,5.0f,5);
    broadleafTree(-11.0f,19.0f,5.5f,3.2f,6); broadleafTree(-26.0f,21.0f,6.5f,3.8f,7);
    for(float x=-26.5f;x<-8.5f;x+=2.2f) {
        if(static_cast<int>((x+30)/2.2f)%2) croton(x,4.3f,x); else shrub(x,4.3f,.45f,x);
    }
    // Curved-facade lawn: areca palm, painted coconut palm, ixora (photo 25).
    arecaPalm(13.5f,6.5f,11); coconutPalm(20.5f,9.0f,11.0f,3,1.0f,12);
    block(18.5f,0,7.3f,22.8f,.3f,7.5f,brick); block(22.6f,0,7.3f,22.8f,.3f,11.2f,brick);
    shrub(10.8f,4.6f,.55f,13,true); shrub(24.5f,4.2f,.6f,14,true); shrub(16.5f,3.9f,.45f,15,true);
    croton(12.0f,4.2f,16); croton(22.0f,3.8f,17); broadleafTree(27.5f,10.0f,6.0f,3.5f,18);
    broadleafTree(11.0f,11.0f,3.5f,1.8f,19);
    // East road edge: hedges, painted trees, gazebo and fan palms (5, 6, 9, 10).
    hedge({39.3f,0,-92},{39.3f,0,CAFE_WORLD_Z-15.5f},1.0f,1.1f,20); hedge({39.3f,0,1.5f},{39.3f,0,14.4f},1.0f,1.1f,21);
    hedge({39.3f,0,17.1f},{39.3f,0,25.5f},1.0f,1.1f,22); hedge({48.2f,0,-92},{48.2f,0,26},1.0f,1.1f,23);
    hedge({30.5f,0,CAFE_WORLD_Z+1.4f},{38.0f,0,CAFE_WORLD_Z+1.4f},.9f,1.0f,24);
    for(float z=-85.0f;z<22.0f;z+=9.5f) broadleafTree(49.8f,z,7.0f+hash1(z)*2.0f,4.2f,z);
    gazebo(35.0f,CAFE_WORLD_Z+4.5f,0);
    fanPalm(38.0f,CAFE_WORLD_Z+3.5f,5.5f,30); fanPalm(31.5f,CAFE_WORLD_Z+3.0f,4.0f,31);
    broadleafTree(34.7f,-11.0f,4.6f,1.7f,32); broadleafTree(36.5f,-59.0f,5.0f,2.0f,33);
    coconutPalm(37.5f,-60.0f,12.0f,3,2.5f,34); broadleafTree(34.7f,-8.0f,4.0f,1.6f,35);
    araucaria(25.0f,-2.8f,6.4f,92);
    hedge({35.2f,0,-23.3f},{35.2f,0,ENTRY_BACK_Z-.5f},.65f,.7f,93);
    hedge({35.2f,0,ENTRY_FRONT_Z+.5f},{35.2f,0,-5.0f},.65f,.7f,94);
    for(float z : {-7.0f,-15.0f,-22.0f}) shrub(34.2f,z,.60f,94+z,true);
    shrub(30.8f,CAFE_WORLD_Z+1.5f,.55f,36); shrub(34.2f,-16.0f,.5f,37);
    // West exterior lawn (photos 1-4).
    coconutPalm(-35.0f,-5.0f,11.0f,5,3.0f,40); coconutPalm(-37.0f,-19.0f,10.0f,7,3.3f,41);
    araucaria(-33.0f,-12.0f,9.0f,42); araucaria(-33.5f,-31.0f,8.0f,43); arecaPalm(-32.0f,-24.0f,44);
    for(int k=0;k<7;++k)
        broadleafTree(-38.0f-hash1(k*3.3f)*6.0f,-8.0f-k*7.0f,7.0f+hash1(k*1.9f)*3.0f,4.0f+hash1(k*2.7f)*1.5f,50+k);
    for(int k=0;k<5;++k) broadleafTree(STAGE_WING_LEFT-3.5f,-38.0f-k*4.0f,4.5f,2.1f,60+k);
    shrub(-31.5f,-16.0f,.5f,66,true); shrub(-31.8f,-41.0f,.45f,67);
    // North side trees.
    for(int k=0;k<6;++k) broadleafTree(-20.0f+k*9.0f,-62.0f-hash1(k*5.1f)*5.0f,8.0f,5.0f,70+k);
    // Courtyard: sparse small trees and flowering shrubs near the edges (13-22).
    broadleafTree(STAGE_WING_LEFT-1.8f,-23.0f,7.0f,2.9f,80,false); broadleafTree(STAGE_WING_LEFT-2.5f,-40.0f,5.8f,2.3f,81,false);
    broadleafTree(14.2f+TALL_BLOCK_OFFSET.x,-21.0f,3.2f,1.5f,82,false);
    // Keep the central stage lawn clear, as in the wide reference photograph.
    broadleafTree(-2.0f,-17.0f,3.0f,1.4f,84,false); broadleafTree(-4.0f,-42.0f,3.0f,1.4f,85,false);
    shrub(STAGE_WING_LEFT-.8f,-36.0f,.55f,86,true); shrub(-2.5f,-42.3f,.5f,87,true); shrub(-5.2f,-42.5f,.5f,88,true);
    shrub(15.2f+TALL_BLOCK_OFFSET.x,-30.0f,.5f,89); shrub(-15.2f,-23.0f,.4f,90); shrub(-4.0f,-16.2f,.45f,91,true);
}

}} // namespace swc::app

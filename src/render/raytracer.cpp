#include "render/raytracer.h"

#include "geometry/mesh.h"
#include "input/camera.h"
#include "scene/layout.h"
#include "scene/scene.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// CPU ray tracer (Whitted style) over the same triangles the rasteriser draws:
// a SAH bounding-volume hierarchy, Moller-Trumbore intersection, shadow rays
// to the sun and the ceiling fixtures, Fresnel reflections, thin-glass
// transmission, ambient occlusion and jittered supersampling.
// ---------------------------------------------------------------------------
namespace {

struct Triangle { Vec3 p0,e1,e2; int vertex; };
struct Node { Vec3 lo,hi; int left,first,count; };      // leaf when count > 0
struct Primitive { Vec3 lo,hi,centre; Triangle triangle; };
struct Hit { float t,u,v; int triangle; };

std::vector<Vertex> vertices;
std::vector<Triangle> triangles;
std::vector<Node> nodes;
std::vector<PointLight> lights;
Mat4 cachedModel{}; bool cachedBaseObjects=false,cacheValid=false;

const Vec3 sunColor{1.0f,.96f,.88f},skyColor{.62f,.74f,.90f},groundBounce{.50f,.50f,.44f},fogColor{.74f,.84f,.94f};

inline Vec3 mul(Vec3 a,Vec3 b) { return {a.x*b.x,a.y*b.y,a.z*b.z}; }
inline Vec3 mixv(Vec3 a,Vec3 b,float t) { return a+(b-a)*t; }
inline Vec3 lowest(Vec3 a,Vec3 b) { return {std::min(a.x,b.x),std::min(a.y,b.y),std::min(a.z,b.z)}; }
inline Vec3 highest(Vec3 a,Vec3 b) { return {std::max(a.x,b.x),std::max(a.y,b.y),std::max(a.z,b.z)}; }
inline float axis(Vec3 v,int a) { return a==0 ? v.x : (a==1 ? v.y : v.z); }
inline float smoothstep(float a,float b,float x) { float t=limit((x-a)/(b-a),0,1); return t*t*(3-2*t); }
inline float fract(float x) { return x-std::floor(x); }
inline float halfArea(Vec3 lo,Vec3 hi) { Vec3 d=hi-lo; return d.x*d.y+d.y*d.z+d.z*d.x; }

// The fragment shader's integer hash and value noise, so both renderers
// produce the same brick, paving, lawn and cloud patterns.
float hash2(float px,float py) {
    std::uint32_t qx=static_cast<std::uint32_t>(static_cast<std::int32_t>(px));
    std::uint32_t qy=static_cast<std::uint32_t>(static_cast<std::int32_t>(py));
    std::uint32_t h=qx*1597334677u ^ qy*3812015801u;
    h=(h^(h>>16))*2246822519u; h^=h>>13;
    return static_cast<float>(h&16777215u)/16777215.0f;
}
float noise(float px,float py) {
    float ix=std::floor(px),iy=std::floor(py),fx=px-ix,fy=py-iy;
    fx=fx*fx*(3-2*fx); fy=fy*fy*(3-2*fy);
    float a=hash2(ix,iy),b=hash2(ix+1,iy),c=hash2(ix,iy+1),d=hash2(ix+1,iy+1);
    return (a+(b-a)*fx)+((c+(d-c)*fx)-(a+(b-a)*fx))*fy;
}
float clouds(float px,float py) {
    float n=0,a=.55f;
    for(int i=0;i<5;++i) { n+=a*noise(px,py); px=px*2.03f+3.7f; py=py*2.03f+1.8f; a*=.48f; }
    return n;
}
Vec3 bonded(Vec3 base,float u,float v,float sx,float sy,Vec3 joint,float jointWidth,float variation,float footprint) {
    float row=std::floor(v/sy);
    float cx=u/sx+.5f*std::fmod(std::fabs(row),2.0f),cy=v/sy;
    float fx=fract(cx)*sx,fy=fract(cy)*sy;
    float mortar=(fx<=jointWidth||fy<=jointWidth) ? 1.0f : 0.0f;
    Vec3 unit=base*(1-variation*.5f+variation*hash2(std::floor(cx),std::floor(cy)));
    float detail=1-smoothstep(.35f,.8f,std::max(footprint/sx,footprint/sy));
    return mixv(mixv(base,joint,.12f),mixv(unit,joint,mortar),detail);
}
Vec3 gridded(Vec3 base,float u,float v,float size,Vec3 joint,float jointWidth,float footprint) {
    float fx=fract(u/size)*size,fy=fract(v/size)*size;
    float mortar=(fx<=jointWidth||fy<=jointWidth) ? 1.0f : 0.0f;
    float detail=1-smoothstep(.35f,.8f,footprint/size);
    return mixv(mixv(base,joint,.04f),mixv(base,joint,mortar),detail);
}
Vec3 sky(Vec3 d) {
    float elevation=std::max(d.y,0.0f);
    Vec3 color=mixv({.75f,.85f,.91f},{.23f,.49f,.78f},std::pow(elevation,.32f));
    float u=d.x/(elevation+.22f)*1.7f+8.2f,v=d.z/(elevation+.22f)*1.7f+3.5f;
    float mask=smoothstep(.47f,.67f,clouds(u,v))*smoothstep(.015f,.15f,elevation);
    Vec3 cloud=mixv({.65f,.74f,.79f},{.98f,.98f,.95f},smoothstep(.46f,.70f,clouds(u+.08f,v+.03f)));
    return mixv(color,cloud,mask);
}

// --- Bounding-volume hierarchy: binned surface-area heuristic ---------------
void subdivide(std::vector<Primitive>& prims,int nodeIndex,int depth) {
    int first=nodes[nodeIndex].first,count=nodes[nodeIndex].count;
    Vec3 lo=prims[first].lo,hi=prims[first].hi,clo=prims[first].centre,chi=clo;
    for(int i=first+1;i<first+count;++i) {
        lo=lowest(lo,prims[i].lo); hi=highest(hi,prims[i].hi);
        clo=lowest(clo,prims[i].centre); chi=highest(chi,prims[i].centre);
    }
    nodes[nodeIndex].lo=lo; nodes[nodeIndex].hi=hi;
    if(count<=4||depth>48) return;
    Vec3 extent=chi-clo;
    int a=extent.x>extent.y ? (extent.x>extent.z ? 0 : 2) : (extent.y>extent.z ? 1 : 2);
    float width=axis(extent,a),origin=axis(clo,a);
    if(width<1e-5f) return;
    constexpr int BINS=12;
    int binCount[BINS]={}; Vec3 binLo[BINS],binHi[BINS];
    auto binOf=[&](const Primitive& p) { return std::min(BINS-1,static_cast<int>((axis(p.centre,a)-origin)/width*BINS)); };
    for(int i=first;i<first+count;++i) {
        int b=binOf(prims[i]);
        if(binCount[b]++==0) { binLo[b]=prims[i].lo; binHi[b]=prims[i].hi; }
        else { binLo[b]=lowest(binLo[b],prims[i].lo); binHi[b]=highest(binHi[b],prims[i].hi); }
    }
    // Cost of splitting after bin s: area(left)*count(left)+area(right)*count(right).
    float leftArea[BINS-1],bestCost=1e30f; int leftCount[BINS-1],split=-1;
    { Vec3 l{},h{}; int n=0;
      for(int s=0;s<BINS-1;++s) {
          if(binCount[s]) { if(n==0) { l=binLo[s]; h=binHi[s]; } else { l=lowest(l,binLo[s]); h=highest(h,binHi[s]); } n+=binCount[s]; }
          leftArea[s]=n ? halfArea(l,h) : 0; leftCount[s]=n;
      } }
    { Vec3 l{},h{}; int n=0;
      for(int s=BINS-1;s>0;--s) {
          if(binCount[s]) { if(n==0) { l=binLo[s]; h=binHi[s]; } else { l=lowest(l,binLo[s]); h=highest(h,binHi[s]); } n+=binCount[s]; }
          if(n==0||leftCount[s-1]==0) continue;
          float cost=leftArea[s-1]*leftCount[s-1]+halfArea(l,h)*n;
          if(cost<bestCost) { bestCost=cost; split=s-1; }
      } }
    if(split<0||(bestCost>=halfArea(lo,hi)*count&&count<=12)) return;
    int middle=static_cast<int>(std::partition(prims.begin()+first,prims.begin()+first+count,
        [&](const Primitive& p) { return binOf(p)<=split; })-prims.begin());
    if(middle==first||middle==first+count) return;
    int left=static_cast<int>(nodes.size());
    nodes.push_back({{},{},0,first,middle-first});
    nodes.push_back({{},{},0,middle,first+count-middle});
    nodes[nodeIndex].left=left; nodes[nodeIndex].count=0;
    subdivide(prims,left,depth+1); subdivide(prims,left+1,depth+1);
}

void gather(const RayCamera& camera) {
    if(cacheValid&&cachedBaseObjects==camera.baseObjects&&std::memcmp(cachedModel.m,camera.model.m,sizeof(cachedModel.m))==0) return;
    vertices.clear(); triangles.clear(); nodes.clear(); lights.clear();
    const Mat3 normals=normalMatrix(camera.model);
    auto add=[&](std::size_t first,std::size_t count,const Mat4& transform,const Mat3& normal) {
        for(std::size_t i=first;i<first+count;++i) {
            Vertex v=mesh[i];
            v.position=transformPoint(transform,v.position); v.normal=normalize(transformNormal(normal,v.normal));
            vertices.push_back(v);
        }
    };
    if(camera.baseObjects) add(baseObjectsFirst,baseObjectsCount,camera.model,normals);
    else {
        add(0,opaqueCount,camera.model,normals);
        for(Vec3 hub:fanPositions) {            // one rotor instance per ceiling fan
            Mat4 fan=multiply(camera.model,multiply(translation(hub),rotationY(fanAngle)));
            add(fanFirst,fanCount,fan,normalMatrix(fan));
        }
        for(const auto& pane:roomGlass) add(pane.first,6,camera.model,normals);
        for(PointLight light:sceneLights) { light.position=transformPoint(camera.model,light.position); lights.push_back(light); }
    }
    std::vector<Primitive> prims; prims.reserve(vertices.size()/3);
    for(std::size_t i=0;i+2<vertices.size();i+=3) {
        Vec3 a=vertices[i].position,b=vertices[i+1].position,c=vertices[i+2].position;
        if(length(cross(b-a,c-a))<1e-10f) continue;
        prims.push_back({lowest(a,lowest(b,c)),highest(a,highest(b,c)),(a+b+c)*(1.0f/3),{a,b-a,c-a,static_cast<int>(i)}});
    }
    nodes.reserve(prims.size()); nodes.push_back({{},{},0,0,static_cast<int>(prims.size())});
    if(!prims.empty()) subdivide(prims,0,0);
    triangles.reserve(prims.size());
    for(const auto& p:prims) triangles.push_back(p.triangle);
    cachedModel=camera.model; cachedBaseObjects=camera.baseObjects; cacheValid=true;
}

// Slab test: entry distance of the ray into a box, or a large value on a miss.
inline float enter(const Node& n,Vec3 o,Vec3 inv,float tmax) {
    float t0=(n.lo.x-o.x)*inv.x,t1=(n.hi.x-o.x)*inv.x; if(t0>t1) std::swap(t0,t1);
    float a=(n.lo.y-o.y)*inv.y,b=(n.hi.y-o.y)*inv.y; if(a>b) std::swap(a,b);
    t0=std::max(t0,a); t1=std::min(t1,b);
    a=(n.lo.z-o.z)*inv.z; b=(n.hi.z-o.z)*inv.z; if(a>b) std::swap(a,b);
    t0=std::max(t0,a); t1=std::min(t1,b);
    return (t1>=t0&&t1>0&&t0<tmax) ? t0 : 1e30f;
}
// Closest hit, or any hit for shadow/occlusion rays (which pass through glass).
bool intersect(Vec3 o,Vec3 d,float tmin,float tmax,Hit& hit,bool anyHit) {
    if(triangles.empty()) return false;
    auto safe=[](float v) { return 1.0f/(std::fabs(v)>1e-9f ? v : (v<0 ? -1e-9f : 1e-9f)); };
    Vec3 inv{safe(d.x),safe(d.y),safe(d.z)};
    int stack[64],top=0,current=0; bool found=false;
    if(enter(nodes[0],o,inv,tmax)>=1e30f) return false;
    for(;;) {
        const Node& node=nodes[current];
        if(node.count>0) {
            for(int i=node.first;i<node.first+node.count;++i) {
                const Triangle& tri=triangles[i];
                // Moller-Trumbore: solve o + t d = p0 + u e1 + v e2.
                Vec3 p=cross(d,tri.e2); float det=dot(tri.e1,p);
                if(std::fabs(det)<1e-12f) continue;
                float invDet=1.0f/det; Vec3 s=o-tri.p0;
                float u=dot(s,p)*invDet; if(u<0||u>1) continue;
                Vec3 q=cross(s,tri.e1); float v=dot(d,q)*invDet; if(v<0||u+v>1) continue;
                float t=dot(tri.e2,q)*invDet; if(t<=tmin||t>=tmax) continue;
                if(anyHit) { if(vertices[tri.vertex].pattern==TRANSPARENT_GLASS) continue; return true; }
                tmax=t; hit={t,u,v,i}; found=true;
            }
            if(top==0) break;
            current=stack[--top];
        } else {
            float a=enter(nodes[node.left],o,inv,tmax),b=enter(nodes[node.left+1],o,inv,tmax);
            int nearChild=node.left,farChild=node.left+1;
            if(b<a) { std::swap(a,b); std::swap(nearChild,farChild); }
            if(a>=1e30f) { if(top==0) break; current=stack[--top]; }
            else { current=nearChild; if(b<1e30f&&top<63) stack[top++]=farChild; }
        }
    }
    return found;
}

struct Random {
    std::uint32_t state;
    float next() { state^=state<<13; state^=state>>17; state^=state<<5; return static_cast<float>(state&16777215u)/16777216.0f; }
};
struct TraceSettings { RayTraceOptions options; float pixelAngle; bool gradientSky; };

Vec3 trace(Vec3 origin,Vec3 direction,int depth,float travelled,const TraceSettings& frame,Random& random) {
    Hit hit;
    if(!intersect(origin,direction,1e-4f,1e30f,hit,false))
        return frame.gradientSky ? sky(direction) : Vec3{.55f,.73f,.92f};
    const Triangle& tri=triangles[hit.triangle];
    const Vertex& v0=vertices[tri.vertex]; const Vertex& v1=vertices[tri.vertex+1]; const Vertex& v2=vertices[tri.vertex+2];
    Vec3 position=origin+direction*hit.t,view=direction*-1.0f;
    Vec3 n=normalize(v0.normal*(1-hit.u-hit.v)+v1.normal*hit.u+v2.normal*hit.v);
    Vec3 geometric=normalize(cross(tri.e1,tri.e2));
    if(dot(geometric,view)<0) geometric=geometric*-1.0f;
    if(dot(n,view)<0) n=n*-1.0f;                                   // two-sided leaves, glass and thin panels
    Vec3 base{v0.color.r,v0.color.g,v0.color.b};
    float gloss=v0.gloss; int pattern=static_cast<int>(v0.pattern+.5f);
    float distance=travelled+hit.t;
    if(pattern==static_cast<int>(EMISSIVE)) return base;
    // Size of one pixel on the surface: fades fine patterns as fwidth() does.
    float footprint=distance*frame.pixelAngle/std::max(std::fabs(dot(n,view)),.12f);
    Vec3 tangent=std::fabs(n.y)>.9f ? Vec3{1,0,0} : normalize(cross({0,1,0},n));
    float wallU=dot(position,tangent),wallV=std::fabs(n.y)>.9f ? position.z : position.y;
    if(pattern==1) base=bonded(base,wallU,wallV,.24f,.075f,{.56f,.49f,.39f},.006f,.25f,footprint);
    else if(pattern==3) base=base*(.82f+.36f*noise(position.x*1.7f,position.z*1.7f)*(.7f+.3f*noise(position.x*9,position.z*9)));
    else if(pattern==4) base=bonded(base,position.x,position.z,.22f,.11f,base*.72f,.012f,.35f,footprint);
    else if(pattern==5) base=base*(.72f+.5f*noise((position.x+position.z)*14,position.y*2.5f));
    else if(pattern==9) {
        float detail=1-smoothstep(.25f,1.0f,footprint*25.0f);
        base=base*(.94f+.12f*noise(position.x*.65f,position.z*.65f));
        base=base*(1+detail*(noise(position.x*18,position.z*18)-.5f)*.65f);
    }
    else if(pattern==6) base=gridded(base,position.x,position.z,.6f,base*.70f,.010f,footprint);

    const Vec3 sun=sunDirection(); Vec3 start=position+geometric*.004f; Hit ignored;
    // Ambient term, darkened by short occlusion rays over the hemisphere.
    Vec3 ambient=pattern==8 ? Vec3{.44f,.44f,.42f} : mixv(groundBounce,skyColor,n.y*.5f+.5f)*.62f;
    if(depth==0&&frame.options.occlusionRays>0) {
        Vec3 helper=std::fabs(geometric.y)<.9f ? Vec3{0,1,0} : Vec3{1,0,0};
        Vec3 t=normalize(cross(helper,geometric)),b=cross(geometric,t); int blocked=0;
        for(int i=0;i<frame.options.occlusionRays;++i) {
            // Cosine-weighted direction: r = sqrt(u1), phi = 2 pi u2.
            float r=std::sqrt(random.next()),phi=2*PI*random.next();
            Vec3 w=normalize(t*(r*std::cos(phi))+b*(r*std::sin(phi))+geometric*std::sqrt(std::max(0.0f,1-r*r)));
            if(intersect(start,w,1e-4f,1.4f,ignored,true)) ++blocked;
        }
        ambient=ambient*(1-.70f*blocked/frame.options.occlusionRays);
    }
    // Sun: Phong diffuse and specular, visible only if the shadow ray escapes.
    float shininess=8+88*gloss,ndl=dot(n,sun);
    float lit=ndl>0&&!intersect(start,sun,1e-4f,1e30f,ignored,true) ? 1.0f : 0.0f;
    float diffuse=std::max(ndl,0.0f)*lit;
    Vec3 reflected=n*(2*dot(n,sun))-sun;
    float specular=std::pow(std::max(dot(view,reflected),0.0f),shininess)*gloss*lit;
    // Ceiling fixtures: attenuated point lights with their own shadow rays.
    Vec3 lampDiffuse{0,0,0},lampSpecular{0,0,0};
    for(const PointLight& light:lights) {
        Vec3 toLight=light.position-position; float d=length(toLight);
        if(d>light.radius||toLight.y<-.45f||toLight.y>4.1f) continue;
        Vec3 l=toLight*(1.0f/d); float window=1-std::pow(d/light.radius,4.0f);
        float attenuation=window*window/(1+.22f*d+.20f*d*d),cosine=std::max(dot(n,l),0.0f);
        float visible=cosine>0&&!intersect(start,l,1e-4f,d-.05f,ignored,true) ? 1.0f : 0.0f;
        Vec3 colour{light.color.r,light.color.g,light.color.b};
        lampDiffuse=lampDiffuse+colour*(attenuation*(.25f+.75f*cosine*visible));
        Vec3 r=n*(2*dot(n,l))-l;
        lampSpecular=lampSpecular+colour*(attenuation*visible*std::pow(std::max(dot(view,r),0.0f),shininess));
    }
    Vec3 color=mul(base,ambient+sunColor*(diffuse*.95f)+lampDiffuse)+sunColor*(specular*.6f)+lampSpecular*(gloss*.5f);

    if(depth<frame.options.maxDepth) {
        float cosine=std::max(dot(n,view),0.0f);
        bool glass=pattern==2||pattern==7;
        // Thin-pane transmission: the pane keeps 20% of its own shading.
        if(pattern==7) color=mixv(trace(position-geometric*.004f,direction,depth+1,distance,frame,random),color,.20f);
        // Fresnel reflectance (Schlick): F = F0 + (1-F0)(1-cos)^5.
        float f0=glass ? .06f : .03f+.07f*gloss;
        float fresnel=(glass||gloss>=.3f) ? f0+(1-f0)*std::pow(1-cosine,5.0f) : 0.0f;
        if(fresnel>.02f) {
            Vec3 mirror=direction-n*(2*dot(direction,n));
            if(dot(mirror,geometric)<.02f) mirror=normalize(mirror+geometric*(.03f-dot(mirror,geometric)));
            color=mixv(color,trace(start,mirror,depth+1,distance,frame,random),std::min(fresnel,.85f));
        }
    }
    float fog=1-std::exp(-std::max(distance-45.0f,0.0f)*.006f);
    return mixv(color,fogColor,fog*.65f);
}

struct Job {
    const RayCamera* camera; const TraceSettings* frame; int width,height,grid; unsigned char* pixels;
    Vec3 forward,right,up; float tanHalf,aspect;
    volatile long nextRow;
};
void renderRows(Job& job) {
    for(;;) {
#ifdef _WIN32
        int y=static_cast<int>(InterlockedIncrement(&job.nextRow))-1;
#else
        int y=static_cast<int>(job.nextRow++);
#endif
        if(y>=job.height) return;
        for(int x=0;x<job.width;++x) {
            Random random{static_cast<std::uint32_t>(y*job.width+x)*2654435761u+12345u};
            random.next();
            Vec3 sum{0,0,0};
            for(int sy=0;sy<job.grid;++sy) for(int sx=0;sx<job.grid;++sx) {
                // Stratified jitter inside the pixel.
                float px=x+(sx+(job.grid>1 ? random.next() : .5f))/job.grid;
                float py=y+(sy+(job.grid>1 ? random.next() : .5f))/job.grid;
                float ndcX=2*px/job.width-1,ndcY=1-2*py/job.height;
                Vec3 origin=job.camera->eye,direction;
                if(job.camera->orthoHalf>0) {
                    origin=origin+job.right*(ndcX*job.camera->orthoHalf*job.aspect)+job.up*(ndcY*job.camera->orthoHalf);
                    direction=job.forward;
                } else direction=normalize(job.forward+job.right*(ndcX*job.aspect*job.tanHalf)+job.up*(ndcY*job.tanHalf));
                sum=sum+trace(origin,direction,0,0,*job.frame,random);
            }
            sum=sum*(1.0f/(job.grid*job.grid));
            unsigned char* out=job.pixels+(static_cast<std::size_t>(y)*job.width+x)*3;
            out[0]=static_cast<unsigned char>(limit(sum.x,0,1)*255+.5f);
            out[1]=static_cast<unsigned char>(limit(sum.y,0,1)*255+.5f);
            out[2]=static_cast<unsigned char>(limit(sum.z,0,1)*255+.5f);
        }
    }
}
#ifdef _WIN32
DWORD WINAPI worker(LPVOID data) { renderRows(*static_cast<Job*>(data)); return 0; }
#endif

} // namespace

RayCamera currentRayCamera() {
    float yaw=radians(cameraYaw);
    Vec3 up=cameraPitch>89.5f ? Vec3{-std::sin(yaw),0,-std::cos(yaw)} : Vec3{0,1,0};
    Mat4 model=multiply(translation(modelTranslation),multiply(rotationY(radians(modelRotation)),scaling(modelScale)));
    return {cameraEye(),cameraTarget,up,cameraFov,cameraOrthoRatio*cameraDistance,model,showingBaseObjects};
}

void rayTrace(const RayCamera& camera,int width,int height,const RayTraceOptions& options,
              std::vector<unsigned char>& pixels) {
    gather(camera);
    pixels.assign(static_cast<std::size_t>(width)*height*3,0);
    int grid=1; while((grid+1)*(grid+1)<=options.samples) ++grid;
    float tanHalf=std::tan(radians(camera.fovDegrees)*.5f);
    TraceSettings frame{options,camera.orthoHalf>0 ? 0.0f : 2*tanHalf/height,camera.orthoHalf==0&&!camera.baseObjects};
    Job job{&camera,&frame,width,height,grid,pixels.data(),{},{},{},tanHalf,static_cast<float>(width)/height,0};
    job.forward=normalize(camera.target-camera.eye);
    job.right=normalize(cross(job.forward,camera.up)); job.up=cross(job.right,job.forward);
#ifdef _WIN32
    SYSTEM_INFO info; GetSystemInfo(&info);
    int threads=std::max(1,std::min(32,static_cast<int>(info.dwNumberOfProcessors)));
    HANDLE handles[32];
    for(int i=0;i<threads;++i) handles[i]=CreateThread(nullptr,0,worker,&job,0,nullptr);
    WaitForMultipleObjects(threads,handles,TRUE,INFINITE);
    for(int i=0;i<threads;++i) CloseHandle(handles[i]);
#else
    renderRows(job);
#endif
}

int rayTraceToFile(int preset,const char* path,int width,int height,int samples) {
    buildScene();
    applyPreset(preset>=0&&preset<presetCount ? preset : 0);
    RayTraceOptions options; options.samples=std::max(samples,1);
    std::vector<unsigned char> pixels;
    RayCamera camera=currentRayCamera();
    rayTrace(camera,width,height,options,pixels);
    std::cout<<"Ray traced "<<triangles.size()<<" triangles in "<<nodes.size()<<" BVH nodes, "
             <<lights.size()<<" point lights, "<<options.samples<<" samples per pixel\n";
    FILE* file=std::fopen(path,"wb");
    if(!file) { std::cerr<<"Could not save "<<path<<'\n'; return EXIT_FAILURE; }
    std::fprintf(file,"P6\n%d %d\n255\n",width,height);
    std::fwrite(pixels.data(),1,pixels.size(),file); std::fclose(file);
    std::cout<<"Saved "<<path<<'\n';
    return EXIT_SUCCESS;
}

}} // namespace swc::app

// KUET Student Welfare Center (SWC) - interactive 3D model.
// The layout follows SWC_Reference.txt, project/ and the right-side photos in project_2/.
// Units are approximately metres. +X is to the viewer's right when facing the
// main entrance, +Z points from the building towards the front plaza, +Y is up.

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "gl_loader.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Custom vector and matrix operations (column-major, OpenGL convention)
// ---------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
struct Color { float r, g, b; };
struct Mat4 { float m[16]; };
struct Mat3 { float m[9]; };

Vec3 operator+(Vec3 a,Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
Vec3 operator-(Vec3 a,Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 operator*(Vec3 a,float s) { return {a.x*s,a.y*s,a.z*s}; }
Vec3 cross(Vec3 a,Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
float dot(Vec3 a,Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
float length(Vec3 v) { return std::sqrt(dot(v,v)); }
Vec3 normalize(Vec3 v) { float l=length(v); return l>1e-8f ? v*(1.0f/l) : Vec3{0,1,0}; }
Vec3 lerp(Vec3 a,Vec3 b,float t) { return a+(b-a)*t; }
float radians(float degrees) { return degrees*PI/180.0f; }
float limit(float value,float low,float high) { return std::max(low,std::min(value,high)); }
Color mix(Color a,Color b,float t) { return {a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t}; }
float hash1(float n) { float s=std::sin(n*12.9898f+78.233f)*43758.5453f; return s-std::floor(s); }

Mat4 identity() { Mat4 r{}; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f; return r; }
Mat4 multiply(const Mat4& a,const Mat4& b) {
    Mat4 result{};
    for(int column=0;column<4;++column)
        for(int row=0;row<4;++row)
            for(int k=0;k<4;++k)
                result.m[column*4+row]+=a.m[k*4+row]*b.m[column*4+k];
    return result;
}
Mat4 translation(Vec3 offset) {
    Mat4 r=identity(); r.m[12]=offset.x; r.m[13]=offset.y; r.m[14]=offset.z; return r;
}
Mat4 rotationX(float angle) {
    Mat4 r=identity(); float c=std::cos(angle),s=std::sin(angle);
    r.m[5]=c; r.m[6]=s; r.m[9]=-s; r.m[10]=c; return r;
}
Mat4 rotationY(float angle) {
    Mat4 r=identity(); float c=std::cos(angle),s=std::sin(angle);
    r.m[0]=c; r.m[2]=-s; r.m[8]=s; r.m[10]=c; return r;
}
Mat4 scaling(Vec3 amount) {
    Mat4 r=identity(); r.m[0]=amount.x; r.m[5]=amount.y; r.m[10]=amount.z; return r;
}
Mat4 scaling(float amount) { return scaling(Vec3{amount,amount,amount}); }
// Composite local transform T * R * S: scale first, then yaw about +Y, then translate.
Mat4 trs(Vec3 position,float yawDegrees,Vec3 scale={1,1,1}) {
    return multiply(translation(position),multiply(rotationY(radians(yawDegrees)),scaling(scale)));
}
Mat4 perspective(float fov,float aspect,float nearPlane,float farPlane) {
    Mat4 result{};
    float f=1.0f/std::tan(fov*0.5f);
    result.m[0]=f/aspect; result.m[5]=f;
    result.m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);
    result.m[11]=-1.0f;
    result.m[14]=(2.0f*farPlane*nearPlane)/(nearPlane-farPlane);
    return result;
}
Mat4 orthographic(float left,float right,float bottom,float top,float nearPlane,float farPlane) {
    Mat4 r=identity();
    r.m[0]=2.0f/(right-left); r.m[5]=2.0f/(top-bottom); r.m[10]=-2.0f/(farPlane-nearPlane);
    r.m[12]=-(right+left)/(right-left); r.m[13]=-(top+bottom)/(top-bottom);
    r.m[14]=-(farPlane+nearPlane)/(farPlane-nearPlane);
    return r;
}
Mat4 lookAt(Vec3 eye,Vec3 target,Vec3 up) {
    Vec3 forward=normalize(target-eye);
    Vec3 side=normalize(cross(forward,up));
    Vec3 correctedUp=cross(side,forward);
    Mat4 result{};
    result.m[0]=side.x; result.m[1]=correctedUp.x; result.m[2]=-forward.x;
    result.m[4]=side.y; result.m[5]=correctedUp.y; result.m[6]=-forward.y;
    result.m[8]=side.z; result.m[9]=correctedUp.z; result.m[10]=-forward.z;
    result.m[12]=-dot(side,eye); result.m[13]=-dot(correctedUp,eye);
    result.m[14]=dot(forward,eye); result.m[15]=1.0f;
    return result;
}
Vec3 transformPoint(const Mat4& m,Vec3 p) {
    return {m.m[0]*p.x+m.m[4]*p.y+m.m[8]*p.z+m.m[12],
            m.m[1]*p.x+m.m[5]*p.y+m.m[9]*p.z+m.m[13],
            m.m[2]*p.x+m.m[6]*p.y+m.m[10]*p.z+m.m[14]};
}
// Inverse-transpose of the upper 3x3, so normals stay perpendicular under non-uniform scaling.
Mat3 normalMatrix(const Mat4& m) {
    float a=m.m[0],b=m.m[4],c=m.m[8],d=m.m[1],e=m.m[5],f=m.m[9],g=m.m[2],h=m.m[6],i=m.m[10];
    float c00=e*i-f*h,c01=-(d*i-f*g),c02=d*h-e*g;
    float c10=-(b*i-c*h),c11=a*i-c*g,c12=-(a*h-b*g);
    float c20=b*f-c*e,c21=-(a*f-c*d),c22=a*e-b*d;
    float det=a*c00+b*c01+c*c02; float k=std::fabs(det)>1e-12f ? 1.0f/det : 0.0f;
    Mat3 n{};
    n.m[0]=c00*k; n.m[3]=c01*k; n.m[6]=c02*k;
    n.m[1]=c10*k; n.m[4]=c11*k; n.m[7]=c12*k;
    n.m[2]=c20*k; n.m[5]=c21*k; n.m[8]=c22*k;
    return n;
}
Vec3 transformNormal(const Mat3& n,Vec3 v) {
    return {n.m[0]*v.x+n.m[3]*v.y+n.m[6]*v.z,n.m[1]*v.x+n.m[4]*v.y+n.m[7]*v.z,n.m[2]*v.x+n.m[5]*v.y+n.m[8]*v.z};
}

// ---------------------------------------------------------------------------
// Materials. "pattern" selects a procedural detail in the fragment shader.
// ---------------------------------------------------------------------------
constexpr float PLAIN=0, BRICK=1, GLASS=2, NATURAL=3, PAVING=4, THATCH=5, TILE=6;
struct Material { Color color; float gloss=.08f; float pattern=PLAIN; };
struct Vertex { Vec3 position, normal; Color color; float gloss, pattern; };

// Base colours from section 12 of SWC_Reference.txt.
const Material concrete{{.84f,.87f,.83f}}, trim{{.94f,.95f,.91f}}, roofMembrane{{.60f,.61f,.58f},.04f,NATURAL};
const Material brick{{.50f,.20f,.13f},.05f,BRICK}, brickLight{{.62f,.31f,.22f},.05f,BRICK};
const Material brickDark{{.36f,.15f,.10f},.04f,BRICK}, warmPanel{{.66f,.36f,.22f},.06f,BRICK};
const Material metal{{.72f,.76f,.73f},.55f}, metalDark{{.20f,.22f,.22f},.45f};
const Material wood{{.29f,.12f,.06f},.25f}, woodGroove{{.20f,.08f,.04f},.2f};
const Material paving{{.53f,.27f,.20f},.06f,PAVING}, tanPaver{{.67f,.47f,.31f},.06f};
const Material grass{{.14f,.43f,.13f},.03f,NATURAL}, soil{{.30f,.24f,.17f},.02f,NATURAL};
const Material walkway{{.82f,.80f,.75f},.30f,TILE}, darkTile{{.17f,.17f,.18f},.35f};
const Material road{{.64f,.63f,.60f},.04f,NATURAL}, darkGap{{.05f,.06f,.06f},.05f};
const Material lampBlack{{.05f,.05f,.05f},.6f}, lampWhite{{1.0f,1.0f,.95f},.2f};
const Material bark{{.33f,.25f,.18f},.04f,NATURAL}, palmBark{{.45f,.40f,.33f},.04f,NATURAL};
const Material whitePaint{{.95f,.95f,.92f},.08f}, redPaint{{.75f,.10f,.08f},.1f};
const Material acBody{{.86f,.87f,.84f},.3f}, bamboo{{.72f,.56f,.32f},.15f}, thatch{{.63f,.49f,.30f},.02f,THATCH};
const Material clearGlass{{.60f,.76f,.78f},.9f,GLASS}, signDark{{.22f,.23f,.24f},.5f};

Material glassFor(float seed) {
    float h=hash1(seed);
    if(h<.10f) return {{.025f,.03f,.03f},.05f};                       // open pane: dark interior gap
    Color dark{.10f,.18f,.18f}, pale{.40f,.50f,.53f};
    return {mix(dark,pale,hash1(seed*1.37f+3.1f)*.65f),.9f,GLASS};
}
Material leafMaterial(float seed,float lightness=0.0f) {
    Color deep{.07f,.25f,.07f}, fresh{.25f,.50f,.14f};
    return {mix(deep,fresh,limit(hash1(seed)*.8f+lightness,0,1)),.06f,NATURAL};
}

// ---------------------------------------------------------------------------
// Scene-graph style transform stack and mesh primitives
// ---------------------------------------------------------------------------
struct Frame { Mat4 model; Mat3 normal; };
std::vector<Vertex> mesh;
std::vector<Frame> frames;

// RAII parent transform: children drawn inside the scope inherit parent * local.
class Place {
public:
    explicit Place(const Mat4& local) {
        Mat4 world=multiply(frames.back().model,local);
        frames.push_back({world,normalMatrix(world)});
    }
    ~Place() { frames.pop_back(); }
    Place(const Place&)=delete;
    Place& operator=(const Place&)=delete;
};

void emit(Vec3 p,Vec3 n,const Material& m) {
    const Frame& f=frames.back();
    mesh.push_back({transformPoint(f.model,p),normalize(transformNormal(f.normal,n)),m.color,m.gloss,m.pattern});
}
void addTriangle(Vec3 a,Vec3 b,Vec3 c,Vec3 n,const Material& m) { emit(a,n,m); emit(b,n,m); emit(c,n,m); }
void addTriangle(Vec3 a,Vec3 b,Vec3 c,const Material& m) { addTriangle(a,b,c,normalize(cross(b-a,c-a)),m); }
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n,const Material& m) { addTriangle(a,b,c,n,m); addTriangle(a,c,d,n,m); }
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,const Material& m) { addQuad(a,b,c,d,normalize(cross(b-a,c-a)),m); }

void box(float x,float y,float z,float width,float height,float depth,const Material& m) {
    float x0=x-width/2,x1=x+width/2,y0=y-height/2,y1=y+height/2,z0=z-depth/2,z1=z+depth/2;
    addQuad({x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},{0,0,1},m);
    addQuad({x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{0,0,-1},m);
    addQuad({x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{1,0,0},m);
    addQuad({x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{-1,0,0},m);
    addQuad({x0,y1,z1},{x1,y1,z1},{x1,y1,z0},{x0,y1,z0},{0,1,0},m);
    addQuad({x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},{0,-1,0},m);
}
// Axis-aligned box from two opposite corners - convenient for architecture.
void block(float x0,float y0,float z0,float x1,float y1,float z1,const Material& m) {
    box((x0+x1)/2,(y0+y1)/2,(z0+z1)/2,std::fabs(x1-x0),std::fabs(y1-y0),std::fabs(z1-z0),m);
}
// Vertical tapered cylinder standing on "base" (smooth normals for Phong shading).
void frustum(Vec3 base,float r0,float r1,float height,const Material& m,int segments=14) {
    float slope=(r0-r1)/height;
    for(int i=0;i<segments;++i) {
        float a=2*PI*i/segments,b=2*PI*(i+1)/segments;
        Vec3 da{std::cos(a),0,std::sin(a)},db{std::cos(b),0,std::sin(b)};
        Vec3 p0=base+da*r0,p1=base+db*r0,p2=base+db*r1+Vec3{0,height,0},p3=base+da*r1+Vec3{0,height,0};
        Vec3 na=normalize({da.x,slope,da.z}),nb=normalize({db.x,slope,db.z});
        emit(p0,na,m); emit(p2,nb,m); emit(p1,nb,m);
        emit(p0,na,m); emit(p3,na,m); emit(p2,nb,m);
        addTriangle(base+Vec3{0,height,0},p2,p3,{0,1,0},m);
    }
}
// Centered elliptical cylinder along Y (kept from the original project API).
void cylinder(float x,float y,float z,float rx,float height,float rz,const Material& m,int segments=20) {
    float bottom=y-height/2,top=y+height/2;
    for(int i=0;i<segments;++i) {
        float a=2*PI*i/segments,b=2*PI*(i+1)/segments;
        Vec3 p0{x+rx*std::cos(a),bottom,z+rz*std::sin(a)},p1{x+rx*std::cos(b),bottom,z+rz*std::sin(b)};
        Vec3 p2{x+rx*std::cos(b),top,z+rz*std::sin(b)},p3{x+rx*std::cos(a),top,z+rz*std::sin(a)};
        Vec3 na=normalize({std::cos(a)/rx,0,std::sin(a)/rz}),nb=normalize({std::cos(b)/rx,0,std::sin(b)/rz});
        emit(p1,nb,m); emit(p0,na,m); emit(p3,na,m);
        emit(p1,nb,m); emit(p3,na,m); emit(p2,nb,m);
        addTriangle({x,top,z},p2,p3,{0,1,0},m);
        addTriangle({x,bottom,z},p0,p1,{0,-1,0},m);
    }
}
// Cylinder between two arbitrary points: railings, branches, palm trunks.
void rod(Vec3 a,Vec3 b,float radius,const Material& m,int segments=8) {
    Vec3 axis=normalize(b-a);
    Vec3 helper=std::fabs(axis.y)<.95f ? Vec3{0,1,0} : Vec3{1,0,0};
    Vec3 u=normalize(cross(axis,helper)),v=cross(axis,u);
    for(int i=0;i<segments;++i) {
        float t0=2*PI*i/segments,t1=2*PI*(i+1)/segments;
        Vec3 n0=u*std::cos(t0)+v*std::sin(t0),n1=u*std::cos(t1)+v*std::sin(t1);
        emit(a+n0*radius,n0,m); emit(a+n1*radius,n1,m); emit(b+n1*radius,n1,m);
        emit(a+n0*radius,n0,m); emit(b+n1*radius,n1,m); emit(b+n0*radius,n0,m);
    }
}
void sphere(float x,float y,float z,float radius,const Material& m,int rings=8,int slices=14) {
    for(int r=0;r<rings;++r) {
        float p0=-PI/2+PI*r/rings,p1=-PI/2+PI*(r+1)/rings;
        for(int s=0;s<slices;++s) {
            float a0=2*PI*s/slices,a1=2*PI*(s+1)/slices;
            Vec3 n00{std::cos(p0)*std::cos(a0),std::sin(p0),std::cos(p0)*std::sin(a0)};
            Vec3 n01{std::cos(p0)*std::cos(a1),std::sin(p0),std::cos(p0)*std::sin(a1)};
            Vec3 n11{std::cos(p1)*std::cos(a1),std::sin(p1),std::cos(p1)*std::sin(a1)};
            Vec3 n10{std::cos(p1)*std::cos(a0),std::sin(p1),std::cos(p1)*std::sin(a0)};
            Vec3 c{x,y,z};
            emit(c+n00*radius,n00,m); emit(c+n11*radius,n11,m); emit(c+n01*radius,n01,m);
            emit(c+n00*radius,n00,m); emit(c+n10*radius,n10,m); emit(c+n11*radius,n11,m);
        }
    }
}
// Non-uniformly scaled sphere: exercises the inverse-transpose normal matrix.
void ellipsoid(Vec3 center,Vec3 radii,const Material& m,int rings=7,int slices=12) {
    Place p(multiply(translation(center),scaling(radii)));
    sphere(0,0,0,1,m,rings,slices);
}
// Flat ring (or ring sector) lying in the XZ plane.
void annulus(float x,float y,float z,float inner,float outer,const Material& m,int segments=64,float a0=0,float a1=2*PI) {
    for(int i=0;i<segments;++i) {
        float a=a0+(a1-a0)*i/segments,b=a0+(a1-a0)*(i+1)/segments;
        addQuad({x+inner*std::cos(a),y,z+inner*std::sin(a)},{x+inner*std::cos(b),y,z+inner*std::sin(b)},
                {x+outer*std::cos(b),y,z+outer*std::sin(b)},{x+outer*std::cos(a),y,z+outer*std::sin(a)},{0,1,0},m);
    }
}
// Straight flight rising towards local -Z; the lowest riser stands at z=front.
void stair(float x,float front,float width,int steps,float riser,float tread,float baseY,const Material& m) {
    for(int i=0;i<steps;++i)
        block(x-width/2,baseY,front-(i+1)*tread,x+width/2,baseY+(i+1)*riser,front-i*tread,m);
}

// ---------------------------------------------------------------------------
// Lettering: 5x5 block glyphs, and a stylised Bengali line (continuous matra
// headstroke with hanging strokes) standing in for the Bengali signage.
// ---------------------------------------------------------------------------
const char* glyph(char c) {
    switch(c) {
    case 'A': return "0111010001111111000110001";
    case 'C': return "0111110000100001000001111";
    case 'D': return "1111010001100011000111110";
    case 'E': return "1111110000111101000011111";
    case 'F': return "1111110000111101000010000";
    case 'I': return "1111100100001000010011111";
    case 'K': return "1000110010111001001010001";
    case 'L': return "1000010000100001000011111";
    case 'N': return "1000111001101011001110001";
    case 'R': return "1111010001111101010010010";
    case 'S': return "0111110000011100000111110";
    case 'T': return "1111100100001000010000100";
    case 'U': return "1000110001100011000101110";
    case 'W': return "1000110001101011101110001";
    default: return "0000000000000000000000000";
    }
}
std::vector<std::string> latinGlyphs(const std::string& text) {
    std::vector<std::string> out;
    for(char c : text) out.push_back(glyph(c));
    return out;
}
std::vector<std::string> bengaliGlyphs(const std::vector<int>& wordLengths,float seed) {
    std::vector<std::string> out;
    for(std::size_t w=0;w<wordLengths.size();++w) {
        if(w>0) out.push_back(glyph(' '));
        for(int k=0;k<wordLengths[w];++k) {
            std::string g(25,'0');
            for(int c=0;c<5;++c) g[c]='1';                                  // matra headstroke
            float s=seed+w*17.0f+k*3.1f;
            int stem=hash1(s)<.6f ? 3 : 1;
            for(int r=1;r<5;++r) g[r*5+stem]='1';
            for(int r=1;r<5;++r) for(int c=0;c<5;++c)
                if(c!=stem && hash1(s+r*7.3f+c*1.9f)>.68f) g[r*5+c]='1';
            out.push_back(g);
        }
    }
    return out;
}
// Text on a local facade plane (+Z outward), centred on centerX.
void flatText(const std::vector<std::string>& glyphs,float centerX,float y,float z,float sx,float sy,float depth,const Material& m) {
    float start=centerX-(glyphs.size()*6-1)*sx/2;
    for(std::size_t k=0;k<glyphs.size();++k)
        for(int row=0;row<5;++row) for(int col=0;col<5;++col)
            if(glyphs[k][row*5+col]=='1') {
                float x=start+(k*6+col)*sx,yy=y+(4-row)*sy;
                block(x,yy,z,x+sx*.86f,yy+sy*.86f,z+depth,m);
            }
}

// ---------------------------------------------------------------------------
// Architectural constants (section 11 starting proportions)
// ---------------------------------------------------------------------------
constexpr float PLINTH=.45f;        // raised walkway / ground-floor level above the lawn
constexpr float STOREY=3.45f;       // two-level wings: roof slab underside at 7.35
constexpr float STOREY_TALL=3.6f;   // taller three-level block: roof underside at 11.25
constexpr float SLAB=.45f;          // visible roof slab/fascia depth
constexpr float ENTRANCE_AXIS=1.2f;
// Move the complete taller block forward and align its 3.2 m corridor with
// the centre of the entrance opening. Keep all of its components in one frame.
constexpr Vec3 TALL_BLOCK_OFFSET{ENTRANCE_AXIS-18.9f,0,3.2f};
constexpr float COURTYARD_RIGHT=17.3f+TALL_BLOCK_OFFSET.x;
constexpr float SWC_CURVE_CENTER_X=15.0f,SWC_CURVE_RADIUS=7.5f;
// Extend only the room side: the courtyard corridor remains aligned with the door.
constexpr float TALL_EXTERIOR_X=SWC_CURVE_CENTER_X+SWC_CURVE_RADIUS-TALL_BLOCK_OFFSET.x;
constexpr float TALL_EXTRA_WIDTH=TALL_EXTERIOR_X-29.0f;
constexpr float CAFETERIA_SETBACK=20.0f; // marked rear bays beside the gazebo
constexpr float CAFETERIA_FRONT_Z=-20.0f-CAFETERIA_SETBACK;
constexpr float CAFETERIA_REAR_Z=CAFETERIA_FRONT_Z-13.0f;
constexpr float CAFE_WORLD_Z=CAFETERIA_FRONT_Z+TALL_BLOCK_OFFSET.z;
constexpr float ENTRY_FRONT_Z=-17.2f,ENTRY_BACK_Z=-20.25f;

// ---------------------------------------------------------------------------
// Reusable components (all in a local facade frame: facade plane z=0,
// outward +Z, running along +X, y measured from the ground).
// ---------------------------------------------------------------------------

// WindowBay: brick panel, projecting brick sill, framed glazing with dense
// security bars, a concrete transom band and a short transom light above.
void windowBay(float x,float floorY,float w,float seed,bool transom=true) {
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

void door(float x,float floorY,float seed,float w=1.0f,float h=2.3f) {
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
struct WindowWing {
    int bays; float bay; int levels; float storey; int perBay;
    float overhang; bool freeColumns; bool acUnits; bool skipGround; bool walkway;
};
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
struct Gallery {
    int bays; float bay; int levels; float storey; float depth;
    unsigned frames; unsigned passages; unsigned voids; bool tileAccents; float frameBase;
};
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
        for(int b=0;b<s.bays;++b) balconyPanel(b*s.bay+.3f,(b+1)*s.bay-.3f,f);
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
    for(int b=0;b<=s.bays;++b) block(b*s.bay-.28f,PLINTH,-.28f,b*s.bay+.28f,top,.28f,concrete);
    block(-.4f,top,-s.depth-1.0f,length+.4f,top+SLAB,1.0f,trim);
    for(int b=0;b<s.bays;++b)
        if(s.frames>>b&1u) openFrame(b*s.bay+.15f,(b+1)*s.bay-.15f,s.frameBase,top+.9f,1.7f);
}

// ---------------------------------------------------------------------------
// Landscape components
// ---------------------------------------------------------------------------
void paintedBand(Vec3 base,float radius) {
    frustum(base,radius+.012f,radius*.96f+.012f,1.0f,whitePaint,12);
    frustum(base+Vec3{0,1.0f,0},radius*.96f+.014f,radius*.95f+.014f,.15f,redPaint,12);
}
void broadleafTree(float x,float z,float height,float spread,float seed,bool painted=true) {
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
void coconutPalm(float x,float z,float height,float leanDeg,float leanDir,float seed,bool painted=true) {
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
void shrub(float x,float z,float radius,float seed,bool flowering=false) {
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
// Site: lawn, plaza with alpona artwork, roads, paths and drains
// ---------------------------------------------------------------------------
void petalRing(float cx,float cz,float y,float r0,float r1,int count,float halfAngle,const Material& a,const Material& b,float phase=0) {
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

// ---------------------------------------------------------------------------
// Courtyard wings, stage and building masses
// ---------------------------------------------------------------------------
void courtyard() {
    // Left side stays open to the lawn, per the corrected site layout.
    // North: stage wing with balcony corridors and projecting frames (13, 14, 21).
    { Place p(trs({-19.5f,0,-44.3f},0));
      galleryWing({4,(COURTYARD_RIGHT+19.5f)/4,2,STOREY,3.2f,0x0Fu,0,0,false,PLINTH+STOREY-.32f},31.0f); }
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
      block(21.25f,0,CAFETERIA_FRONT_Z,25.45f+TALL_EXTRA_WIDTH,11.7f,passageBack,concrete);
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
    block(-26.25f,0,-55.25f,COURTYARD_RIGHT,7.8f,-48.25f,concrete);
    // Roof parapet edges and rooftop stair/water tank housings (photos 21, 22).
    block(-8.0f,7.8f,-54.0f,-4.0f,10.2f,-50.0f,concrete);
}

void stage() {
    const float cx=-9.5f;
    block(cx-3.8f,0,-44.1f,cx+3.8f,4.95f,-43.5f,brick);
    block(cx-3.9f,4.95f,-44.15f,cx+3.9f,5.05f,-43.45f,brickDark);
    // Raised platform: circular segment with a low curved brick edge.
    const float chordZ=-43.5f,half=5.5f,sag=3.2f,r=(half*half+sag*sag)/(2*sag),centerZ=chordZ+sag-r;
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
    { Place p(trs({-4.0f,0,-42.3f},90)); stair(0,0,1.8f,3,.18f,.30f,0,trim); }
}

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
    block(x0,ceiling,ENTRY_BACK_Z,x1,ceiling+.28f,ENTRY_FRONT_Z,concrete);
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
    block(9.3f,0,front+.3f,10.6f,.55f,front+1.4f,trim);
    for(int k=0;k<7;++k) {
        float a=2*PI*k/7;
        rod({9.95f,.55f,front+.85f},{9.95f+std::cos(a)*.45f,1.15f,front+.85f+std::sin(a)*.45f},.03f,{{.35f,.55f,.25f}},4);
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
    { Place p(trs({COURTYARD_RIGHT,0,-56},180)); windowedWing({5,(COURTYARD_RIGHT+27.0f)/5,2,STOREY,3,2.2f,false,true,false,true},71.0f); }
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
    for(int k=0;k<5;++k) broadleafTree(-32.5f,-38.0f-k*4.0f,3.0f,1.5f,60+k);
    shrub(-31.5f,-16.0f,.5f,66,true); shrub(-31.8f,-41.0f,.45f,67);
    // North side trees.
    for(int k=0;k<6;++k) broadleafTree(-20.0f+k*9.0f,-62.0f-hash1(k*5.1f)*5.0f,8.0f,5.0f,70+k);
    // Courtyard: sparse small trees and flowering shrubs near the edges (13-22).
    broadleafTree(-14.8f,-18.0f,3.4f,1.6f,80,false); broadleafTree(-14.6f,-38.5f,3.8f,1.8f,81,false);
    broadleafTree(14.2f+TALL_BLOCK_OFFSET.x,-21.0f,3.2f,1.5f,82,false);
    broadleafTree(14.5f+TALL_BLOCK_OFFSET.x,-37.0f,3.6f,1.7f,83,false);
    broadleafTree(-2.0f,-17.0f,3.0f,1.4f,84,false); broadleafTree(-4.0f,-42.0f,3.0f,1.4f,85,false);
    shrub(-15.0f,-28.0f,.55f,86,true); shrub(-2.5f,-42.3f,.5f,87,true); shrub(-5.2f,-42.5f,.5f,88,true);
    shrub(15.2f+TALL_BLOCK_OFFSET.x,-30.0f,.5f,89); shrub(-15.2f,-23.0f,.4f,90); shrub(-4.0f,-16.2f,.45f,91,true);
}

void buildScene() {
    frames.assign(1,{identity(),normalMatrix(identity())});
    mesh.clear(); mesh.reserve(3000000);
    site();
    frontWing();
    curvedBrickFacade();
    courtyard();
    stage();
    exteriorElevations();
    rightSideWing();
    frontGapCorridor();
    photoCorridorEntrance();
    straightEntryCorridor();
    landscaping();
}

// ---------------------------------------------------------------------------
// Shaders: per-fragment Phong lighting (ambient + diffuse + specular) with
// shadow mapping and procedural surface detail.
// ---------------------------------------------------------------------------
const char* sceneVertexSource=R"(
    #version 330 core
    layout(location=0) in vec3 position;
    layout(location=1) in vec3 normal;
    layout(location=2) in vec3 color;
    layout(location=3) in vec2 material;
    uniform mat4 viewProjection;
    uniform mat4 model;
    uniform mat3 normalMatrix;
    uniform mat4 lightSpace;
    out vec3 surfaceNormal;
    out vec3 surfaceColor;
    out vec3 worldPosition;
    out vec2 surfaceMaterial;
    out vec4 lightPosition;
    void main(){
        vec4 world=model*vec4(position,1.0);
        gl_Position=viewProjection*world;
        surfaceNormal=normalMatrix*normal;
        surfaceColor=color;
        surfaceMaterial=material;
        worldPosition=world.xyz;
        lightPosition=lightSpace*vec4(world.xyz+normalize(surfaceNormal)*0.05,1.0);
    }
)";
const char* sceneFragmentSource=R"(
    #version 330 core
    in vec3 surfaceNormal;
    in vec3 surfaceColor;
    in vec3 worldPosition;
    in vec2 surfaceMaterial;
    in vec4 lightPosition;
    uniform vec3 cameraPosition;
    uniform vec3 sunDirection;
    uniform sampler2D shadowMap;
    uniform int shadowsEnabled;
    uniform int highlightRolloff;
    out vec4 pixel;

    const vec3 sunColor=vec3(1.0,0.96,0.88);
    const vec3 skyColor=vec3(0.62,0.74,0.90);
    const vec3 groundBounce=vec3(0.36,0.40,0.30);
    const vec3 fogColor=vec3(0.74,0.84,0.94);

    float hash(vec2 p){ return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453); }
    float noise(vec2 p){
        vec2 i=floor(p),f=fract(p); f=f*f*(3.0-2.0*f);
        return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
    }
    // Running-bond units with mortar joints, faded out where they would alias.
    vec3 bonded(vec3 base,vec2 uv,vec2 size,vec3 joint,float jointWidth,float variation){
        float row=floor(uv.y/size.y);
        vec2 cell=vec2(uv.x/size.x+0.5*mod(row,2.0),uv.y/size.y);
        vec2 f=fract(cell)*size;
        float mortar=max(step(f.x,jointWidth),step(f.y,jointWidth));
        vec3 unit=base*(1.0-variation*0.5+variation*hash(floor(cell)));
        float detail=1.0-smoothstep(0.35,0.8,max(fwidth(cell.x),fwidth(cell.y)));
        return mix(mix(base,joint,0.12),mix(unit,joint,mortar),detail);
    }
    float shadowFactor(vec3 n){
        if(shadowsEnabled==0) return 1.0;
        vec3 p=lightPosition.xyz/lightPosition.w*0.5+0.5;
        if(p.z>1.0||p.x<0.0||p.x>1.0||p.y<0.0||p.y>1.0) return 1.0;
        float bias=0.0006+0.0012*(1.0-max(dot(n,sunDirection),0.0));
        vec2 texel=1.0/vec2(textureSize(shadowMap,0));
        float lit=0.0;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y)
            lit+=p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r ? 0.0 : 1.0;
        return lit/9.0;
    }
    void main(){
        vec3 n=normalize(surfaceNormal);
        vec3 viewDirection=normalize(cameraPosition-worldPosition);
        if(dot(n,viewDirection)<0.0) n=-n;                    // two-sided leaves, glass and thin panels
        vec3 base=surfaceColor;
        float gloss=surfaceMaterial.x;
        int pattern=int(surfaceMaterial.y+0.5);
        vec3 tangent=abs(n.y)>0.9 ? vec3(1,0,0) : normalize(cross(vec3(0,1,0),n));
        vec2 wallUV=vec2(dot(worldPosition,tangent),abs(n.y)>0.9 ? worldPosition.z : worldPosition.y);
        if(pattern==1) base=bonded(base,wallUV,vec2(0.24,0.075),vec3(0.72,0.66,0.58),0.011,0.35);
        else if(pattern==3) base*=0.82+0.36*noise(worldPosition.xz*1.7)*(0.7+0.3*noise(worldPosition.xz*9.0));
        else if(pattern==4) base=bonded(base,worldPosition.xz,vec2(0.22,0.11),base*0.72,0.012,0.35);
        else if(pattern==5) base*=0.72+0.5*noise(vec2((worldPosition.x+worldPosition.z)*14.0,worldPosition.y*2.5));
        else if(pattern==6) base=bonded(base,worldPosition.xz,vec2(0.6,0.6),base*0.86,0.012,0.08);

        // Phong illumination: ambient + diffuse + specular.
        float shadow=shadowFactor(n);
        vec3 ambient=mix(groundBounce,skyColor,n.y*0.5+0.5)*0.62;
        float diffuse=max(dot(n,sunDirection),0.0)*shadow;
        vec3 reflection=reflect(-sunDirection,n);
        float shininess=mix(8.0,96.0,gloss);
        float specular=pow(max(dot(viewDirection,reflection),0.0),shininess)*gloss*shadow;
        vec3 color=base*(ambient+sunColor*diffuse*0.95)+sunColor*specular*0.6;
        if(pattern==2){
            vec3 r=reflect(-viewDirection,n);
            vec3 env=mix(groundBounce*0.6,skyColor*1.1,clamp(r.y*0.5+0.55,0.0,1.0));
            float fresnel=0.08+0.62*pow(1.0-max(dot(n,viewDirection),0.0),3.0);
            color=mix(color,env,fresnel*0.35);
        }
        // Orthographic elevations/plan: soften overexposed sunlit white roofs instead of clipping.
        if(highlightRolloff==1) color=mix(color,0.78+0.22*(1.0-exp(-(color-0.78)/0.22)),step(0.78,color));
        float distanceToEye=length(cameraPosition-worldPosition);
        float fog=1.0-exp(-max(distanceToEye-45.0,0.0)*0.006);
        pixel=vec4(mix(color,fogColor,fog*0.65),1.0);
    }
)";
const char* depthVertexSource=R"(
    #version 330 core
    layout(location=0) in vec3 position;
    uniform mat4 lightSpace;
    uniform mat4 model;
    void main(){ gl_Position=lightSpace*model*vec4(position,1.0); }
)";
const char* depthFragmentSource=R"(
    #version 330 core
    void main(){}
)";

GLuint compileShader(GLenum type,const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=GL_FALSE; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
        std::cerr<<"Shader compilation failed:\n"<<log<<'\n'; glDeleteShader(shader); return 0; }
    return shader;
}
GLuint makeProgram(const char* vertexSource,const char* fragmentSource) {
    GLuint vertex=compileShader(GL_VERTEX_SHADER,vertexSource),fragment=compileShader(GL_FRAGMENT_SHADER,fragmentSource);
    if(!vertex||!fragment) return 0;
    GLuint result=glCreateProgram(); glAttachShader(result,vertex); glAttachShader(result,fragment); glLinkProgram(result);
    glDeleteShader(vertex); glDeleteShader(fragment); GLint ok=GL_FALSE; glGetProgramiv(result,GL_LINK_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetProgramInfoLog(result,sizeof(log),nullptr,log);
        std::cerr<<"Shader linking failed:\n"<<log<<'\n'; glDeleteProgram(result); return 0; }
    return result;
}

// ---------------------------------------------------------------------------
// Camera: orbit around a target, plus presets matching the reference photos.
// ---------------------------------------------------------------------------
// orthoHalf > 0 selects an orthographic projection of that half-height
// (architectural elevations and plan); 0 keeps the perspective fov.
struct CameraPreset { const char* name; Vec3 eye, target; float fov; float orthoHalf=0; };
// Building footprint (without site and planting): x -27.5..32.9, z -60.3..3.2, top 13.8.
constexpr float PLAN_CENTER_X=2.7f,PLAN_CENTER_Z=-28.5f;
const CameraPreset presets[]={
    {"Main entrance (photo 26)",               {3.5f,1.7f,31.0f},{4.0f,4.3f,-2.0f},55},
    {"Curved SWC facade (photo 25)",           {8.0f,1.6f,17.5f},{15.0f,4.6f,0.0f},60},
    {"Front-left colonnade (photo 23)",        {8.0f,1.7f,22.0f},{-10.0f,3.6f,-1.0f},60},
    {"Front-right wide view (photo 30)",       {-5.0f,1.7f,27.0f},{8.0f,4.0f,0.0f},60},
    {"Cafeteria entrance (photo 12)",          {45.0f+TALL_BLOCK_OFFSET.x+TALL_EXTRA_WIDTH,1.7f,-27.5f+TALL_BLOCK_OFFSET.z-CAFETERIA_SETBACK},{TALL_EXTERIOR_X+TALL_BLOCK_OFFSET.x,5.2f,-27.0f+TALL_BLOCK_OFFSET.z-CAFETERIA_SETBACK},60},
    {"Courtyard stage (photo 21)",             {-8.0f,1.7f,-21.0f},{-9.5f,3.0f,-44.0f},66},
    {"Open left courtyard lawn",              {-3.5f,1.7f,-23.0f},{-19.0f,3.6f,-33.0f},62},
    {"Straight entrance-to-corridor view",     {ENTRANCE_AXIS,2.10f,-4.5f},{ENTRANCE_AXIS,2.10f,-34.0f},66},
    {"Aerial overview",                        {48.0f,58.0f,52.0f},{0.0f,0.0f,-20.0f},50},
    {"Right-side shops (project_2)",           {44.0f,2.1f,8.0f},{27.0f,3.9f,-15.0f},62},
    {"Cafeteria and right-hand stair (project_2)",{42.0f,2.0f,CAFE_WORLD_Z+1.0f},{26.0f,4.0f,CAFE_WORLD_Z-10.0f},64},
    {"Right-hand entrance and straight corridor",{37.0f,1.8f,-19.1f},{5.0f,2.1f,-19.1f},64},
    // Orthographic views; the eye sits just outside the planting so distant site trees are clipped.
    {"Front elevation",                        {PLAN_CENTER_X,10.0f,30.0f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z},50,10.0f},
    {"Back elevation",                         {PLAN_CENTER_X,10.0f,-72.0f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z},50,10.0f},
    {"Left elevation",                         {-40.0f,10.0f,PLAN_CENTER_Z+4.5f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z+4.5f},50,10.0f},
    {"Right elevation",                        {46.0f,10.0f,PLAN_CENTER_Z+4.5f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z+4.5f},50,10.0f},
    {"Top view (plan)",                        {PLAN_CENTER_X,40.0f,PLAN_CENTER_Z+6.0f},{PLAN_CENTER_X,0.0f,PLAN_CENTER_Z+6.0f},50,40.0f},
};
constexpr int presetCount=sizeof(presets)/sizeof(presets[0]);

Vec3 cameraTarget{0,0,0};
float cameraYaw=0,cameraPitch=0,cameraDistance=30,cameraFov=55;
float cameraOrthoRatio=0; // orthographic half-height per unit of distance, so zoom still works
float modelRotation=0.0f,modelScale=1.0f;
Vec3 modelTranslation{0.0f,0.0f,0.0f};
bool rotating=false,panning=false,shadowsOn=true;
double lastMouseX=0.0,lastMouseY=0.0;
int framebufferWidth=1280,framebufferHeight=800;

void applyPreset(int index) {
    const CameraPreset& p=presets[index];
    Vec3 d=p.eye-p.target;
    cameraTarget=p.target; cameraDistance=length(d); cameraFov=p.fov;
    cameraOrthoRatio=p.orthoHalf/cameraDistance;
    cameraPitch=std::asin(d.y/cameraDistance)*180.0f/PI;
    cameraYaw=std::atan2(d.x,d.z)*180.0f/PI;
    std::cout<<"View: "<<p.name<<'\n';
}
Vec3 cameraEye() {
    float yaw=radians(cameraYaw),pitch=radians(cameraPitch);
    return {cameraTarget.x+cameraDistance*std::cos(pitch)*std::sin(yaw),
            cameraTarget.y+cameraDistance*std::sin(pitch),
            cameraTarget.z+cameraDistance*std::cos(pitch)*std::cos(yaw)};
}
void pan(float right,float forward) {
    float yaw=radians(cameraYaw);
    Vec3 f{-std::sin(yaw),0,-std::cos(yaw)},r{std::cos(yaw),0,-std::sin(yaw)};
    cameraTarget=cameraTarget+r*right+f*forward;
}

void resize(GLFWwindow*,int width,int height) {
    framebufferWidth=std::max(width,1); framebufferHeight=std::max(height,1);
}
void mouseButton(GLFWwindow* window,int button,int action,int) {
    if(button==GLFW_MOUSE_BUTTON_LEFT) rotating=action==GLFW_PRESS;
    if(button==GLFW_MOUSE_BUTTON_RIGHT) panning=action==GLFW_PRESS;
    glfwGetCursorPos(window,&lastMouseX,&lastMouseY);
}
void mouseMove(GLFWwindow*,double x,double y) {
    float dx=static_cast<float>(x-lastMouseX),dy=static_cast<float>(y-lastMouseY);
    if(rotating) {
        cameraYaw+=dx*.25f;
        cameraPitch=limit(cameraPitch-dy*.20f,-15.0f,85.0f);
    }
    if(panning) { float k=cameraDistance*.0018f; pan(-dx*k,dy*k); }
    lastMouseX=x; lastMouseY=y;
}
void scroll(GLFWwindow*,double,double offset) {
    cameraDistance=limit(cameraDistance*std::pow(.9f,static_cast<float>(offset)),.5f,160.0f);
}
bool down(GLFWwindow* window,int key) { return glfwGetKey(window,key)==GLFW_PRESS; }
bool keyWasDown[GLFW_KEY_LAST+1]={};
bool pressed(GLFWwindow* window,int key) {
    bool now=down(window,key),result=now&&!keyWasDown[key];
    keyWasDown[key]=now;
    return result;
}
void input(GLFWwindow* window,float delta) {
    if(down(window,GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window,GLFW_TRUE);
    if(down(window,GLFW_KEY_A)||down(window,GLFW_KEY_LEFT)) cameraYaw-=42*delta;
    if(down(window,GLFW_KEY_D)||down(window,GLFW_KEY_RIGHT)) cameraYaw+=42*delta;
    if(down(window,GLFW_KEY_W)||down(window,GLFW_KEY_UP)) cameraPitch=limit(cameraPitch+32*delta,-15,85);
    if(down(window,GLFW_KEY_S)||down(window,GLFW_KEY_DOWN)) cameraPitch=limit(cameraPitch-32*delta,-15,85);
    if(down(window,GLFW_KEY_Q)) cameraDistance=limit(cameraDistance*(1+1.2f*delta),.5f,160);
    if(down(window,GLFW_KEY_E)) cameraDistance=limit(cameraDistance*(1-1.2f*delta),.5f,160);
    float move=(down(window,GLFW_KEY_LEFT_SHIFT) ? 24.0f : 8.0f)*delta;
    if(down(window,GLFW_KEY_T)) pan(0,move);
    if(down(window,GLFW_KEY_G)) pan(0,-move);
    if(down(window,GLFW_KEY_PAGE_UP)) cameraTarget.y+=move;
    if(down(window,GLFW_KEY_PAGE_DOWN)) cameraTarget.y=std::max(0.2f,cameraTarget.y-move);
    if(down(window,GLFW_KEY_J)) modelRotation-=45*delta;
    if(down(window,GLFW_KEY_L)) modelRotation+=45*delta;
    if(down(window,GLFW_KEY_Z)) modelScale=limit(modelScale-0.5f*delta,.5f,1.5f);
    if(down(window,GLFW_KEY_X)) modelScale=limit(modelScale+0.5f*delta,.5f,1.5f);
    if(down(window,GLFW_KEY_U)) modelTranslation.x-=5.0f*delta;
    if(down(window,GLFW_KEY_O)) modelTranslation.x+=5.0f*delta;
    if(down(window,GLFW_KEY_I)) modelTranslation.z-=5.0f*delta;
    if(down(window,GLFW_KEY_K)) modelTranslation.z+=5.0f*delta;
    if(down(window,GLFW_KEY_C)) { modelRotation=0; modelScale=1; modelTranslation={0,0,0}; }
    if(pressed(window,GLFW_KEY_R)) applyPreset(0);
    if(pressed(window,GLFW_KEY_F)) { applyPreset(0); modelRotation=0; modelScale=1; modelTranslation={0,0,0}; }
    for(int i=0;i<9;++i) if(pressed(window,GLFW_KEY_1+i)) applyPreset(i);
    if(pressed(window,GLFW_KEY_0)) applyPreset(9);
    if(pressed(window,GLFW_KEY_V)) applyPreset(10);
    if(pressed(window,GLFW_KEY_H)) applyPreset(11);
    for(int i=0;i<5;++i) if(pressed(window,GLFW_KEY_F1+i)) applyPreset(12+i);
    if(pressed(window,GLFW_KEY_B)) shadowsOn=!shadowsOn;
}

bool writeScreenshot(const char* path,int width,int height) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    FILE* file=std::fopen(path,"wb");
    if(!file) return false;
    std::fprintf(file,"P6\n%d %d\n255\n",width,height);
    for(int y=height-1;y>=0;--y) std::fwrite(&pixels[static_cast<std::size_t>(y)*width*3],1,static_cast<std::size_t>(width)*3,file);
    std::fclose(file);
    return true;
}

} // namespace

int main(int argc,char** argv) {
    // Optional: image_project.exe --capture <view 1-17> <file.ppm> [width height] renders one frame to a file.
    int captureView=-1; const char* capturePath=nullptr;
    int windowWidth=1280,windowHeight=800;
    if((argc==4||argc==6)&&std::strcmp(argv[1],"--capture")==0) { captureView=std::atoi(argv[2])-1; capturePath=argv[3]; }
    if(capturePath&&argc==6) { windowWidth=std::max(std::atoi(argv[4]),64); windowHeight=std::max(std::atoi(argv[5]),64); }

    if(!glfwInit()) { std::cerr<<"Could not initialize GLFW.\n"; return EXIT_FAILURE; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_SAMPLES,4);
    if(capturePath) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window=glfwCreateWindow(windowWidth,windowHeight,"KUET Student Welfare Center - Interactive 3D View",nullptr,nullptr);
    if(!window) { glfwTerminate(); return EXIT_FAILURE; }
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    if(!load_opengl_functions(reinterpret_cast<void*(*)(const char*)>(glfwGetProcAddress))) {
        std::cerr<<"Could not load OpenGL functions.\n"; glfwDestroyWindow(window); glfwTerminate(); return EXIT_FAILURE;
    }
    glfwSetFramebufferSizeCallback(window,resize); glfwSetMouseButtonCallback(window,mouseButton);
    glfwSetCursorPosCallback(window,mouseMove); glfwSetScrollCallback(window,scroll);
    glfwGetFramebufferSize(window,&framebufferWidth,&framebufferHeight);
    glEnable(GL_DEPTH_TEST); glEnable(GL_MULTISAMPLE);

    buildScene();
    std::cout<<"SWC scene: "<<mesh.size()/3<<" triangles\n";
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,gloss)));
    glEnableVertexAttribArray(3);

    GLuint program=makeProgram(sceneVertexSource,sceneFragmentSource);
    GLuint depthProgram=makeProgram(depthVertexSource,depthFragmentSource);
    if(!program||!depthProgram) return EXIT_FAILURE;
    GLint viewProjectionLocation=glGetUniformLocation(program,"viewProjection");
    GLint modelLocation=glGetUniformLocation(program,"model");
    GLint normalLocation=glGetUniformLocation(program,"normalMatrix");
    GLint lightSpaceLocation=glGetUniformLocation(program,"lightSpace");
    GLint cameraLocation=glGetUniformLocation(program,"cameraPosition");
    GLint sunLocation=glGetUniformLocation(program,"sunDirection");
    GLint shadowMapLocation=glGetUniformLocation(program,"shadowMap");
    GLint shadowsLocation=glGetUniformLocation(program,"shadowsEnabled");
    GLint rolloffLocation=glGetUniformLocation(program,"highlightRolloff");
    GLint depthLightLocation=glGetUniformLocation(depthProgram,"lightSpace");
    GLint depthModelLocation=glGetUniformLocation(depthProgram,"model");

    // Shadow map: one consistent sun from the front-right, high in the sky.
    constexpr int SHADOW_SIZE=4096;
    GLuint shadowTexture=0,shadowFramebuffer=0;
    glGenTextures(1,&shadowTexture); glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,SHADOW_SIZE,SHADOW_SIZE,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    const float border[4]={1,1,1,1}; glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
    glGenFramebuffers(1,&shadowFramebuffer); glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture,0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) { std::cerr<<"Shadow map unavailable.\n"; shadowsOn=false; }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    const Vec3 sun=normalize({.42f,1.0f,.62f}),sceneCenter{6,0,-20};
    const Mat4 lightSpace=multiply(orthographic(-95,95,-95,95,1,320),lookAt(sceneCenter+sun*150.0f,sceneCenter,{0,1,0}));

    applyPreset(captureView>=0&&captureView<presetCount ? captureView : 0);
    std::cout<<"Keys 1-9: original views; 0: shops; V: cafeteria; H: connecting corridor;\n"
             "F1-F5: front/back/left/right/top orthographic views; B: shadows.\n";
    double previous=glfwGetTime(); int frame=0;
    while(!glfwWindowShouldClose(window)) {
        double now=glfwGetTime(); float delta=static_cast<float>(std::min(now-previous,.05)); previous=now;
        if(!capturePath) input(window,delta);
        Vec3 eye=cameraEye();
        float aspect=static_cast<float>(framebufferWidth)/framebufferHeight,half=cameraOrthoRatio*cameraDistance;
        Mat4 projection=cameraOrthoRatio>0 ? orthographic(-half*aspect,half*aspect,-half,half,.15f,450.0f)
                                           : perspective(radians(cameraFov),aspect,.15f,450.0f);
        // Looking straight down, "up" on screen is the building's rear (-Z) so the front faces the viewer.
        float yaw=radians(cameraYaw);
        Vec3 up=cameraPitch>89.5f ? Vec3{-std::sin(yaw),0,-std::cos(yaw)} : Vec3{0,1,0};
        Mat4 viewProjection=multiply(projection,lookAt(eye,cameraTarget,up));
        // Composite object transformation: translation * rotation * scaling.
        Mat4 model=multiply(translation(modelTranslation),multiply(rotationY(radians(modelRotation)),scaling(modelScale)));
        Mat3 normals=normalMatrix(model);
        glBindVertexArray(vao);

        if(shadowsOn) {
            glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer); glViewport(0,0,SHADOW_SIZE,SHADOW_SIZE);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(depthProgram);
            glUniformMatrix4fv(depthLightLocation,1,GL_FALSE,lightSpace.m);
            glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,model.m);
            glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(mesh.size()));
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }
        glViewport(0,0,framebufferWidth,framebufferHeight);
        glClearColor(.55f,.73f,.92f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(program);
        glUniformMatrix4fv(viewProjectionLocation,1,GL_FALSE,viewProjection.m);
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.m);
        glUniformMatrix3fv(normalLocation,1,GL_FALSE,normals.m);
        glUniformMatrix4fv(lightSpaceLocation,1,GL_FALSE,lightSpace.m);
        glUniform3f(cameraLocation,eye.x,eye.y,eye.z);
        glUniform3f(sunLocation,sun.x,sun.y,sun.z);
        glUniform1i(shadowMapLocation,0);
        glUniform1i(shadowsLocation,shadowsOn ? 1 : 0);
        glUniform1i(rolloffLocation,cameraOrthoRatio>0 ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D,shadowTexture);
        glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(mesh.size()));

        if(capturePath&&++frame==3) {
            bool ok=writeScreenshot(capturePath,framebufferWidth,framebufferHeight);
            std::cout<<(ok ? "Saved " : "Could not save ")<<capturePath<<'\n';
            glfwSetWindowShouldClose(window,GLFW_TRUE);
        }
        glfwSwapBuffers(window); glfwPollEvents();
    }
    glDeleteFramebuffers(1,&shadowFramebuffer); glDeleteTextures(1,&shadowTexture);
    glDeleteProgram(program); glDeleteProgram(depthProgram); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
    glfwDestroyWindow(window); glfwTerminate(); return EXIT_SUCCESS;
}

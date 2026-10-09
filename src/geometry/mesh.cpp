#include "geometry/mesh.h"



namespace swc { namespace app {

std::vector<Vertex> mesh;
std::vector<Frame> frames;
std::vector<GlassPane> roomGlass;
std::vector<PointLight> sceneLights;
std::vector<Vec3> fanPositions;
std::vector<VehicleMesh> vehicleMeshes;
std::vector<Vehicle> vehicles;
float trafficTime=28.2f;   // captures use this fixed moment
GLsizei opaqueCount=0,fanCount=0;
GLint fanFirst=0,baseObjectsFirst=0;
GLsizei baseObjectsCount=0;
bool showingBaseObjects=false;
float fanAngle=0;
bool fansRunning=true;
void emit(Vec3 p,Vec3 n,const Material& m) {
    const Frame& f=frames.back();
    mesh.push_back({transformPoint(f.model,p),normalize(transformNormal(f.normal,n)),m.color,m.gloss,m.pattern});
}
void addTriangle(Vec3 a,Vec3 b,Vec3 c,Vec3 n,const Material& m) { emit(a,n,m); emit(b,n,m); emit(c,n,m); }
void addTriangle(Vec3 a,Vec3 b,Vec3 c,const Material& m) { addTriangle(a,b,c,normalize(cross(b-a,c-a)),m); }
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n,const Material& m) {
    canonical::quad(a,b,c,d,n,[&](Vec3 p,Vec3 normal) { emit(p,normal,m); });
}
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,const Material& m) { addQuad(a,b,c,d,normalize(cross(b-a,c-a)),m); }

// Canonical geometry adapters. Keep instance arithmetic and triangle order
// stable so the existing architecture, normals and procedural patterns match.
void box(float x,float y,float z,float width,float height,float depth,const Material& m) {
    float x0=x-width/2,x1=x+width/2,y0=y-height/2,y1=y+height/2,z0=z-depth/2,z1=z+depth/2;
    auto point=[&](Vec3 p) { return Vec3{p.x<0 ? x0:x1,p.y<0 ? y0:y1,p.z<0 ? z0:z1}; };
    canonical::cubeFaces([&](Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n) {
        addQuad(point(a),point(b),point(c),point(d),n,m);
    });
}
void block(float x0,float y0,float z0,float x1,float y1,float z1,const Material& m) {
    box((x0+x1)/2,(y0+y1)/2,(z0+z1)/2,std::fabs(x1-x0),std::fabs(y1-y0),std::fabs(z1-z0),m);
}
// A tapered unit-circle sweep. Preserve the original top cap and open bottom.
void frustum(Vec3 base,float r0,float r1,float height,const Material& m,int segments) {
    float slope=(r0-r1)/height;
    canonical::circleSectors(segments,[&](Vec3 da,Vec3 db) {
        Vec3 p0=base+da*r0,p1=base+db*r0,p2=base+db*r1+Vec3{0,height,0},p3=base+da*r1+Vec3{0,height,0};
        Vec3 na=normalize({da.x,slope,da.z}),nb=normalize({db.x,slope,db.z});
        emit(p0,na,m); emit(p2,nb,m); emit(p1,nb,m);
        emit(p0,na,m); emit(p3,na,m); emit(p2,nb,m);
        addTriangle(base+Vec3{0,height,0},p2,p3,{0,1,0},m);
    });
}
// Scale the canonical cylinder's XZ radius and centred Y interval separately.
void cylinder(float x,float y,float z,float rx,float height,float rz,const Material& m,int segments) {
    float bottom=y-height/2,top=y+height/2;
    canonical::cylinder([&](Vec3 p,Vec3 n) {
        Vec3 position{x+rx*p.x,p.y<0 ? bottom:top,z+rz*p.z};
        Vec3 normal=n.y!=0 ? n:normalize(Vec3{n.x/rx,0,n.z/rz});
        emit(position,normal,m);
    },segments);
}
// Orient the unit circular cross-section around the endpoint-defined axis.
void rod(Vec3 a,Vec3 b,float radius,const Material& m,int segments) {
    Vec3 axis=normalize(b-a);
    Vec3 helper=std::fabs(axis.y)<.95f ? Vec3{0,1,0}:Vec3{1,0,0};
    Vec3 u=normalize(cross(axis,helper)),v=cross(axis,u);
    canonical::circleSectors(segments,[&](Vec3 da,Vec3 db) {
        Vec3 n0=u*da.x+v*da.z,n1=u*db.x+v*db.z;
        emit(a+n0*radius,n0,m); emit(a+n1*radius,n1,m); emit(b+n1*radius,n1,m);
        emit(a+n0*radius,n0,m); emit(b+n1*radius,n1,m); emit(b+n0*radius,n0,m);
    });
}
void sphere(float x,float y,float z,float radius,const Material& m,int rings,int slices) {
    Vec3 center{x,y,z};
    canonical::sphere([&](Vec3 p,Vec3 n) { emit(center+p*radius,n,m); },rings,slices);
}
// Non-uniform scale uses the existing inverse-transpose normal transform.
void ellipsoid(Vec3 center,Vec3 radii,const Material& m,int rings,int slices) {
    Place p(multiply(translation(center),scaling(radii)));
    sphere(0,0,0,1,m,rings,slices);
}
void annulus(float x,float y,float z,float inner,float outer,const Material& m,int segments,float a0,float a1) {
    canonical::ringSectors(segments,a0,a1,[&](Vec3 a,Vec3 b) {
        addQuad({x+inner*a.x,y,z+inner*a.z},{x+inner*b.x,y,z+inner*b.z},
                {x+outer*b.x,y,z+outer*b.z},{x+outer*a.x,y,z+outer*a.z},{0,1,0},m);
    });
}
// Straight flight rising towards local -Z; the lowest riser stands at z=front.
void stair(float x,float front,float width,int steps,float riser,float tread,float baseY,const Material& m) {
    for(int i=0;i<steps;++i)
        block(x-width/2,baseY,front-(i+1)*tread,x+width/2,baseY+(i+1)*riser,front-i*tread,m);
}

}} // namespace swc::app

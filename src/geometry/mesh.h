#pragma once

#include <vector>
#include "gl_loader.h"
#include "geometry/canonical_shapes.h"
#include "scene/materials.h"

namespace swc { namespace app {

struct Vertex { Vec3 position, normal; Color color; float gloss, pattern; };
struct Frame { Mat4 model; Mat3 normal; };
struct GlassPane { Vec3 center; float width,height; GLint first=0; };
extern std::vector<Vertex> mesh;
extern std::vector<Frame> frames;
extern std::vector<GlassPane> roomGlass;
extern GLsizei opaqueCount,fanCount,baseObjectsCount;
extern GLint fanFirst,baseObjectsFirst;
extern bool showingBaseObjects,fansRunning;
extern float fanAngle;
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

void emit(Vec3 p,Vec3 n,const Material& m);
void addTriangle(Vec3 a,Vec3 b,Vec3 c,Vec3 n,const Material& m);
void addTriangle(Vec3 a,Vec3 b,Vec3 c,const Material& m);
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 n,const Material& m);
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,const Material& m);
void box(float x,float y,float z,float width,float height,float depth,const Material& m);
void block(float x0,float y0,float z0,float x1,float y1,float z1,const Material& m);
void frustum(Vec3 base,float r0,float r1,float height,const Material& m,int segments=14);
void cylinder(float x,float y,float z,float rx,float height,float rz,const Material& m,int segments=20);
void rod(Vec3 a,Vec3 b,float radius,const Material& m,int segments=8);
void sphere(float x,float y,float z,float radius,const Material& m,int rings=8,int slices=14);
void ellipsoid(Vec3 center,Vec3 radii,const Material& m,int rings=7,int slices=12);
void annulus(float x,float y,float z,float inner,float outer,const Material& m,int segments=64,float a0=0,float a1=2*PI);
void stair(float x,float front,float width,int steps,float riser,float tread,float baseY,const Material& m);

}} // namespace swc::app

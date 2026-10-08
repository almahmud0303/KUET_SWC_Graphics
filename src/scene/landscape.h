#pragma once
#include "geometry/geometry_math.h"
#include "scene/materials.h"

namespace swc { namespace app {

void paintedBand(Vec3 base,float radius);
void broadleafTree(float x,float z,float height,float spread,float seed,bool painted=true);
void frond(Vec3 start,float azimuth,float len,float lift,float leaflet,const Material& m);
void coconutPalm(float x,float z,float height,float leanDeg,float leanDir,float seed,bool painted=true);
void fanPalm(float x,float z,float height,float seed);
void arecaPalm(float x,float z,float seed);
void araucaria(float x,float z,float height,float seed);
void shrub(float x,float z,float radius,float seed,bool flowering=false);
void croton(float x,float z,float seed);
void hedge(Vec3 a,Vec3 b,float height,float width,float seed);
void gazebo(float x,float z,float yaw);
void landscaping();

}} // namespace swc::app

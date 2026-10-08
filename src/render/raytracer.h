#pragma once
#include <vector>
#include "geometry/geometry_math.h"

namespace swc { namespace app {

// Camera for one ray-traced frame. orthoHalf > 0 selects parallel rays.
struct RayCamera {
    Vec3 eye,target,up;
    float fovDegrees,orthoHalf;
    Mat4 model;             // interactive model transform applied to the scene
    bool baseObjects;       // trace the canonical-object gallery instead of the SWC
};
struct RayTraceOptions {
    int samples=4;          // jittered samples per pixel (rounded down to a square)
    int maxDepth=3;         // reflection / transmission bounces
    int occlusionRays=8;    // ambient-occlusion rays per primary sample (0 disables)
};

// Camera matching the state the rasteriser is currently using.
RayCamera currentRayCamera();
// Renders top-down RGB8 rows into pixels (width*height*3 bytes).
void rayTrace(const RayCamera& camera,int width,int height,const RayTraceOptions& options,
              std::vector<unsigned char>& pixels);
// Headless: builds the scene, applies a camera preset and writes a binary PPM.
int rayTraceToFile(int preset,const char* path,int width,int height,int samples);

}} // namespace swc::app

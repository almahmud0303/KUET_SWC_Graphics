#pragma once
#include "geometry/geometry_math.h"

namespace swc::rooms {
constexpr float centers[]={6.1f,11.2f,16.3f};
constexpr float front=-20.25f, back=-27.25f, halfWidth=2.55f;
constexpr float floorY=.45f, ceiling=3.77f, eyeHeight=2.05f;
inline Mat4 fanTransform(float x,float angle) {
    return multiply(translation({x,3.13f,-23.55f}),rotationY(angle));
}
}

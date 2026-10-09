#pragma once
#include <vector>
#include "geometry/mesh.h"

namespace swc { namespace app {

// One moving thing to draw this frame: a mesh range and where it is now.
struct MovingInstance { GLint first; GLsizei count; Mat4 transform; };

// Builds the pedestrian meshes and their walking routes. Call after the
// static scene, with buildVehicleMeshes().
void buildPeopleMeshes();
// Every vehicle and pedestrian at the given time.
void collectMovers(float time,std::vector<MovingInstance>& out);

}} // namespace swc::app

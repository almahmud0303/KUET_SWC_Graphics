#pragma once
#include "geometry/mesh.h"

namespace swc { namespace app {

// road/ photographs: boundary wall, footpath, road, divider and the row of
// stalls behind the SWC, with the town buildings beyond them.
void backStreet();
// Builds the easybike, cycle-van and bus meshes and the list of vehicles.
// Call after the static scene, so they stay out of the static draw range.
void buildVehicleMeshes();
// Where a vehicle is at a given time; false while it is beyond the road ends.
bool vehicleTransform(const Vehicle& vehicle,float time,Mat4& transform);

}} // namespace swc::app

#pragma once
#include "geometry/geometry_math.h"

namespace swc { namespace app {

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

constexpr float STAGE_WING_LEFT=-39.6f, STAGE_BAY=5.6f;
constexpr float STAGE_CENTER=STAGE_WING_LEFT+2.1f*STAGE_BAY;

}} // namespace swc::app

#include "scene/scene.h"

#include "geometry/mesh.h"
#include "navigation/navigation.h"
#include "room_navigation.h"
#include "scene/base_objects.h"
#include "scene/courtyard.h"
#include "scene/exterior.h"
#include "scene/front.h"
#include "scene/interior.h"
#include "scene/landscape.h"
#include "scene/layout.h"
#include "scene/rooms.h"
#include "scene/site.h"
#include "scene/people.h"
#include "scene/street.h"

namespace swc { namespace app {

void buildScene() {
    frames.assign(1,{identity(),normalMatrix(identity())});
    mesh.clear(); mesh.reserve(3000000); roomGlass.clear(); sceneLights.clear(); fanPositions.clear(); navigationReady=false;
    site();
    backStreet();
    frontWing();
    curvedBrickFacade();
    courtyard();
    stage();
    exteriorElevations();
    rightSideWing();
    frontGapCorridor();
    photoCorridorEntrance();
    straightEntryCorridor();
    // Three visible treads connect the two-bay corridor to the courtyard lawn.
    { Place access(trs({TERRACE_EDGE_X,0,-24.0f},-90));
      stair(0,.99f,2.2f,3,.15f,.33f,0,trim); }
    landscaping();
    room(rooms::centers[0],{.66f,.77f,.72f});
    room(rooms::centers[1],{.72f,.79f,.86f});
    room(rooms::centers[2],{.85f,.76f,.62f});
    buildInteriors();
    buildRoomDrawRanges();
    buildVehicleMeshes();
    buildPeopleMeshes();
    buildBaseObjectGallery();
}

}} // namespace swc::app

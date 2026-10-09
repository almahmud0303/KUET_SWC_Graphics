#include "scene/people.h"

#include "scene/layout.h"
#include "scene/street.h"

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Pedestrians. A person is one of a few low-polygon meshes (shirt colour x
// stride pose) moved along a fixed walking route by a time-based transform.
// Routes follow floors the walk check already proves are clear: the plaza,
// the corridors and halls, the upper terrace, the lawn, the campus road, the
// footpath outside the wall and the apron in front of the stalls.
// ---------------------------------------------------------------------------
namespace {

struct Route { std::vector<Vec3> points; bool closed; std::vector<float> lengths; float total; };
struct Person { int route,shirt; float speed,offset; };

std::vector<Route> routes;
std::vector<Person> people;
std::vector<VehicleMesh> personMeshes;     // index = shirt*2 + pose
constexpr int SHIRTS=5;

// Faces +X, feet on y=0. stride = +1 or -1 swaps which leg and arm lead.
void personMesh(Color shirt,float stride) {
    const Material cloth{shirt,.1f},trousers{{.16f,.18f,.26f},.1f},skin{{.55f,.38f,.26f},.1f},hair{{.06f,.05f,.05f},.1f};
    for(float side : {-1.0f,1.0f}) {
        float lead=side*stride;
        rod({0,.84f,side*.10f},{lead*.22f,.06f,side*.10f},.070f,trousers,6);
        box(lead*.22f+.05f,.04f,side*.10f,.24f,.08f,.11f,hair);                       // shoe
        rod({0,1.38f,side*.25f},{-lead*.18f,.88f,side*.27f},.045f,cloth,6);
        sphere(-lead*.18f,.84f,side*.27f,.05f,skin,4,6);
    }
    box(0,1.13f,0,.24f,.60f,.42f,cloth);
    rod({0,1.42f,0},{0,1.50f,0},.05f,skin,6);
    sphere(.01f,1.60f,0,.12f,skin,6,10);
    box(-.03f,1.69f,0,.20f,.08f,.22f,hair);
}

void addRoute(std::initializer_list<Vec3> points,bool closed=false) {
    Route r{points,closed,{},0};
    std::size_t segments=r.points.size()-(closed ? 0 : 1);
    for(std::size_t i=0;i<segments;++i) {
        float l=length(r.points[(i+1)%r.points.size()]-r.points[i]);
        r.lengths.push_back(l); r.total+=l;
    }
    routes.push_back(r);
}
// count people on the most recently added route, evenly spread with some jitter.
void walkers(int count,float seed) {
    int route=static_cast<int>(routes.size())-1; float span=routes[route].total*(routes[route].closed ? 1 : 2);
    for(int k=0;k<count;++k)
        people.push_back({route,static_cast<int>(hash1(seed+k*3.7f)*SHIRTS)%SHIRTS,1.05f+.5f*hash1(seed+k*1.9f),
                          span*(k+hash1(seed+k*5.3f)*.6f)/count});
}

} // namespace

void buildPeopleMeshes() {
    routes.clear(); people.clear(); personMeshes.clear();
    const Color shirts[SHIRTS]={{.80f,.80f,.76f},{.16f,.30f,.62f},{.70f,.18f,.16f},{.20f,.48f,.32f},{.86f,.66f,.20f}};
    for(int s=0;s<SHIRTS;++s) for(float stride : {1.0f,-1.0f}) {
        GLint first=static_cast<GLint>(mesh.size());
        personMesh(shirts[s],stride);
        personMeshes.push_back({first,static_cast<GLsizei>(mesh.size())-first});
    }
    const float g=.03f,f0=PLINTH,f1=PLINTH+STOREY_TALL;
    // Outside: plaza, entrance steps, lawn, campus road, street.
    addRoute({{-6,g,8},{7,g,21},{-4,g,24}},true);                                   walkers(3,1);
    addRoute({{1.2f,g,24},{1.2f,g,3.4f},{1.2f,f0,2.1f},{1.2f,f0,-10},{1.2f,f0,-18.7f}}); walkers(3,2);
    addRoute({{-10,g,-22},{-22,g,-36},{-12,g,-30}},true);                           walkers(2,3);
    addRoute({{44,g,24},{44,g,-58}});                                               walkers(3,4);
    addRoute({{-40,g,-65},{38,g,-65},{60,g,-65.6f}});                               walkers(4,5);
    addRoute({{-40,g,-75.3f},{62,g,-75.3f}});                                       walkers(5,6);
    // Inside, ground floor.
    addRoute({{1.2f,f0,-20},{1.2f,f0,-40}});                                        walkers(2,7);
    addRoute({{-2.4f,f0,-18},{-2.4f,f0,-46},{-12,f0,-46}});                         walkers(2,8);
    addRoute({{2,f0,-18.7f},{26,f0,-18.7f}});                                       walkers(2,9);
    addRoute({{8.5f,f0,-9.4f},{23.5f,f0,-9.4f}});                                   walkers(2,10);
    addRoute({{6.4f,f0,-42.9f},{19,f0,-42.9f}});                                    walkers(2,11);
    addRoute({{-7.3f,f0,-4.9f},{-22,f0,-4.9f}});                                    walkers(1,12);
    // Upper floor: terrace and corridor.
    addRoute({{-2.3f,f1,-20},{-2.3f,f1,-40}});                                      walkers(2,13);
    addRoute({{1.2f,f1,-14},{1.2f,f1,-38}});                                        walkers(2,14);
}

static bool personTransform(const Person& person,float time,Mat4& transform) {
    const Route& r=routes[person.route];
    float s=std::fmod(person.offset+person.speed*time,r.closed ? r.total : 2*r.total);
    bool returning=!r.closed&&s>r.total;                    // open routes are walked there and back
    if(returning) s=2*r.total-s;
    for(std::size_t i=0;i<r.lengths.size();++i) {
        if(s>r.lengths[i]&&i+1<r.lengths.size()) { s-=r.lengths[i]; continue; }
        Vec3 a=r.points[i],b=r.points[(i+1)%r.points.size()];
        Vec3 position=lerp(a,b,limit(s/r.lengths[i],0,1)),d=returning ? a-b : b-a;
        // The mesh faces +X; R_y(angle) turns +X to (cos, 0, -sin).
        transform=multiply(translation(position),rotationY(std::atan2(-d.z,d.x)));
        return true;
    }
    return false;
}

void collectMovers(float time,std::vector<MovingInstance>& out) {
    out.clear();
    Mat4 place;
    for(const Vehicle& vehicle:vehicles)
        if(vehicleTransform(vehicle,time,place))
            out.push_back({vehicleMeshes[vehicle.kind].first,vehicleMeshes[vehicle.kind].count,place});
    for(std::size_t i=0;i<people.size();++i)
        if(personTransform(people[i],time,place)) {
            // Two stride poses alternate about 1.9 times a second.
            int pose=static_cast<int>(time*1.9f*people[i].speed+i*.37f)&1;
            const VehicleMesh& range=personMeshes[people[i].shirt*2+pose];
            out.push_back({range.first,range.count,place});
        }
}

}} // namespace swc::app

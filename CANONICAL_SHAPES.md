# Canonical shapes

`src/geometry/canonical_shapes.h` provides reusable base geometry. It depends only
on `src/geometry/geometry_math.h` and the C++ standard library, so both headers can
be copied into another C++17 project without GLFW or OpenGL.

Each generator calls an emitter with a position and normal. Every three calls
form a triangle. The generator owns no buffers or graphics state.

| Generator in `swc::canonical` | Canonical coordinates | Defaults |
|---|---|---|
| `cube(out)` | Centre at origin; each axis from -0.5 to +0.5 | 12 triangles |
| `cylinder(out, segments)` | Radius 1; Y from -0.5 to +0.5; both caps | 20 segments |
| `sphere(out, rings, slices)` | Centre at origin; radius 1 | 8 rings, 14 slices |
| `cone(out, segments)` | Base radius 1 at Y=0; apex at Y=1; base cap | 20 segments |
| `frustum(out, topRatio, segments, bottomCap, topCap)` | Base radius 1 at Y=0; top radius `topRatio` at Y=1 | 0.5, 14, true, true |
| `tube(out, segments)` | Radius 1; Y from 0 to 1; open ends | 8 segments |
| `annulus(out, innerRatio, segments, a0, a1)` | XZ plane; outer radius 1; normal +Y | 0.5, 64, 0, 2*pi |
| `plane(out)` | XZ square from -0.5 to +0.5; normal +Y | 2 triangles |
| `triangle(out)` | XY points (0,0), (1,0), (0,1); normal +Z | 1 triangle |

Cylinder, cone, tube and frustum segment counts must be at least 3. Sphere
rings/slices must be at least 2/3. Ring segments must be positive, inner radius
ratio must be in [0,1], and frustum top ratio must be nonnegative. Invalid
values produce no vertices. Angles are radians; `trs` takes its yaw in degrees.

## Use a base shape in this scene

Inside a scene-building function, use the existing transform stack and material
emitter. These examples are illustrative; they are not added to the SWC view.

```cpp
// A 4 x 2 x 3 brick box, rotated 30 degrees around Y.
{
    Place instance(trs({10, 1, -5}, 30, {4, 2, 3}));
    canonical::cube([&](Vec3 p, Vec3 n) { emit(p, n, brick); });
}

// A column of radius 0.25 and height 6, standing on Y=0.
{
    Place instance(trs({12, 3, -5}, 0, {.25f, 6, .25f}));
    canonical::cylinder([&](Vec3 p, Vec3 n) { emit(p, n, concrete); });
}

// An ellipsoid: scale the unit sphere independently along each axis.
{
    Place instance(trs({8, 4, -5}, 0, {2, 3, 1}));
    canonical::sphere([&](Vec3 p, Vec3 n) { emit(p, n, grass); });
}
```

`Place` composes parent and local transforms. The order is `T * R * S`: scale,
then rotate, then translate. Its inverse-transpose normal matrix preserves
correct lighting for non-uniform scales. Use nonzero scales for lit shapes.

## Use the headers in another project

```cpp
#include "geometry/canonical_shapes.h"
#include <vector>

struct Vertex { swc::Vec3 position, normal; };

int main() {
    using namespace swc;
    std::vector<Vertex> vertices;
    Mat4 model = trs({3, 1, -2}, 45, {2, 2, 2});
    Mat3 normals = normalMatrix(model);
    canonical::cube([&](Vec3 p, Vec3 n) {
        vertices.push_back({transformPoint(model, p),
                            normalize(transformNormal(normals, n))});
    });
    // Upload vertices to your renderer or export them to your mesh format.
}
```

## How the SWC uses canonical geometry

The existing `box`, `block`, `cylinder`, `sphere`, `ellipsoid`, `frustum`, `rod`,
`annulus` and `stair` scene APIs remain available. All building and landscaping
calls retain their original arguments. Cubes, cylinders and spheres now use
canonical topology; rods and tapered shapes sweep the same canonical unit
circle. Rings use canonical angular sectors. Stairs are composites of boxes,
and custom curved facades continue to use triangle/quad geometry.

The adapters deliberately keep the original floating-point evaluation order,
triangle order, cap choices and tessellation. For example, existing scene rods
have open ends and the original winding, and existing frusta have no bottom
cap. Standalone canonical solids have outward winding and the caps listed
above. Adding caps or changing segment counts in the scene would change its
mesh and shadows, so the refactor preserves those choices.

## Verify the base geometry

```powershell
cmake -S . -B build -DSWC_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

The tests cover bounds, radii, triangle counts, normals, winding, sectors,
invalid tessellation inputs and normal transforms under non-uniform scaling.
Tests are optional and do not change the application or its startup view.

The refactor was also checked against the complete pre-refactor scene mesh:
882,180 vertices matched byte for byte. Perspective view 12 and orthographic
view 17 also produced byte-identical rendered images before and after the
refactor. Temporary comparison files are not required to build or run.

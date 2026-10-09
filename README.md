# KUET Student Welfare Centre — Interactive 3D Graphics Project

A C++17 / OpenGL 3.3 model of the Student Welfare Centre (SWC) at Khulna
University of Engineering & Technology, built from photographs. You can orbit
it, walk through it, watch fans, traffic and people move, and render any view
with a ray tracer. All geometry, mathematics, lighting and animation are
written in this project; no 3D model files, textures or math libraries are
loaded.

![The main entrance: colonnade, portico, curved brick wall and plaza artwork](docs/images/main-entrance.png)

**Contents**

1. [What the project contains](#1-what-the-project-contains)
2. [Build and run](#2-build-and-run)
3. [Controls](#3-controls)
4. [Command line](#4-command-line)
5. [Requirements checklist](#5-requirements-checklist)
6. [Project layout: every file explained](#6-project-layout-every-file-explained)
7. [How a frame is produced](#7-how-a-frame-is-produced)
8. [The model, space by space](#8-the-model-space-by-space)
9. [Graphics concepts and equations](#9-graphics-concepts-and-equations)
10. [Reference photographs](#10-reference-photographs)
11. [Testing and validation](#11-testing-and-validation)
12. [Known limits](#12-known-limits)

---

## 1. What the project contains

| Part | What it is |
|---|---|
| The building | Front colonnade and portico, curved brick "Student Welfare Center" wall, three-storey block, stage wing, cafeteria, right-hand shop wing |
| The interior | Entrance lobby, indoor-games hall, Shwapno shop, cafeteria dining hall, corridors, three glazed rooms, two stairs, first-floor terrace and hall |
| The site | Plaza with circular alpona artwork, courtyard lawn and stage, campus roads, hedges, palms and trees, bamboo gazebo |
| The street | Boundary wall, two gates, footpath, road with a central divider, a row of stalls, town buildings |
| Things that move | 21 ceiling fans, 7 vehicles (easybikes, cycle-vans, a bus), 35 pedestrians |
| Two renderers | A real-time OpenGL rasteriser and a CPU ray tracer, sharing one scene |
| A walking mode | First-person movement with collision, stairs and floor levels |

The scene is about 367,000 triangles, lit by one sun and 113 point lights.

![Aerial view of the whole site](docs/images/aerial-overview.png)

Coordinates: one unit is about one metre. **+Y is up, +X is to the right when
you face the main entrance, +Z points from the building toward the front
plaza.** The street is on the −Z side, behind the building.

---

## 2. Build and run

Requirements: CMake 3.20 or newer, a C++17 compiler (developed with MinGW
GCC 6.3 on Windows), and an OpenGL 3.3 graphics driver. GLFW 3.4 is downloaded
automatically the first time you configure, so that step needs internet access.

```powershell
cd D:\image_project
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
.\build\image_project.exe
```

- The build type defaults to `Release`. The ray tracer needs the optimiser;
  pass `-DCMAKE_BUILD_TYPE=Debug` only if you want to debug.
- **Close the viewer window before rebuilding.** Windows will not let the
  linker replace a running `image_project.exe`.
- To build the tests as well, add `-DSWC_BUILD_TESTS=ON` (see
  [section 11](#11-testing-and-validation)).

VS Code: `.vscode/c_cpp_properties.json` points IntelliSense at
`C:/MinGW/bin/g++.exe`. CMake writes `build/compile_commands.json` with the
real compiler flags. If include errors linger after configuring, run
**C/C++: Reset IntelliSense Database** and reload the window.

---

## 3. Controls

The program starts in **orbit mode**: the camera circles a target point.
Press `F6` for **walking mode**.

### Orbit mode

| Control | Action |
|---|---|
| Left mouse drag, or `A` `D` `W` `S` / arrow keys | Rotate around the target |
| Right mouse drag | Pan the target |
| Mouse wheel, or `Q` / `E` | Zoom out / in |
| `T` / `G` | Move the target forward / back (hold `Shift` for 3x speed) |
| `Page Up` / `Page Down` | Raise / lower the target |
| `J` / `L` | Rotate the whole model |
| `Z` / `X` | Scale the model down / up (0.5 to 1.5) |
| `U` / `O`, `I` / `K` | Translate the model left / right, away / toward |
| `C` | Reset the model transform |
| `R` | Return to the main entrance view |
| `F` | Main entrance view and reset the model transform |
| `B` | Toggle shadows |
| `P` | Pause / resume fans, traffic and pedestrians |
| `Y` | Ray trace the current view ([section 9.15](#915-ray-tracing)) |
| `Esc` | Quit |

### Walking mode (`F6`)

| Control | Action |
|---|---|
| `W` `A` `S` `D` | Walk (hold `Shift` to walk faster) |
| Arrow keys, or left mouse drag | Look around |
| `F6` or `R` | Leave walking mode |

Walking starts where the camera is if that is a floor you could stand on;
otherwise it starts on the front plaza. Walls, glass, railings, furniture and
low ceilings stop you. Steps and stairs change your height automatically.

### Views

Each view has a **capture number** used on the command line.

| Key | Capture | View |
|---|---|---|
| `1` | 1 | Main entrance |
| `2` | 2 | Curved SWC wall |
| `3` | 3 | Front-left colonnade |
| `4` | 4 | Front-right wide view |
| `5` | 5 | Cafeteria entrance |
| `6` | 6 | Courtyard and stage |
| `7` | 7 | Open courtyard lawn |
| `8` | 8 | Straight view from the entrance doors down the corridor |
| `9` | 9 | Aerial overview |
| `0` | 10 | Right-side shop wing |
| `V` | 11 | Cafeteria corner and external stair |
| `H` | 12 | Right-hand grilled entrance and straight corridor |
| `F1` – `F5` | 13 – 17 | Orthographic front, back, left, right elevations and top plan |
| — | 18 | Corridor in front of the three glazed rooms |
| `F7` | 19 | Inside a glazed room, with its ceiling fan |
| `F8` | 20 | Looking out through a room's glass front |
| `F9` | 21 | Gallery of the nine basic shapes (press again to return) |
| `Shift+1` | 22 | Entrance lobby |
| `Shift+2` | 23 | Indoor-games hall |
| `Shift+3` | 24 | Shwapno shop front |
| `Shift+4` | 25 | Cafeteria dining hall |
| `Shift+5` | 26 | Two-bay ground corridor |
| `Shift+6` | 27 | First-floor terrace |
| `Shift+7` | 28 | First-floor hall |
| `Shift+8` | 29 | Side corridor toward the stair hall |
| `Shift+9` | 30 | Curved stair, seen from the first-floor bridge |
| `Shift+0` | 31 | Corridor windows into the cafeteria |
| `Shift+H` | 32 | Stair at the end of the courtyard corridor |
| `Shift+V` | 33 | Stalls across the back road |
| `Shift+B` | 34 | Boundary wall and road |
| `N` | 35 | Campus gate, seen from the street |
| `Shift+N` (left Shift) | 36 | Open path from the portico to the curved stair |
| `F10` | 37 | Traffic on the back road |

---

## 4. Command line

```powershell
.\build\image_project.exe                                    # interactive viewer
.\build\image_project.exe --capture 9 aerial.ppm             # save one view and exit
.\build\image_project.exe --capture 17 plan.ppm 1600 1600    # with a size
.\build\image_project.exe --raytrace 25 cafe.ppm             # ray trace: 960 x 600, 4 samples
.\build\image_project.exe --raytrace 25 cafe.ppm 1000 750 9  # size and samples per pixel
.\build\image_project.exe --check-walk                       # run the walking-route checks
```

- `--capture <1-37> <file.ppm> [width height]` opens a hidden window, renders
  the view with OpenGL, writes a binary PPM image and exits. It needs a
  graphics driver. Fans, traffic and people are frozen at one fixed moment so
  captures are repeatable.
- `--raytrace <1-37> <file.ppm> [width height [samples]]` renders on the CPU
  with no window and no graphics driver.
- `--check-walk` builds the scene and walks a list of routes through it,
  printing each point reached. It exits with an error if a route is blocked.

PPM files open in GIMP, IrfanView and most image viewers. The PNG files in
`docs/images/` are lossless conversions of such captures.

---

## 5. Requirements checklist

The assignment requirements are in [src/requirement](src/requirement).

| # | Requirement | How it is met | Section |
|---|---|---|---|
| 1 | A virtual 3D environment | The SWC building, interior, site and street | [8](#8-the-model-space-by-space) |
| 2 | Objects from basic primitives | Nine canonical shapes; everything is assembled from them | [9.1](#91-primitives-and-triangle-meshes) |
| 3 | Translation, rotation, scaling | `translation()`, `rotationX()`, `rotationY()`, `scaling()` | [9.3](#93-transformations) |
| 4 | Composite transformations | `trs()` = T·R·S, the `Place` transform stack, instanced fans, vehicles, people | [9.3](#93-transformations), [9.4](#94-hierarchy-and-instancing) |
| 5 | Custom vector and matrix operations | `Vec3`, `Mat4`, `Mat3`, dot, cross, multiply, inverse-transpose | [9.2](#92-vectors-and-matrices) |
| 6 | Camera and viewing transformations | Custom `lookAt()`, orbit and walking cameras, 37 preset views | [9.5](#95-camera-and-viewing) |
| 7 | Perspective projection | Custom `perspective()`; also `orthographic()` | [9.6](#96-projection) |
| 8 | Interactive manipulation | Mouse and keyboard camera, model transform keys, toggles | [3](#3-controls) |
| 9 | Ambient, diffuse, specular lighting | Sun plus 113 point lights | [9.7](#97-phong-illumination), [9.8](#98-point-lights-and-emission) |
| 10 | Phong model, per-fragment shading | Fragment shader; the ray tracer evaluates the same model per ray | [9.7](#97-phong-illumination) |
| 11 | Explore from different viewpoints | Preset views, orthographic elevations, walking mode | [3](#3-controls), [9.14](#914-walking-and-collision) |

Beyond the requirements: shadow mapping, procedural materials and sky, alpha
blended glass, time-based animation, collision detection, and ray tracing.

---

## 6. Project layout: every file explained

```text
image_project/
├── CMakeLists.txt          build script
├── README.md               this file
├── SWC_Reference.txt       written description of every reference photograph
├── CANONICAL_SHAPES.md     notes on the nine basic shapes
├── include/                headers shared by the program and the tests
├── src/                    the program
├── tests/                  two small test programs
├── docs/images/            rendered images used in this README
├── project/                30 photographs: exterior and courtyard
├── project_2/              14 photographs: right-hand side
├── project_3/              24 photographs: interior walkthrough
├── road/                   25 photographs: the street behind the SWC
├── *.png (root)            three layout images for the stair
└── build/                  generated by CMake (not committed)
```

### `include/`

| File | Purpose |
|---|---|
| `gl_loader.h` | Declares the OpenGL 3.3 functions the program uses. Windows only ships OpenGL 1.1 headers, so newer functions are looked up at run time. |
| `walk_navigation.h` | `WalkNavigation`: the collision world for walking mode. Header-only so the tests can use it without a window. |
| `room_navigation.h` | Positions and sizes of the three glazed rooms and their fans. |

### `src/`

| File | Purpose |
|---|---|
| `main.cpp` | Entry point; calls `runApplication()`. |
| `gl_loader.c` | Looks up the OpenGL function pointers through GLFW. |
| `requirement` | The assignment requirements and their status. |
| **`app/`** | |
| `application.cpp` | Reads the command line and starts the viewer, the ray tracer or the walk check. |
| **`geometry/`** | |
| `geometry_math.h` | All vector and matrix mathematics. Header-only. |
| `canonical_shapes.h` | Generators for the nine unit shapes. Header-only. |
| `mesh.h`, `mesh.cpp` | The one vertex array for the whole scene, the transform stack (`Place`), and the shape builders `box`, `block`, `cylinder`, `frustum`, `rod`, `sphere`, `ellipsoid`, `annulus`, `stair`. Also the shared lists of lights, fans and vehicles. |
| **`scene/`** | |
| `layout.h` | Building dimensions shared by all modules: storey heights, the entrance axis, the curved wall's centre and radius, the sun direction. |
| `materials.h`, `.cpp` | Named materials (colour, gloss, pattern number). |
| `components.cpp` | Reusable building parts: `windowBay`, `door`, `acUnit`, `balconyPanel`, `openFrame`, `windowedWing`, `galleryWing`. |
| `lettering.cpp` | 5×5 block letters for signs; stylised strokes stand in for Bengali text. |
| `site.cpp` | Ground, plaza, alpona artwork, campus roads, paths. |
| `front.cpp` | Colonnade, portico, timber entrance, entrance corridor, corridor-end stair, the curved brick wall. |
| `courtyard.cpp` | Stage wing, the three-storey block's masses, the stage. |
| `exterior.cpp` | Shop wing, covered corridors, grilled entrance, cafeteria front and its external stair. |
| `interior.cpp` | Lobby, games hall, Shwapno shop, stair hall with the curved stair, cafeteria hall, terrace, upper hall, wing link, ceiling lights. |
| `rooms.cpp` | The three glazed rooms, the ceiling-fan mount, and the draw ranges for fans and glass. |
| `landscape.cpp` | Trees, palms, shrubs, hedges, the gazebo, and where they stand. |
| `street.cpp` | Boundary wall, gates, road, divider, stalls, town buildings, and the three vehicle meshes with their motion. |
| `people.cpp` | Pedestrian meshes, walking routes, and `collectMovers()`, which lists every vehicle and person for the current moment. |
| `base_objects.cpp` | The `F9` gallery of basic shapes. |
| `scene.cpp` | `buildScene()`: calls all of the above in order. |
| **`input/`** | |
| `camera.cpp` | The 37 view presets, orbit and walking cameras, all key and mouse handling. |
| **`navigation/`** | |
| `navigation.cpp` | Feeds the scene's triangles into `WalkNavigation`, and the `--check-walk` routes. |
| **`render/`** | |
| `shaders.cpp` | GLSL source: scene shader (Phong, shadows, patterns, point lights, fog), depth shader, sky shader, image shader. |
| `renderer.cpp` | The window and the frame loop. |
| `raytracer.cpp` | The CPU ray tracer. |
| `screenshot.cpp` | Reads the framebuffer and writes a PPM file. |

### `tests/`

| File | Purpose |
|---|---|
| `canonical_shapes_test.cpp` | Checks triangle counts, bounds and normals of the basic shapes. |
| `room_navigation_test.cpp` | Checks doors, walls, sliding, stairs, headroom and fan rotation in a small test world. |

---

## 7. How a frame is produced

**Once, at start-up.** `buildScene()` runs every scene module. Each one calls
shape builders, which push vertices into one array (`mesh`). A vertex is
eleven numbers: position (3), normal (3), colour (3), gloss (1) and a pattern
number (1). Placement transforms are applied as the vertices are created, so
the static scene is already in world coordinates. The array is uploaded to the
graphics card once.

The array is laid out in ranges:

| Range | Contents | Drawn |
|---|---|---|
| Static scene | Building, site, street | Once per pass |
| Fan rotor | One rotor | 21 times, one per fan |
| Vehicles | Easybike, cycle-van, bus | Once per visible vehicle |
| People | 10 meshes (5 shirts × 2 poses) | Once per person |
| Glass panes | One quad each | Sorted back to front, blended |
| Shape gallery | The `F9` display | Only in gallery mode |

**Every frame:**

```mermaid
flowchart LR
    A[Read input] --> B[Advance fans, traffic, people]
    B --> C[Shadow pass: depth from the sun]
    C --> D[Sky]
    D --> E[Pick the 24 nearest lights]
    E --> F[Opaque scene, fans, vehicles, people]
    F --> G[Glass, back to front]
    G --> H[Show the frame]
    A -. Y key .-> R[Ray trace on the CPU, show as an image]
    R --> H
```

---

## 8. The model, space by space

Each row names the function that builds the space, so you can find the code.

### Outside

| Space | What is modelled | Function |
|---|---|---|
| Plaza | Red pavers with tan groups, the circular alpona of rings and petals | `site()`, `alpona()` |
| Front wing | Double-height colonnade, two floors of barred windows with brick sills, AC units | `frontWing()`, `windowedWing()` |
| Portico | Canopy on tall columns, timber doors with louvres, upper balcony | `frontWing()` |
| Curved wall | A partial brick cylinder with a flared edge, pale bands, louvre strips with fins, lettering that follows the curve | `curvedBrickFacade()` |
| Courtyard | Lawn, stage wing with balconies and fins, brick backdrop, curved platform | `courtyard()`, `stage()` |
| Three-storey block | Galleries on three floors, window walls, roof housings | `courtyard()`, `exteriorElevations()` |
| Cafeteria front | Signed canopy, glazed balcony, tall louvre, external stair with a brick side wall | `cafeteria()`, `cafeteriaRightBlock()` |
| Shop wing | Thin canopy on tall columns, counters with stocked shelves, end balconies | `rightSideWing()` |
| Planting | Broadleaf trees, coconut, fan and areca palms, araucarias, shrubs, crotons, hedges, gazebo | `landscaping()` |

![Courtyard lawn and stage wing](docs/images/courtyard-stage.png)

### Inside

| Space | What is modelled | Function |
|---|---|---|
| Entrance lobby | Wide terracotta relief wall facing the plaza; a white side wall at a right angle to it with the INDOOR GAMES door; tournament board | `entranceLobby()`, `gamesHall()` |
| Indoor-games hall | Ground floor of the front wing: windows on both long walls, table-tennis tables, stacked chairs, desk, four fans | `gamesHall()` |
| Shwapno shop | On the right of the entrance hall: red fascia with the name, glazed bays, open doorway, shelves, gondola, sale stand | `shopHall()` |
| Corridor behind the curved wall | Tiled corridor with a diagonal-rib ceiling, from the entrance corridor to the right-hand side | `frontGapCorridor()`, `shopHall()` |
| Stair hall | Double-height hall inside the curved wall, entered from the portico or a side corridor. One curved flight of 20 steps follows the wall up to a first-floor bridge | `ovalStairHall()` |
| Three glazed rooms | Aluminium-framed glass fronts, desk, board, ceiling fan each | `room()` |
| Cafeteria hall | L-shaped dining hall: columns, beams, 24 table sets, counter, 14 fans. Its corridor wall is glass | `cafeteriaHall()` |
| Courtyard corridor | Two bays wide at ground level; black edge strip and accent tiles | `galleryWing()`, `courtyardTerrace()` |
| Corridor-end stair | Dog-leg stair serving all three floors, tall landing window | `northStairwell()` |
| First-floor terrace | Open terrace under white portal frames; brick-inset parapet, planters, bench | `courtyardTerrace()` |
| Wing link | Joins the three-storey block to the stage wing on both levels | `wingLink()` |
| First-floor hall | White wall with two timber double doors, seats, waste bin | `upperHall()` |

| | |
|---|---|
| ![Entrance lobby](docs/images/interior-lobby.png) Lobby — capture 22 | ![Indoor-games hall](docs/images/interior-games-hall.png) Games hall — capture 23 |
| ![Shwapno shop front](docs/images/interior-shop-hall.png) Shwapno — capture 24 | ![Cafeteria dining hall](docs/images/interior-cafeteria-hall.png) Cafeteria — capture 25 |
| ![Two-bay ground corridor](docs/images/interior-ground-corridor.png) Ground corridor — capture 26 | ![First-floor terrace](docs/images/interior-terrace.png) Terrace — capture 27 |
| ![First-floor hall](docs/images/interior-upper-hall.png) First-floor hall — capture 28 | ![Side corridor](docs/images/interior-stair-corridor.png) Side corridor — capture 29 |
| ![Curved stair](docs/images/interior-oval-stair.png) Curved stair — capture 30 | ![Open path into the stair hall](docs/images/interior-stair-entry.png) Path to the stair — capture 36 |
| ![Corridor windows into the cafeteria](docs/images/interior-cafeteria-windows.png) Cafeteria windows — capture 31 | ![Corridor-end stair](docs/images/interior-corridor-stair.png) Corridor-end stair — capture 32 |
| ![Inside a glazed room](docs/images/room-interior.png) Glazed room — capture 19 | ![Looking out through a room's glass front](docs/images/glass-entrance.png) Glass front — capture 20 |

### The street behind the SWC

| Element | What is modelled | Function |
|---|---|---|
| Boundary wall | White piers and coping with red-brick panels | `boundaryWall()` |
| Lattice gate | White diagonal-lattice gate, one leaf open | `latticeGate()` |
| Campus gate | Piers with globe lamps, two steel leaves with notice boards, brick steps | `campusGate()` |
| Road | Worn asphalt with a low concrete divider down the middle, open opposite the gate | `backStreet()` |
| Stalls | Tin-roofed stalls with lit signboards, stocked shelves, counters, plastic chairs | `stall()` |
| Town | Painted multi-storey buildings with balconies, utility poles | `townBuilding()` |
| Traffic | Four easybikes, two cycle-vans, one bus | `buildVehicleMeshes()` |
| People | 35 pedestrians on 14 routes, inside and outside the building | `buildPeopleMeshes()` |

| | |
|---|---|
| ![Stalls across the road](docs/images/street-stalls.png) Stalls — capture 33 | ![Boundary wall and road](docs/images/street-wall.png) Wall and road — capture 34 |
| ![Campus gate](docs/images/street-gate.png) Campus gate — capture 35 | ![Bus on the back road](docs/images/street-traffic.png) Traffic — capture 37 |
| ![Easybike](docs/images/traffic-easybike.png) Easybike | ![Cycle-van](docs/images/traffic-van.png) Cycle-van |

### How the spaces connect

```mermaid
flowchart LR
    Plaza --> Lobby[Entrance lobby]
    Lobby --> Games[Indoor-games hall]
    Lobby --> StairHall[Stair hall inside the curved wall]
    Lobby --> Axis[Entrance corridor]
    Axis --> Shop[Shwapno shop]
    Axis --> Behind[Corridor behind the curved wall]
    Behind --> StairHall
    Axis --> Straight[Straight corridor]
    Straight --> Rooms[Three glazed rooms]
    Straight --> SideDoor[Right-hand grilled entrance]
    Axis --> Gallery[Courtyard corridor]
    Gallery --> Dining[Cafeteria hall]
    Gallery --> EndStair[Corridor-end stair: all floors]
    Gallery --> StageWing[Stage wing]
    Gallery --> Lawn[Courtyard lawn]
    Dining --> Forecourt[Cafeteria forecourt]
    StairHall --> Upper[First-floor hall]
    Upper --> Terrace[First-floor terrace]
    Terrace --> StageWing
```

Every arrow can be walked in walking mode.

---

## 9. Graphics concepts and equations

Matrices multiply column vectors, so in a product the rightmost matrix acts
first. Angles are in radians unless marked.

### 9.1 Primitives and triangle meshes

Everything drawn is triangles. A four-sided face becomes two:
$(A,B,C,D)\rightarrow(A,B,C),(A,C,D)$.

[canonical_shapes.h](src/geometry/canonical_shapes.h) defines nine unit
shapes; [mesh.cpp](src/geometry/mesh.cpp) sizes and places them.

![The nine basic shapes](docs/images/basic-objects.png)

**Box.** Centre $(c_x,c_y,c_z)$, size $(w,h,d)$: corners at
$c_x\pm\tfrac w2,\;c_y\pm\tfrac h2,\;c_z\pm\tfrac d2$. Six faces, twelve
triangles. Used for walls, slabs, beams, steps, furniture.

**Cylinder and tube.** With $N$ segments and $\theta_i=2\pi i/N$:

$$
\mathbf p(\theta,y)=\begin{bmatrix}c_x+r_x\cos\theta\\y\\c_z+r_z\sin\theta\end{bmatrix}.
$$

Different $r_x,r_z$ give an elliptical cylinder. Used for columns, lamps, wheels.

**Cone and frustum.** The radius changes linearly with height $t\in[0,1]$:
$r(t)=(1-t)r_0+t\,r_1$. A cone has $r_1=0$. The side normal is proportional to
$(\cos\theta,\;(r_0-r_1)/h,\;\sin\theta)$. Used for planter pots and tree trunks.

**Sphere and ellipsoid.** Latitude $\phi$, longitude $\theta$:

$$
\mathbf p=\mathbf c+r\begin{bmatrix}\cos\phi\cos\theta\\\sin\phi\\\cos\phi\sin\theta\end{bmatrix}.
$$

Scaling the three axes differently gives an ellipsoid. Used for foliage, heads, lamp globes.

**Ring (annulus), plane, triangle.** A ring lies in the XZ plane between two
radii; limiting the angle gives an arc. Used for the alpona and road patches.

**Rod.** A cylinder between two arbitrary points $A$ and $B$. The axis is
$\hat{\mathbf a}=\operatorname{normalize}(B-A)$; two perpendicular vectors
$\mathbf u=\operatorname{normalize}(\hat{\mathbf a}\times\mathbf h)$ and
$\mathbf v=\hat{\mathbf a}\times\mathbf u$ span the cross-section. Used for
railings, branches, bicycle frames, arms and legs.

**Stair.** Step $i$ of a straight flight with riser $h_r$ and tread $d_t$ is a
box whose top is at $y_i=y_0+(i+1)h_r$ and which spans
$z\in[z_f-(i+1)d_t,\;z_f-i\,d_t]$.

**Curved surfaces from flat pieces.** The curved brick wall is 60 × 14 flat
quads on a cylinder. A point at angle $\theta$, height $y$, radius $R$ about
centre $(c_x,c_z)$ is

$$
\mathbf p=(c_x+R\sin\theta,\;y,\;c_z+R\cos\theta),
$$

and the left edge's starting angle shifts with height to make the flare.
Each vertex gets the true radial normal, so lighting is smooth across the facets.

### 9.2 Vectors and matrices

All in [geometry_math.h](src/geometry/geometry_math.h).

$$
\mathbf a\cdot\mathbf b=a_xb_x+a_yb_y+a_zb_z,\qquad
\|\mathbf a\|=\sqrt{\mathbf a\cdot\mathbf a},\qquad
\hat{\mathbf a}=\mathbf a/\|\mathbf a\|
$$

$$
\mathbf a\times\mathbf b=\begin{bmatrix}a_yb_z-a_zb_y\\a_zb_x-a_xb_z\\a_xb_y-a_yb_x\end{bmatrix}
$$

- The **dot product** measures how closely two directions agree. Lighting uses it.
- The **cross product** gives a vector perpendicular to two others. A flat
  triangle's normal is $\operatorname{normalize}((B-A)\times(C-A))$.
- **Linear interpolation**: $\operatorname{lerp}(A,B,t)=(1-t)A+tB$.

A 4×4 matrix product: $(AB)_{ij}=\sum_{k=0}^{3}A_{ik}B_{kj}$. Matrices are
stored column by column, as OpenGL expects.

### 9.3 Transformations

A point is written $(x,y,z,1)^T$ so that translation is also a matrix.

$$
T=\begin{bmatrix}1&0&0&t_x\\0&1&0&t_y\\0&0&1&t_z\\0&0&0&1\end{bmatrix},\quad
S=\begin{bmatrix}s_x&0&0&0\\0&s_y&0&0\\0&0&s_z&0\\0&0&0&1\end{bmatrix}
$$

$$
R_y(\theta)=\begin{bmatrix}\cos\theta&0&\sin\theta&0\\0&1&0&0\\-\sin\theta&0&\cos\theta&0\\0&0&0&1\end{bmatrix},\quad
R_x(\theta)=\begin{bmatrix}1&0&0&0\\0&\cos\theta&-\sin\theta&0\\0&\sin\theta&\cos\theta&0\\0&0&0&1\end{bmatrix}
$$

**Composite transformation.** `trs()` builds $M=T\,R_y\,S$: scale first, then
rotate, then translate. The order matters; translating first and then
rotating would swing the object around the origin.

**Normals.** A normal must stay perpendicular to its surface. Under
non-uniform scaling the model matrix does not preserve that, so normals use
the inverse transpose of its upper-left 3×3 part $A$:

$$
\mathbf n'=\operatorname{normalize}\left((A^{-1})^T\mathbf n\right).
$$

Where each kind of transformation is used:

| Transformation | Example in the project |
|---|---|
| Translation | Placing each window bay along a wall |
| Rotation about Y | Turning a wing to face east; aiming a pedestrian along a route |
| Rotation about X | Turning a cylinder on its side to make a wheel |
| Non-uniform scale | The shop wing is built at one size and scaled by (0.68, 1, 0.78) |
| Composite | Fan rotor: scene · translate to hub · rotate by time |
| Interactive | `J L Z X U O I K` apply $T\,R_y\,S$ to the whole model |

### 9.4 Hierarchy and instancing

**Transform stack.** `Place` is a small class. Creating one multiplies the
current matrix by a local matrix and pushes the result; when it goes out of
scope the previous matrix returns:

$$
M_{\text{child}}=M_{\text{parent}}\,M_{\text{local}}.
$$

So a window is written once in its own coordinates, placed in a wing, and the
wing is placed on the site. The cafeteria front, for example, is written
facing +Z and placed with one `Place` that turns it to face the road.

**Instancing.** One mesh drawn several times with different matrices:

| Mesh | Instances | Matrix |
|---|---|---|
| Fan rotor | 21 | $M_{\text{scene}}\,T(\mathbf h_k)\,R_y(\theta(t))$ |
| Easybike, cycle-van, bus | 7 vehicles | $M_{\text{scene}}\,T(x(t),0.03,z_l)\,R_y(0\text{ or }\pi)$ |
| Person (10 meshes) | 35 people | $M_{\text{scene}}\,T(\mathbf p(t))\,R_y(\psi)$ |

### 9.5 Camera and viewing

**Orbit camera.** A target $\mathbf c$, distance $d$, yaw $\psi$ and pitch
$\phi$ give the eye position on a sphere around the target:

$$
\mathbf e=\mathbf c+d\begin{bmatrix}\cos\phi\sin\psi\\\sin\phi\\\cos\phi\cos\psi\end{bmatrix}.
$$

Dragging changes $\psi$ and $\phi$. The wheel changes $d$ by a factor of
$0.9$ per notch, kept between 0.5 and 160.

**View matrix.** `lookAt()` builds three perpendicular axes from the eye,
the target and an up hint $\mathbf u_0$:

$$
\mathbf f=\operatorname{normalize}(\mathbf c-\mathbf e),\quad
\mathbf s=\operatorname{normalize}(\mathbf f\times\mathbf u_0),\quad
\mathbf u=\mathbf s\times\mathbf f
$$

$$
V=\begin{bmatrix}s_x&s_y&s_z&-\mathbf s\cdot\mathbf e\\u_x&u_y&u_z&-\mathbf u\cdot\mathbf e\\-f_x&-f_y&-f_z&\mathbf f\cdot\mathbf e\\0&0&0&1\end{bmatrix}
$$

$V$ moves the world so that the eye sits at the origin looking down −Z.

**Walking camera.** The same camera with $d=1$; the eye, not the target, is
what moves, and the collision system decides where it may go.

### 9.6 Projection

**Perspective.** With vertical field of view $\alpha$, aspect ratio $a$, near
and far distances $n,f$, and $q=1/\tan(\alpha/2)$:

$$
P=\begin{bmatrix}q/a&0&0&0\\0&q&0&0\\0&0&-\dfrac{f+n}{f-n}&-\dfrac{2fn}{f-n}\\0&0&-1&0\end{bmatrix}
$$

The last row copies depth into $w$. Dividing by $w$ afterwards makes distant
things smaller. The program uses $n=0.15$ and $f=450$.

**Orthographic.** For the elevations (`F1`–`F5`) and for the sun's shadow
camera, with bounds $l,r,b,t$:

$$
P_o=\begin{bmatrix}\dfrac{2}{r-l}&0&0&-\dfrac{r+l}{r-l}\\0&\dfrac{2}{t-b}&0&-\dfrac{t+b}{t-b}\\0&0&-\dfrac{2}{f-n}&-\dfrac{f+n}{f-n}\\0&0&0&1\end{bmatrix}
$$

No division by depth, so size does not depend on distance.

![Orthographic plan of the site](docs/images/orthographic-plan.png)

**The whole chain** for a vertex $\mathbf p$:

$$
\mathbf p_{\text{clip}}=P\,V\,M\,\mathbf p,\qquad
\mathbf p_{\text{ndc}}=\mathbf p_{\text{clip},xyz}/p_{\text{clip},w}.
$$

### 9.7 Phong illumination

Computed per pixel in the fragment shader
([shaders.cpp](src/render/shaders.cpp)), using the normal interpolated across
each triangle. With $\mathbf n$ the normal, $\mathbf l$ the direction to the
sun, $\mathbf v$ the direction to the eye, $g$ the material's gloss and $S_h$
the shadow factor (0 in shadow, 1 lit):

$$
\text{diffuse}\quad D=\max(\mathbf n\cdot\mathbf l,0)\,S_h
$$

$$
\text{specular}\quad\mathbf r=2(\mathbf n\cdot\mathbf l)\mathbf n-\mathbf l,\qquad
k=8+88g,\qquad H=\max(\mathbf v\cdot\mathbf r,0)^k\,g\,S_h
$$

$$
\text{ambient}\quad\mathbf A=0.62\,\operatorname{mix}\left(\mathbf C_{\text{ground}},\mathbf C_{\text{sky}},\tfrac12n_y+\tfrac12\right)
$$

$$
\mathbf C=\mathbf C_{\text{base}}\odot\left(\mathbf A+0.95\,\mathbf C_{\text{sun}}D+\mathbf L_d\right)+0.6\,\mathbf C_{\text{sun}}H+0.5\,g\,\mathbf L_s
$$

- **Ambient** stands for light arriving from everywhere. Surfaces facing up
  take the sky's colour, surfaces facing down take the ground's.
- **Diffuse** is brightest where the surface faces the light.
- **Specular** is the highlight; higher gloss makes it smaller and sharper.
- $\mathbf L_d,\mathbf L_s$ are the point-light sums of the next section.
- Interior materials use a neutral ambient $(0.44,0.44,0.42)$, since indoor
  surfaces are lit by bounce from white walls, not by blue sky.
- Normals facing away from the eye are flipped, so leaves and thin panels
  are lit on both sides.

### 9.8 Point lights and emission

Sunlight does not reach the corridors and halls, so every ceiling fixture
and stall lamp is a point light at $\mathbf p_i$ with colour $\mathbf c_i$ and
radius $r_i$. For a surface point $\mathbf p$:

$$
d_i=\|\mathbf p_i-\mathbf p\|,\qquad\mathbf l_i=(\mathbf p_i-\mathbf p)/d_i
$$

$$
a_i=s_i\,\frac{\left(1-(d_i/r_i)^4\right)^2}{1+0.22\,d_i+0.20\,d_i^2}
$$

The denominator is the standard constant–linear–quadratic falloff. The
numerator brings the light smoothly to zero at its radius. $s_i$ fades the
light out between 3.3 and 4.3 below the fixture, so a lamp lights only its
own storey.

$$
\mathbf L_d=\sum_i\mathbf c_i\,a_i\left(0.25+0.75\max(\mathbf n\cdot\mathbf l_i,0)\right),\qquad
\mathbf L_s=\sum_i\mathbf c_i\,a_i\max(\mathbf v\cdot\mathbf r_i,0)^k
$$

The 0.25 is a fill term standing in for light bounced off white surfaces.
The rasteriser sends the 24 lights nearest the eye to the shader each frame;
the ray tracer uses all of them.

**Emission.** Window glazing seen from indoors, fluorescent tubes, lamp
globes and lit signs output their own colour regardless of lighting.

### 9.9 Shadow mapping

1. Render the scene from the sun's position with an orthographic camera,
   keeping only depth, into a 4096 × 4096 texture.
2. When shading a pixel, transform its world position into the sun's view.
   If it is farther from the sun than the stored depth, something is in
   front of it: it is in shadow.

$$
\mathbf p_L=P_LV_L\begin{bmatrix}\mathbf p+0.05\,\hat{\mathbf n}\\1\end{bmatrix},\qquad
\mathbf q=\tfrac12\,\mathbf p_{L,xyz}/p_{L,w}+\tfrac12
$$

A small bias stops a surface from shadowing itself, larger where the surface
is at a grazing angle to the sun:
$\beta=0.0006+0.0012\,(1-\max(\mathbf n\cdot\mathbf l,0))$.

Nine neighbouring texels are tested and averaged (percentage-closer
filtering), which softens the edge:

$$
S_h=\frac19\sum_{i=-1}^{1}\sum_{j=-1}^{1}\left[\,q_z-\beta\le D_{\text{map}}\!\left(q_x+\tfrac{i}{4096},\,q_y+\tfrac{j}{4096}\right)\right]
$$

Fans, vehicles and people are drawn into the shadow map too, so they cast
moving shadows.

### 9.10 Procedural materials, sky and fog

No image textures are used. Each vertex carries a pattern number and the
shader computes the pattern from world position.

| Pattern | Used for | Method |
|---|---|---|
| Brick | Walls, curved wall | Running bond: every other row shifted half a brick, mortar lines, per-brick colour variation |
| Paving | Plaza, paths | Same, with paver proportions |
| Tile | Floors | Straight 0.6 m grid with thin joints |
| Natural | Road, bark, roof | Value noise at two scales |
| Lawn | Grass | Large patches plus fine blade noise |
| Thatch | Gazebo roof | Stretched noise |
| Glass | Windows | Reflection of sky or ground by viewing angle |
| Interior | Indoor surfaces | Neutral ambient |
| Emissive | Lamps, lit signs | Own colour only |

**Running bond.** For surface coordinates $(u,v)$ and a unit of size $(w,h)$:

$$
j=\lfloor v/h\rfloor,\qquad\mathbf c=\left(u/w+\tfrac12(j\bmod2),\;v/h\right)
$$

The fractional part of $\mathbf c$ says where in the brick the pixel is; near
zero it is mortar. The whole part identifies the brick and seeds a hash that
varies its colour.

**Value noise.** Random values on a grid, blended with the smooth weight
$w(t)=t^2(3-2t)$.

**Anti-aliasing of patterns.** When a brick becomes smaller than a pixel the
pattern would shimmer, so it is faded toward its average colour using the
screen-space derivative of the pattern coordinate.

**Sky.** A ray is formed for each pixel, the colour blends from horizon to
zenith by the ray's height, and clouds come from five layers of noise:

$$
C_{\text{cloud}}=\sum_{i=0}^{4}0.55\,(0.48)^i\,N(\mathbf p_i),\qquad\mathbf p_{i+1}=2.03\,\mathbf p_i+(3.7,1.8)
$$

**Fog.** Distant surfaces blend toward pale blue:
$F=1-e^{-0.006\max(d-45,0)}$, applied at 65% strength.

### 9.11 Transparent glass

Glass panes are drawn last, with blending on and depth writing off, sorted
from farthest to nearest so that panes seen through other panes combine
correctly:

$$
\mathbf C_{\text{out}}=\alpha\,\mathbf C_{\text{glass}}+(1-\alpha)\,\mathbf C_{\text{behind}},\qquad\alpha=0.20
$$

Glass also reflects more at shallow angles:
$F_g=0.08+0.62\,(1-\max(\mathbf n\cdot\mathbf v,0))^3$.

Used for the three rooms, the shop front and the cafeteria's corridor wall.
Glass is part of collision: you cannot walk through it.

### 9.12 Animation: fans

Each frame measures the time since the last one, capped at 0.05 s so a long
pause does not cause a jump:

$$
\Delta t=\min(t_{\text{now}}-t_{\text{prev}},\,0.05),\qquad
\theta\leftarrow(\theta+\omega\,\Delta t)\bmod2\pi,\qquad\omega=240^\circ/\text{s}
$$

The mount and rod are part of the static scene; only the rotor turns. The
hub of fan $k$ is $\mathbf h_k=(x_k,\;y_{\text{ceiling}}-0.64,\;z_k)$.

### 9.13 Animation: traffic and people

**Vehicles.** Each vehicle has a lane coordinate $z_l$, direction
$\sigma=\pm1$, speed $v$, starting offset $s_0$ and loop length $L$. The
distance travelled round its loop is

$$
s(t)=(s_0+vt)\bmod L.
$$

Only the first $D=116$ m of the loop is on the road. The vehicle is drawn
while $s\le D$, at

$$
x=\begin{cases}x_{\text{start}}+s&\sigma=+1\\x_{\text{end}}-s&\sigma=-1\end{cases}
$$

Traffic keeps left: the lane by the campus runs toward −X at 5.5 m/s with
$L=360$; the lane by the stalls runs toward +X at 4.5 m/s with $L=240$.
Vehicles in one lane share speed and loop, so their spacing never changes and
they cannot collide. The bus is on the road for $D/L\approx32\%$ of the time,
which is why it is seen only sometimes.

**People.** A route is a list of points $\mathbf w_0\ldots\mathbf w_n$ with
segment lengths $\ell_i$ and total $L$. A walker's distance along it is

$$
s(t)=\begin{cases}(s_0+vt)\bmod L&\text{closed loop}\\L-\left|(s_0+vt)\bmod2L-L\right|&\text{open route: there and back}\end{cases}
$$

On the segment containing $s$, at fraction $u$ along it, with travel
direction $\mathbf d$:

$$
\mathbf p=(1-u)\,\mathbf w_i+u\,\mathbf w_{i+1},\qquad\psi=\operatorname{atan2}(-d_z,\,d_x)
$$

The mesh faces +X and $R_y(\psi)$ turns +X to $(\cos\psi,0,-\sin\psi)$, so the
walker faces where it is going. Because $\mathbf p$ is interpolated in all
three coordinates, a route that climbs the entrance steps lifts the walker
with it. Two meshes with opposite legs forward alternate about twice a
second to suggest a stride.

### 9.14 Walking and collision

Collision uses the same triangles that are drawn (the static scene and the
glass), so what you see is what stops you. Moving things are not included.

**Spatial grid.** Triangles are filed into 2 m cells by
$(i,j)=(\lfloor x/2\rfloor,\lfloor z/2\rfloor)$, so each step tests only those nearby.

**The walker** is a cylinder: radius 0.18, height 1.78, eyes at 1.6. It can
step up 0.24 and down 0.65.

**Movement.** From forward input $f$, sideways input $r$ and camera yaw $\psi$:

$$
\mathbf d=(r\cos\psi-f\sin\psi,\;0,\;-r\sin\psi-f\cos\psi),\qquad
\Delta\mathbf p=\hat{\mathbf d}\,v\,\Delta t
$$

with $v=2.4$ m/s, or 4.2 with `Shift`. Normalising $\mathbf d$ stops diagonal
movement being faster. The step is split into pieces no longer than 0.06 so
thin walls cannot be skipped.

**Finding the floor.** For each nearly horizontal triangle $ABC$ nearby,
solve for the point's barycentric coordinates in plan and read off the height:

$$
\begin{bmatrix}B_x-A_x&C_x-A_x\\B_z-A_z&C_z-A_z\end{bmatrix}\begin{bmatrix}u\\v\end{bmatrix}=\begin{bmatrix}x-A_x\\z-A_z\end{bmatrix},\qquad
y=A_y+u(B_y-A_y)+v(C_y-A_y)
$$

The point is over the triangle when $u\ge0$, $v\ge0$, $u+v\le1$. The highest
reachable floor under the foot disc (centre plus eight rim points) is chosen.
If there is none, the move is refused, so you cannot walk off a balcony.

**Hitting things.** Every other triangle is clipped to the height range of
the walker's body and projected to plan. The closest point on an edge $AB$ to
the walker's centre $P$ is

$$
t=\operatorname{clamp}\left(\frac{(P-A)\cdot(B-A)}{\|B-A\|^2},0,1\right),\qquad Q=A+t(B-A)
$$

If $\|P-Q\|$ is less than the body radius, the move is blocked.

**Sliding.** If a move is blocked, its X part and its Z part are tried
separately, so you slide along a wall instead of stopping.

### 9.15 Ray tracing

[raytracer.cpp](src/render/raytracer.cpp) renders the same triangles,
materials, lights and camera without OpenGL.

![The cafeteria: rasterised on the left, ray traced on the right](docs/images/raster-vs-raytraced.png)

**Left: OpenGL. Right: ray tracer, same camera.** The ray tracer adds the
floor's mirror image of windows and furniture, a shadow from every lamp, and
darkening where surfaces meet.

Press `Y` to ray trace the current view at the window's size (one sample per
pixel; the window does not respond while it works). The picture stays until
you move the view. For better quality use `--raytrace` with more samples.

```mermaid
flowchart TD
    A[Sample inside a pixel] --> B[Ray from the eye]
    B --> C{Nearest triangle hit?}
    C -- no --> S[Sky colour]
    C -- yes --> D[Material colour and normal at the hit]
    D --> E[Ambient, reduced by occlusion rays]
    D --> F[Shadow ray to the sun]
    D --> G[Shadow ray to each nearby lamp]
    E --> H[Phong sum]
    F --> H
    G --> H
    H --> I{Glossy or glass, fewer than 3 bounces?}
    I -- yes --> J[Reflected ray; ray through glass]
    J --> C
    I -- no --> K[Fog, then average the samples]
```

**Rays from the camera.** This is projection run backwards: the rasteriser
maps points to pixels, the ray tracer maps pixels to directions. With the
camera axes $\mathbf f,\mathbf s,\mathbf u$ of section 9.5 and a sample at
$(p_x,p_y)$ in a $W\times H$ image:

$$
x_n=\frac{2p_x}{W}-1,\quad y_n=1-\frac{2p_y}{H},\qquad
\mathbf d=\operatorname{normalize}\left(\mathbf f+x_n\,a\tan\tfrac\alpha2\,\mathbf s+y_n\tan\tfrac\alpha2\,\mathbf u\right)
$$

Points on the ray are $\mathbf o+t\,\mathbf d$.

**Ray–triangle intersection (Möller–Trumbore).** A triangle with corner
$\mathbf p_0$ and edges $\mathbf e_1,\mathbf e_2$ contains the points
$\mathbf p_0+u\mathbf e_1+v\mathbf e_2$. Setting that equal to the ray and
solving, with $\mathbf s=\mathbf o-\mathbf p_0$:

$$
\mathbf h=\mathbf d\times\mathbf e_2,\quad\mathbf q=\mathbf s\times\mathbf e_1,\quad\Delta=\mathbf e_1\cdot\mathbf h,\qquad
u=\frac{\mathbf s\cdot\mathbf h}{\Delta},\quad v=\frac{\mathbf d\cdot\mathbf q}{\Delta},\quad t=\frac{\mathbf e_2\cdot\mathbf q}{\Delta}
$$

It is a hit when $u\ge0$, $v\ge0$, $u+v\le1$ and $t>0$. The same $u,v$
interpolate the three corner normals.

**Bounding-volume hierarchy.** Testing every triangle for every ray is far
too slow, so triangles are grouped into a tree of boxes. A ray that misses a
box skips everything inside. A ray meets a box if the intervals in which it
is between each pair of faces overlap:

$$
t_{\text{in}}=\max_k\min\left(\frac{b^{\min}_k-o_k}{d_k},\frac{b^{\max}_k-o_k}{d_k}\right),\qquad
t_{\text{out}}=\min_k\max\left(\frac{b^{\min}_k-o_k}{d_k},\frac{b^{\max}_k-o_k}{d_k}\right)
$$

with a hit when $t_{\text{out}}\ge t_{\text{in}}$ and $t_{\text{out}}>0$.
The tree is built by trying 12 split positions along the longest axis and
choosing the one with the lowest cost $A_LN_L+A_RN_R$, where $A$ is a child
box's surface area and $N$ its triangle count. A box's area is proportional
to the chance a random ray hits it, so this estimates the work ahead (the
surface-area heuristic).

**Shadow rays.** A light counts only if nothing is in the way. From the hit
point, nudged 4 mm off the surface, a ray goes toward each light and stops at
the first thing it meets. Shadows are exact and every lamp casts them.

**Reflection.** For polished surfaces the ray is mirrored about the normal,
$\mathbf d_r=\mathbf d-2(\mathbf d\cdot\mathbf n)\mathbf n$, and traced
again. How much is reflected depends on the viewing angle (Schlick's
approximation to the Fresnel equations):

$$
F=F_0+(1-F_0)(1-\cos\theta)^5,\qquad\mathbf C=(1-F)\,\mathbf C_{\text{local}}+F\,\mathbf C(\mathbf d_r)
$$

with $F_0=0.06$ for glass and $0.03+0.07g$ for other glossy materials.
Looking straight down at the floor it reflects a few percent; looking along
it, it is nearly a mirror.

**Through glass.** A thin flat pane bends a ray on the way in and bends it
back on the way out, so the ray continues in its original direction. The
tracer continues it and keeps 20% of the pane's own colour, matching the
rasteriser's blend.

**Ambient occlusion.** Eight short rays (1.4 m) are sent over the hemisphere
above the surface, and the ambient term is reduced by the fraction blocked:
$\mathbf A'=\mathbf A\,(1-0.70\,N_{\text{blocked}}/N)$. This darkens corners
and the floor under furniture. Directions are cosine-weighted: from random
numbers $\xi_1,\xi_2$, $r=\sqrt{\xi_1}$, $\phi=2\pi\xi_2$,
$\boldsymbol\omega=r\cos\phi\,\mathbf t+r\sin\phi\,\mathbf b+\sqrt{1-\xi_1}\,\mathbf n$.

**Anti-aliasing.** Each pixel is split into a $g\times g$ grid with one
randomly placed sample per cell, and the results are averaged.

**Threads.** Image rows are independent, so one worker thread per processor
takes rows from a shared counter.

| | Rasteriser | Ray tracer |
|---|---|---|
| What is visible | Depth buffer | Nearest hit in the tree |
| Sun shadows | Shadow map with 3×3 filtering | Shadow ray, exact |
| Lamp shadows | None | Shadow ray per lamp |
| Reflections | Estimated, on glass only | Traced, on every polished surface |
| Ambient | Constant per direction | Reduced by occlusion rays |
| Edge smoothing | 4× multisampling | Several samples per pixel |
| Speed | Real time | Seconds to minutes per frame |

| | |
|---|---|
| ![Ray-traced shop front](docs/images/raytraced-shop.png) Shop front | ![Ray-traced terrace](docs/images/raytraced-terrace.png) Terrace |
| ![Ray-traced corridor](docs/images/raytraced-corridor.png) Corridor beside the cafeteria | ![Ray-traced stair hall](docs/images/raytraced-oval-stair.png) Stair hall |
| ![Ray-traced entrance](docs/images/raytraced-entrance.png) Main entrance | |

These five ray-traced images were made at an earlier stage of the model. The
shop, the stair and the entrance have since changed, and the images predate
the traffic and the people. Re-run `--raytrace` to see the current state.

### 9.16 The curved stair

The stair follows the inside of the curved wall, so its steps are sectors of
a ring. With the wall's centre $(c_x,c_z)=(15,-3.5)$ and inner radius $7.06$,
the flight lies between radii $r_{\text{in}}=5.56$ and $r_{\text{out}}=7.02$.
Each of the 20 steps rises $h_r=3.6/20=0.18$ and turns through

$$
\Delta\theta=\frac{d_t}{r_{\text{mid}}}=\frac{0.28}{6.29}\ \text{rad}\approx2.55^\circ
$$

so that the tread is 0.28 deep along the middle of the step. Step $i$ covers

$$
\theta\in[\theta_0+i\,\Delta\theta,\;\theta_0+(i+1)\Delta\theta],\qquad
\mathbf p(\theta,r)=\left(c_x+r\sin\theta,\;0.45+(i+1)h_r,\;c_z+r\cos\theta\right)
$$

The whole flight turns about 51°. A solid balustrade on the inner radius
rises with the steps.

![Reference images beside the rendered corridor and stair](docs/images/stair-comparison.jpg)

---

## 10. Reference photographs

| Folder | Count | Shows |
|---|---|---|
| `project/` | 30 | Exterior, courtyard, entrance |
| `project_2/` | 14 | Right-hand shop wing, cafeteria corner |
| `project_3/` | 24 | Interior walkthrough, ground and first floor |
| `road/` | 25 | The street, wall, stalls, a gate |
| Root folder | 3 | `corridor-with-door.png`, `corridor-with-stair.png`, `stair-behind-oval.png` |

The program does not read these files. They are what the model was built
from. [SWC_Reference.txt](SWC_Reference.txt) describes every photograph and
records each decision about how the parts relate, including corrections
made along the way.

![Photographs beside the matching rendered views](docs/images/photo-comparison.jpg)

**Photograph on the left of each pair, model on the right.** Rows: lobby and
main entrance; shop and cafeteria; cafeteria windows and corridor-end stair;
terrace and first-floor hall; stalls and boundary wall.

Dimensions are estimated from the photographs. Nothing was measured on site.

---

## 11. Testing and validation

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DSWC_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

| Test | Checks |
|---|---|
| `canonical_shapes` | The basic shapes have the expected triangle counts, sizes and normals |
| `room_navigation` | In a small test world: doorways pass, walls and glass block, movement slides along walls, stairs change height, low ceilings block, fans rotate about a fixed hub |
| `swc_walk_routes` | Runs `--check-walk` on the full scene |

The route check walks, among others: the plaza, the entrance, the courtyard,
each glazed room, the shop, the corridor past it, the curved stair up and
down, the corridor-end stair to the second floor and back, the games hall
through its side door, the cafeteria through both entrances, the upper hall,
the first-floor terrace, and the link to the stage wing on both levels. It
also checks that four barriers stop the walker: the shop glazing, the
cafeteria's glass wall, the terrace parapet and the upper hall parapet.

What the tests do **not** cover: nothing compares rendered pixels, and
nothing compares the model with the photographs. Those were judged by eye.
Traffic and pedestrians were checked in still captures.

To reproduce any image in this README, use its capture number:

```powershell
.\build\image_project.exe --capture 27 terrace.ppm 1000 750
.\build\image_project.exe --raytrace 27 terrace-rt.ppm 1000 750 9
```

---

## 12. Known limits

**Accuracy to the building**
- Sizes are estimates. Room depths, column counts and furniture counts come
  from single photographs.
- Nothing shows the second-floor interior or the rooms behind closed doors.
  The three glazed rooms with fans are illustrative, not photographed.
- The street's side of the campus and its distance from the building are
  assumed. Stalls and town buildings are generated, not copied one by one.
- The side corridor to the stair is about 5 m long; the reference shows a
  longer one. The Shwapno shop is shallower than photographed.
- Bengali lettering is replaced by generated strokes. People, banners,
  posters and parked vehicles in the photographs are left out.

**Rendering**
- Window glazing seen from inside the halls is a bright panel, not an
  opening, so sunlight does not enter through it.
- The rasteriser gives no shadows from point lights and uses only the 24
  nearest. The ray tracer has no soft shadows, no refraction through thick
  glass, and no bounced light beyond the ambient and fill terms.
- The curved stair's balustrade is 20 flat pieces stepping up with the
  treads, not one smooth sloping wall.

**Movement**
- Vehicles appear and disappear at the ends of the road, do not turn, and
  their wheels do not spin.
- People follow fixed routes and do not avoid each other, vehicles or the
  camera. Their stride is two alternating poses.
- Vehicles and people are not part of collision: in walking mode you pass
  through them.
- There is no jumping or falling; a move with no floor under it is refused.

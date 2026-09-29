# KUET Student Welfare Centre - Interactive 3D OpenGL Project

A C++17/OpenGL 3.3 architectural visualization of KUET's Student Welfare
Centre (SWC), modeled from the 30 reference photographs in `project/`.

The layout follows `SWC_Reference.txt`: connected wings of differing heights
around a large courtyard lawn, the deep front portico and long colonnade, the
flared curved brick SWC facade with bands, louvers and bilingual lettering,
the courtyard stage, the cafeteria with its external stair, the plaza alpona,
the bamboo gazebo, roads, hedges and palms. Lighting is per-fragment Phong
with shadow mapping from one consistent sun.

The corrected layout leaves the left side open. Behind the entrance and curved
SWC sign, a covered tiled corridor connects to the three-storey block, with
aligned upper walkways and return stairs serving all three floors. The central
entrance doorway is open so the route through to the courtyard is visible.
The three-storey block is shifted forward and onto the doorway's centreline:
walking through the main door now continues straight along its corridor.
Its room side extends to the right edge of the oval SWC facade, with additional
front window bays; the corridor and stair alignment stays fixed.
The cafeteria occupies the rear bays beside the gazebo, at a setback of 20
units from the old side entrance. Facing it, the external stair is on the
right and joins a separate two-storey balcony/windowed block. Camera preset
`5` faces the entrance and `V` shows the complete corner.

The 14 additional photographs in `project_2/` guide the right-side shop wing:
a two-storey shop canopy extends back from the curved sign,
with tall free-standing columns, a recessed plain upper wall, open
shops and brick-inset end balconies. Farther back, the cafeteria's right-hand
stair connects to the grilled balcony of its adjoining low block. The roadside
planting, raised tiled arcade, and paved forecourt follow the new views.
Dimensions remain visual estimates from photographs, rather than a survey.

The grass gap behind the oval/front rooms is now a covered tiled corridor,
with an upper walkway, brick-inset balcony and open grille entrance based on
`project_2/WhatsApp Image 2026-09-29 at 6.42.32 PM.jpeg`. Side doorways connect
both levels to the main entrance corridor. The stall wing is 32% shorter and
22% narrower and is tucked closer behind the oval facade.
The passage also runs along the inner side of the stall wing, filling the
marked strip beside the taller block. Its rear grille entrance and upper
balcony connect continuously around the corner to the front corridor;
the stall rooms are recessed to keep that route clear. `H` shows this entrance.
The ground-floor door enclosures behind the oval, roller shutter, seats and
bin have been removed. Two counters sit farther left; the clear bay to their
right has front steps and a paved approach through the hedge, allowing entry
into the corridor from the front. `H` now faces that front approach.
The front entrance follows the 6.42.32 PM photo with an open upper balcony,
brick-inset parapet, metal grille leaves and transom, diagonal soffit members,
small entrance pots, and the adjacent three-storey window wall. The central
gate opening remains clear; the two stalls stay to its left.
The doorway now sits farther right, replacing the ground-floor window bay.
Both stalls are wider, and the steps and approach follow the relocated door.
A straight tiled corridor passes through the room block to the courtyard;
the opening is cut through the walls, rather than ending at a window or wall.

## Build and run (PowerShell)

```powershell
cd D:\image_project
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
.\build\image_project.exe
```

The current executable is `build/image_project.exe`. Close the viewer before
rebuilding so Windows can replace it. The old `build/updated/` copy is obsolete.

The first CMake configuration downloads GLFW. Later builds normally work
offline. The program opens at a front three-quarter presentation view.

## Camera controls

| Control | Action |
|---|---|
| `1` - `9` | Reference-photo views (see below) |
| `0` | Right-side shops and curved-wall connection |
| `V` | Right-side balcony, external stair, and cafeteria forecourt |
| `H` | Covered corridor behind the oval facade |
| `F1` - `F5` | Orthographic front / back / left / right / top (plan) views |
| Hold left mouse + drag | Orbit around the view target |
| Hold right mouse + drag | Pan the view target |
| Mouse wheel | Zoom in or out |
| `A` / `D` or Left / Right | Rotate horizontally |
| `W` / `S` or Up / Down | Rotate vertically |
| `Q` / `E` | Zoom out / in |
| `T` / `G` | Walk the target forward / back (hold Shift to go faster) |
| `Page Up` / `Page Down` | Raise / lower the target |
| `R` | Return to the main entrance view |
| `F` | Main entrance view and reset model transform |
| `B` | Toggle shadows |
| `J` / `L` | Rotate the 3D model |
| `Z` / `X` | Scale the 3D model down / up |
| `U` / `O` | Translate the 3D model left / right |
| `I` / `K` | Translate the 3D model away / toward the camera |
| `C` | Reset model translation, rotation, and scale |
| `Esc` | Close the program |

| Key | View | Reference photo |
|---|---|---|
| `1` | Main entrance | 3.08.02 AM |
| `2` | Curved SWC facade | 3.08.02 AM (1) |
| `3` | Front-left colonnade | 3.08.01 AM (1) |
| `4` | Front-right wide view | 3.08.04 AM |
| `5` | Cafeteria entrance | 3.07.55 AM |
| `6` | Courtyard stage | 3.07.59 AM |
| `7` | Open left courtyard lawn | Corrected site layout |
| `8` | Straight view through entrance door into the three-storey corridor | Corrected alignment |
| `9` | Aerial overview | - |
| `0` | Right-side shops | project_2, 6.42.27-6.42.31 PM |
| `V` | Right-side stair and balcony | project_2, 6.42.34 PM |
| `H` | Covered connecting corridor | project_2, 6.42.32 PM |
| `F1` | Front elevation (orthographic) | All 44 photos |
| `F2` | Back elevation (orthographic) | All 44 photos |
| `F3` | Left elevation (orthographic) | All 44 photos |
| `F4` | Right elevation (orthographic) | All 44 photos |
| `F5` | Top view / plan, front at the bottom (orthographic) | All 44 photos |

`image_project.exe --capture <1-12> out.ppm` renders one view in a hidden window
and exits. Capture `10` is the `0` key view; capture `11` is the `V` key view.
Capture `12` is the `H` key view. Captures `13`-`17` are the `F1`-`F5`
orthographic views. An optional size can follow the file name:

```powershell
.\build\image_project.exe --capture 13 front.ppm 2200 600   # front elevation
.\build\image_project.exe --capture 17 top.ppm 1600 1600    # top view
```

Rendered front, back, left, right and top views are in `build/views/`, with
all five on one sheet in `build/views/SWC_all_views.png`.

## Scene structure

`buildScene()` in `src/main.cpp` assembles the model from reusable components
placed with composite `T * R * S` transforms on a transform stack (`Place`):

- `windowBay`, `acUnit`, `door`, `balconyPanel`, `openFrame`, `stair`
- `windowedWing` (long windowed wings) and `galleryWing` (balcony corridors)
- `curvedBrickFacade`, `frontWing`, `entranceCorridor`, `connectedStairwell`,
  `courtyard`, `stage`, `rightSideWing`, `frontGapCorridor`, `cafeteria`, `cafeteriaRightBlock`
- `broadleafTree`, `coconutPalm`, `fanPalm`, `arecaPalm`, `araucaria`,
  `shrub`, `croton`, `hedge`, `gazebo`, `alpona`

Wing lengths, bay counts, storey heights and the curved wall's radius, arc,
flare and heights are parameters, so they can be refined against the photos.

## Folder layout

```text
image_project/
|-- CMakeLists.txt
|-- include/gl_loader.h
|-- src/gl_loader.c
|-- src/main.cpp
|-- project/             30 reference photographs
|-- project_2/           14 additional right-side photographs
`-- build/               generated files and executable
```

## Everyday workflow

1. Edit `src/main.cpp`; scene geometry starts in `buildScene()`.
2. Run `cmake --build build --parallel`.
3. Run `.\build\image_project.exe`.

The photographs remain in `project/` for visual comparison. They are reference
inputs; the executable does not need to load them at runtime.

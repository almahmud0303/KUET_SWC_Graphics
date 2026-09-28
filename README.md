# KUET Student Welfare Centre - Interactive 3D OpenGL Project

A C++17/OpenGL 3.3 architectural visualization of KUET's Student Welfare
Centre (SWC), modeled from the 30 reference photographs in `project/`.

The layout follows `SWC_Reference.txt`: connected wings of differing heights
around a large courtyard lawn, the deep front portico and long colonnade, the
flared curved brick SWC facade with bands, louvers and bilingual lettering,
the courtyard stage, the cafeteria with its external stair, the plaza alpona,
the bamboo gazebo, roads, hedges and palms. Lighting is per-fragment Phong
with shadow mapping from one consistent sun.

## Build and run (PowerShell)

```powershell
cd D:\image_project
cmake -S . -B build -G "MinGW Makefiles"
cmake --build build --parallel
.\build\image_project.exe
```

The first CMake configuration downloads GLFW. Later builds normally work
offline. The program opens at a front three-quarter presentation view.

## Camera controls

| Control | Action |
|---|---|
| `1` - `9` | Reference-photo views (see below) |
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
| `7` | Windowed courtyard wing | 3.07.56 AM |
| `8` | Ground-floor corridor | 3.07.57 AM |
| `9` | Aerial overview | - |

`image_project.exe --capture <1-9> out.ppm` renders one view to an image file
and exits, which is useful for comparing against the photographs.

## Scene structure

`buildScene()` in `src/main.cpp` assembles the model from reusable components
placed with composite `T * R * S` transforms on a transform stack (`Place`):

- `windowBay`, `acUnit`, `door`, `balconyPanel`, `openFrame`, `stair`
- `windowedWing` (long windowed wings) and `galleryWing` (balcony corridors)
- `curvedBrickFacade`, `frontWing`, `courtyard`, `stage`, `cafeteria`, `shopfront`
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
`-- build/               generated files and executable
```

## Everyday workflow

1. Edit `src/main.cpp`; scene geometry starts in `buildScene()`.
2. Run `cmake --build build --parallel`.
3. Run `.\build\image_project.exe`.

The photographs remain in `project/` for visual comparison. They are reference
inputs; the executable does not need to load them at runtime.

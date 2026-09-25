# KUET Student Welfare Centre - Interactive 3D OpenGL Project

A C++17/OpenGL 3.3 architectural visualization of KUET's Student Welfare
Centre (SWC), modeled from the 30 reference photographs in `project/`.

The scene includes the curved brick frontage, main portico, two-storey wings,
internal courtyard, stage, windows, galleries, plaza, trees, and landscaping.

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
| Hold left mouse + drag | Orbit around the building |
| Mouse wheel | Zoom in or out |
| `A` / `D` or Left / Right | Rotate horizontally |
| `W` / `S` or Up / Down | Rotate vertically |
| `Q` / `E` | Zoom out / in |
| `R` | Reset the presentation view |
| `J` / `L` | Rotate the 3D model |
| `Z` / `X` | Scale the 3D model down / up |
| `C` | Reset model rotation and scale |
| `Esc` | Close the program |

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

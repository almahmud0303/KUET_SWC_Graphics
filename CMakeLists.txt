cmake_minimum_required(VERSION 3.20)
project(image_project VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Keep editor include paths and compiler flags aligned with the actual build.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glfw)

find_package(OpenGL REQUIRED)

add_executable(image_project
    src/app/application.cpp
    src/geometry/mesh.cpp
    src/input/camera.cpp
    src/main.cpp
    src/navigation/navigation.cpp
    src/render/renderer.cpp
    src/render/screenshot.cpp
    src/render/shaders.cpp
    src/scene/base_objects.cpp
    src/scene/components.cpp
    src/scene/courtyard.cpp
    src/scene/exterior.cpp
    src/scene/front.cpp
    src/scene/landscape.cpp
    src/scene/lettering.cpp
    src/scene/materials.cpp
    src/scene/rooms.cpp
    src/scene/scene.cpp
    src/scene/site.cpp
    src/gl_loader.c
)
target_include_directories(image_project PRIVATE include src)
target_link_libraries(image_project PRIVATE glfw OpenGL::GL)

if(MSVC)
    target_compile_options(image_project PRIVATE /W4)
else()
    target_compile_options(image_project PRIVATE -Wall -Wextra -Wpedantic)
endif()

option(SWC_BUILD_TESTS "Build renderer-independent canonical geometry checks" OFF)
if(SWC_BUILD_TESTS)
    enable_testing()
    add_executable(canonical_shapes_test tests/canonical_shapes_test.cpp)
    target_include_directories(canonical_shapes_test PRIVATE include src)
    add_test(NAME canonical_shapes COMMAND canonical_shapes_test)
    add_executable(room_navigation_test tests/room_navigation_test.cpp)
    target_include_directories(room_navigation_test PRIVATE include src)
    add_test(NAME room_navigation COMMAND room_navigation_test)
    add_test(NAME swc_walk_routes COMMAND image_project --check-walk)
endif()

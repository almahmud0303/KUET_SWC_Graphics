cmake_minimum_required(VERSION 3.20)
project(image_project VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

add_executable(image_project src/main.cpp src/gl_loader.c)
target_include_directories(image_project PRIVATE include)
target_link_libraries(image_project PRIVATE glfw OpenGL::GL)

if(MSVC)
    target_compile_options(image_project PRIVATE /W4)
else()
    target_compile_options(image_project PRIVATE -Wall -Wextra -Wpedantic)
endif()

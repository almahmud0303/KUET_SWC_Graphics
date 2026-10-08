#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "geometry/geometry_math.h"

namespace swc { namespace app {

extern const int presetCount;
extern Vec3 cameraTarget,modelTranslation;
extern float cameraYaw,cameraPitch,cameraDistance,cameraFov,cameraOrthoRatio,modelRotation,modelScale;
extern bool shadowsOn;
extern int framebufferWidth,framebufferHeight;

void applyPreset(int index);
Vec3 cameraEye();
void pan(float right,float forward);
void resize(GLFWwindow*,int width,int height);
void mouseButton(GLFWwindow* window,int button,int action,int);
void mouseMove(GLFWwindow*,double x,double y);
void scroll(GLFWwindow*,double,double offset);
bool down(GLFWwindow* window,int key);
bool pressed(GLFWwindow* window,int key);
void input(GLFWwindow* window,float delta);

}} // namespace swc::app

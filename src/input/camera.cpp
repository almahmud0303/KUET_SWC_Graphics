#include "input/camera.h"

#include "geometry/mesh.h"
#include "navigation/navigation.h"
#include "room_navigation.h"
#include "scene/layout.h"
#include <iostream>

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Camera: orbit around a target, plus presets matching the reference photos.
// ---------------------------------------------------------------------------
// orthoHalf > 0 selects an orthographic projection of that half-height
// (architectural elevations and plan); 0 keeps the perspective fov.
struct CameraPreset { const char* name; Vec3 eye, target; float fov; float orthoHalf=0; };
// Building footprint (without site and planting): x -40..32.9, z -60.3..3.2, top 13.8.
constexpr float PLAN_CENTER_X=-3.55f,PLAN_CENTER_Z=-28.5f;
const CameraPreset presets[]={
    {"Main entrance (photo 26)",               {3.5f,1.7f,31.0f},{4.0f,4.3f,-2.0f},55},
    {"Curved SWC facade (photo 25)",           {8.0f,1.6f,17.5f},{15.0f,4.6f,0.0f},60},
    {"Front-left colonnade (photo 23)",        {8.0f,1.7f,22.0f},{-10.0f,3.6f,-1.0f},60},
    {"Front-right wide view (photo 30)",       {-5.0f,1.7f,27.0f},{8.0f,4.0f,0.0f},60},
    {"Cafeteria entrance (photo 12)",          {45.0f+TALL_BLOCK_OFFSET.x+TALL_EXTRA_WIDTH,1.7f,-27.5f+TALL_BLOCK_OFFSET.z-CAFETERIA_SETBACK},{TALL_EXTERIOR_X+TALL_BLOCK_OFFSET.x,5.2f,-27.0f+TALL_BLOCK_OFFSET.z-CAFETERIA_SETBACK},60},
    {"Courtyard stage (3.07.59 AM photo)",      {-20.0f,1.65f,-15.0f},{-20.0f,4.7f,-44.3f},65},
    {"Open left courtyard lawn",              {-3.5f,1.7f,-23.0f},{-19.0f,3.6f,-33.0f},62},
    {"Straight entrance-to-corridor view",     {ENTRANCE_AXIS,2.10f,-4.5f},{ENTRANCE_AXIS,2.10f,-34.0f},66},
    {"Aerial overview",                        {48.0f,58.0f,52.0f},{0.0f,0.0f,-20.0f},50},
    {"Right-side shops (project_2)",           {44.0f,2.1f,8.0f},{27.0f,3.9f,-15.0f},62},
    {"Cafeteria and right-hand stair (project_2)",{42.0f,2.0f,CAFE_WORLD_Z+1.0f},{26.0f,4.0f,CAFE_WORLD_Z-10.0f},64},
    {"Right-hand entrance and straight corridor",{37.0f,1.8f,-19.1f},{5.0f,2.1f,-19.1f},64},
    // Orthographic views; the eye sits just outside the planting so distant site trees are clipped.
    {"Front elevation",                        {PLAN_CENTER_X,10.0f,30.0f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z},50,10.0f},
    {"Back elevation",                         {PLAN_CENTER_X,10.0f,-72.0f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z},50,10.0f},
    {"Left elevation",                         {-40.0f,10.0f,PLAN_CENTER_Z+4.5f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z+4.5f},50,10.0f},
    {"Right elevation",                        {46.0f,10.0f,PLAN_CENTER_Z+4.5f},{PLAN_CENTER_X,10.0f,PLAN_CENTER_Z+4.5f},50,10.0f},
    {"Top view (plan)",                        {PLAN_CENTER_X,40.0f,PLAN_CENTER_Z+6.0f},{PLAN_CENTER_X,0.0f,PLAN_CENTER_Z+6.0f},50,40.0f},
    {"Room corridor / walk start", {11.2f,2.05f,-18.3f},{11.2f,2.05f,-23.0f},72},
    {"Room interior and ceiling fan", {11.2f,2.05f,-21.2f},{11.2f,2.7f,-25.0f},78},
    {"Room glass entrance from inside", {12.8f,2.05f,-24.5f},{10.8f,2.15f,-19.5f},78},
    {"Canonical base objects", {9,14,-5},{0,.3f,-20},48},
    // Interior views matched to the photographs (Shift+1 ... Shift+9, Shift+0, Shift+H).
    {"Entrance lobby (6.14.05 PM)",             {2.9f,1.8f,-.9f},{-5.0f,2.6f,-4.6f},80},
    {"Indoor games hall (6.14.06 PM)",          {-4.9f,2.0f,-4.85f},{-20.0f,1.7f,-7.2f},78},
    {"Shwapno shopfront (6.14.06 PM (2))",      {-.25f,1.9f,-13.7f},{3.5f,2.0f,-12.3f},92},
    {"Cafeteria dining hall (6.14.08 PM (2))",  {4.5f,2.1f,-38.2f},{16.0f,1.3f,-46.5f},80},
    {"Two-bay ground corridor (6.14.11 PM (1))",{-2.7f,2.0f,-40.2f},{.4f,1.8f,-18.0f},76},
    {"First-floor terrace (6.14.11 PM (2))",    {-1.3f,5.75f,-40.4f},{-2.7f,4.9f,-20.0f},80},
    {"First-floor hall (6.14.13 PM (1))",       {3.2f,5.7f,-6.9f},{-1.6f,5.0f,-10.4f},86},
    {"Side corridor to the stair (corridor-with-stair.png)",{15.5f,1.9f,-10.7f},{16.1f,2.0f,-1.0f},72},
    {"Stair behind the curved wall (stair-behind-oval.png)",{15.0f,5.75f,.2f},{11.2f,1.5f,1.5f},92},
    {"Corridor windows into the cafeteria (6.14.08 PM (1))",{-.1f,2.0f,-28.6f},{3.6f,1.8f,-34.6f},82},
    {"Corridor-end stair (3.07.57 AM)",          {1.3f,2.0f,-27.0f},{1.4f,2.3f,-46.0f},70},
    // road/ photographs: the street behind the SWC (Shift+V, Shift+B).
    {"Stalls across the back road (road photos)", {4.0f,1.7f,-65.2f},{2.0f,2.2f,-80.0f},74},
    {"Boundary wall and gate from the road (road photos)",{-14.0f,1.7f,-72.5f},{12.0f,1.6f,-63.5f},70},
    {"Campus gate on the east road (gate photo)",   {44.0f,1.7f,-69.6f},{44.0f,1.9f,-63.5f},72},
    {"Open path from the portico to the curved stair",{9.1f,1.8f,-2.2f},{12.4f,2.0f,1.8f},84},
    {"Traffic on the back road",                    {-34.0f,7.0f,-62.0f},{2.0f,.6f,-71.0f},68},
};
const int presetCount=sizeof(presets)/sizeof(presets[0]);

Vec3 cameraTarget{0,0,0};
float cameraYaw=0,cameraPitch=0,cameraDistance=30,cameraFov=55;
float cameraOrthoRatio=0; // orthographic half-height per unit of distance, so zoom still works
float modelRotation=0.0f,modelScale=1.0f;
Vec3 modelTranslation{0.0f,0.0f,0.0f};
bool rotating=false,panning=false,shadowsOn=true,walking=false,rayTraceRequested=false;
double lastMouseX=0.0,lastMouseY=0.0;
int framebufferWidth=1280,framebufferHeight=800;

void applyPreset(int index) {
    walking=false;
    showingBaseObjects=index==20;
    const CameraPreset& p=presets[index];
    Vec3 d=p.eye-p.target;
    cameraTarget=p.target; cameraDistance=length(d); cameraFov=p.fov;
    cameraOrthoRatio=p.orthoHalf/cameraDistance;
    cameraPitch=std::asin(d.y/cameraDistance)*180.0f/PI;
    cameraYaw=std::atan2(d.x,d.z)*180.0f/PI;
    std::cout<<"View: "<<p.name<<'\n';
}
Vec3 cameraEye() {
    float yaw=radians(cameraYaw),pitch=radians(cameraPitch);
    return {cameraTarget.x+cameraDistance*std::cos(pitch)*std::sin(yaw),
            cameraTarget.y+cameraDistance*std::sin(pitch),
            cameraTarget.z+cameraDistance*std::cos(pitch)*std::cos(yaw)};
}
void pan(float right,float forward) {
    float yaw=radians(cameraYaw);
    Vec3 f{-std::sin(yaw),0,-std::cos(yaw)},r{std::cos(yaw),0,-std::sin(yaw)};
    cameraTarget=cameraTarget+r*right+f*forward;
}

void resize(GLFWwindow*,int width,int height) {
    framebufferWidth=std::max(width,1); framebufferHeight=std::max(height,1);
}
void mouseButton(GLFWwindow* window,int button,int action,int) {
    if(button==GLFW_MOUSE_BUTTON_LEFT) rotating=action==GLFW_PRESS;
    if(button==GLFW_MOUSE_BUTTON_RIGHT) panning=action==GLFW_PRESS;
    glfwGetCursorPos(window,&lastMouseX,&lastMouseY);
}
void mouseMove(GLFWwindow*,double x,double y) {
    float dx=static_cast<float>(x-lastMouseX),dy=static_cast<float>(y-lastMouseY);
    Vec3 previousEye=cameraEye();
    if(rotating) {
        cameraYaw+=dx*.25f;
        cameraPitch=limit(cameraPitch-dy*.20f,walking ? -70.0f : -15.0f,walking ? 70.0f : 85.0f);
    }
    if(walking) cameraTarget=cameraTarget+previousEye-cameraEye();
    if(panning&&!walking) { float k=cameraDistance*.0018f; pan(-dx*k,dy*k); }
    lastMouseX=x; lastMouseY=y;
}
void scroll(GLFWwindow*,double,double offset) {
    if(walking) return;
    cameraDistance=limit(cameraDistance*std::pow(.9f,static_cast<float>(offset)),.5f,160.0f);
}
bool down(GLFWwindow* window,int key) { return glfwGetKey(window,key)==GLFW_PRESS; }
bool keyWasDown[GLFW_KEY_LAST+1]={};
bool pressed(GLFWwindow* window,int key) {
    bool now=down(window,key),result=now&&!keyWasDown[key];
    keyWasDown[key]=now;
    return result;
}
void input(GLFWwindow* window,float delta) {
    if(down(window,GLFW_KEY_ESCAPE)) glfwSetWindowShouldClose(window,GLFW_TRUE);
    if(pressed(window,GLFW_KEY_P)) fansRunning=!fansRunning;
    if(pressed(window,GLFW_KEY_N)) applyPreset(down(window,GLFW_KEY_LEFT_SHIFT) ? 35 : 34);   // campus gate; Shift+N: path to the curved stair
    if(pressed(window,GLFW_KEY_F10)) applyPreset(36);        // traffic on the back road
    if(pressed(window,GLFW_KEY_Y)) rayTraceRequested=true;   // ray trace the current view
    if(pressed(window,GLFW_KEY_F6)) {
        if(walking) applyPreset(0);
        else {
            buildNavigation();
            Vec3 start=cameraEye(),grounded{ENTRANCE_AXIS,1.62f,8.0f};
            bool keepLocation=!showingBaseObjects && walkNavigation.supported(start,start.y-WalkNavigation::eyeHeight,grounded);
            if(!keepLocation) {
                start={ENTRANCE_AXIS,1.62f,8.0f};
                walkNavigation.supported(start,.02f,grounded);
            }
            showingBaseObjects=false; walking=true;
            modelRotation=0; modelScale=1; modelTranslation={0,0,0};
            cameraOrthoRatio=0; cameraFov=72;
            cameraPitch=limit(cameraPitch,-65,65);
            if(!keepLocation) { cameraYaw=0; cameraPitch=0; }
            cameraDistance=1;
            cameraTarget={0,0,0}; cameraTarget=grounded-cameraEye();
            std::cout<<"Walk SWC: WASD, Shift to walk faster, arrows/left drag to look; F6/R to leave.\n";
        }
    }
    if(pressed(window,GLFW_KEY_F7)) applyPreset(18);
    if(pressed(window,GLFW_KEY_F8)) applyPreset(19);
    if(pressed(window,GLFW_KEY_F9)) {
        applyPreset(showingBaseObjects ? 0 : 20);
        modelRotation=0; modelScale=1; modelTranslation={0,0,0};
    }
    if(walking) {
        Vec3 eye=cameraEye();
        if(down(window,GLFW_KEY_LEFT)) cameraYaw-=65*delta;
        if(down(window,GLFW_KEY_RIGHT)) cameraYaw+=65*delta;
        if(down(window,GLFW_KEY_UP)) cameraPitch=limit(cameraPitch-50*delta,-70,70);
        if(down(window,GLFW_KEY_DOWN)) cameraPitch=limit(cameraPitch+50*delta,-70,70);
        cameraTarget=cameraTarget+eye-cameraEye();
        float forward=float(down(window,GLFW_KEY_W))-float(down(window,GLFW_KEY_S));
        float right=float(down(window,GLFW_KEY_D))-float(down(window,GLFW_KEY_A));
        float yaw=radians(cameraYaw);
        Vec3 displacement{right*std::cos(yaw)-forward*std::sin(yaw),0,-right*std::sin(yaw)-forward*std::cos(yaw)};
        if(length(displacement)>0) displacement=normalize(displacement)*((down(window,GLFW_KEY_LEFT_SHIFT) ? 4.2f : 2.4f)*delta);
        cameraTarget=cameraTarget+walkNavigation.move(eye,displacement)-eye;
        if(pressed(window,GLFW_KEY_R)) applyPreset(0);
        return;
    }
    if(down(window,GLFW_KEY_A)||down(window,GLFW_KEY_LEFT)) cameraYaw-=42*delta;
    if(down(window,GLFW_KEY_D)||down(window,GLFW_KEY_RIGHT)) cameraYaw+=42*delta;
    if(down(window,GLFW_KEY_W)||down(window,GLFW_KEY_UP)) cameraPitch=limit(cameraPitch+32*delta,-15,85);
    if(down(window,GLFW_KEY_S)||down(window,GLFW_KEY_DOWN)) cameraPitch=limit(cameraPitch-32*delta,-15,85);
    if(down(window,GLFW_KEY_Q)) cameraDistance=limit(cameraDistance*(1+1.2f*delta),.5f,160);
    if(down(window,GLFW_KEY_E)) cameraDistance=limit(cameraDistance*(1-1.2f*delta),.5f,160);
    float move=(down(window,GLFW_KEY_LEFT_SHIFT) ? 24.0f : 8.0f)*delta;
    if(down(window,GLFW_KEY_T)) pan(0,move);
    if(down(window,GLFW_KEY_G)) pan(0,-move);
    if(down(window,GLFW_KEY_PAGE_UP)) cameraTarget.y+=move;
    if(down(window,GLFW_KEY_PAGE_DOWN)) cameraTarget.y=std::max(0.2f,cameraTarget.y-move);
    if(down(window,GLFW_KEY_J)) modelRotation-=45*delta;
    if(down(window,GLFW_KEY_L)) modelRotation+=45*delta;
    if(down(window,GLFW_KEY_Z)) modelScale=limit(modelScale-0.5f*delta,.5f,1.5f);
    if(down(window,GLFW_KEY_X)) modelScale=limit(modelScale+0.5f*delta,.5f,1.5f);
    if(down(window,GLFW_KEY_U)) modelTranslation.x-=5.0f*delta;
    if(down(window,GLFW_KEY_O)) modelTranslation.x+=5.0f*delta;
    if(down(window,GLFW_KEY_I)) modelTranslation.z-=5.0f*delta;
    if(down(window,GLFW_KEY_K)) modelTranslation.z+=5.0f*delta;
    if(down(window,GLFW_KEY_C)) { modelRotation=0; modelScale=1; modelTranslation={0,0,0}; }
    if(pressed(window,GLFW_KEY_R)) applyPreset(0);
    if(pressed(window,GLFW_KEY_F)) { applyPreset(0); modelRotation=0; modelScale=1; modelTranslation={0,0,0}; }
    bool shift=down(window,GLFW_KEY_LEFT_SHIFT)||down(window,GLFW_KEY_RIGHT_SHIFT);
    for(int i=0;i<9;++i) if(pressed(window,GLFW_KEY_1+i)) applyPreset(shift ? 21+i : i);
    if(pressed(window,GLFW_KEY_0)) applyPreset(shift ? 30 : 9);
    if(pressed(window,GLFW_KEY_V)) applyPreset(shift ? 32 : 10);
    if(pressed(window,GLFW_KEY_H)) applyPreset(shift ? 31 : 11);
    for(int i=0;i<5;++i) if(pressed(window,GLFW_KEY_F1+i)) applyPreset(12+i);
    if(pressed(window,GLFW_KEY_B)) { if(shift) applyPreset(33); else shadowsOn=!shadowsOn; }
}

}} // namespace swc::app

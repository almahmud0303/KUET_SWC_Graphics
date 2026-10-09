#include "render/renderer.h"

#include "geometry/mesh.h"
#include "input/camera.h"
#include "render/raytracer.h"
#include "render/screenshot.h"
#include "render/shaders.h"
#include "room_navigation.h"
#include "scene/layout.h"
#include "scene/scene.h"
#include "scene/people.h"
#include <string>
#include <cstddef>
#include <cstdlib>
#include <iostream>

namespace swc { namespace app {

int runViewer(int captureView,const char* capturePath,int windowWidth,int windowHeight) {
    if(!glfwInit()) { std::cerr<<"Could not initialize GLFW.\n"; return EXIT_FAILURE; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_SAMPLES,4);
    if(capturePath) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow* window=glfwCreateWindow(windowWidth,windowHeight,"KUET Student Welfare Center - Interactive 3D View",nullptr,nullptr);
    if(!window) { glfwTerminate(); return EXIT_FAILURE; }
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    if(!load_opengl_functions(reinterpret_cast<void*(*)(const char*)>(glfwGetProcAddress))) {
        std::cerr<<"Could not load OpenGL functions.\n"; glfwDestroyWindow(window); glfwTerminate(); return EXIT_FAILURE;
    }
    glfwSetFramebufferSizeCallback(window,resize); glfwSetMouseButtonCallback(window,mouseButton);
    glfwSetCursorPosCallback(window,mouseMove); glfwSetScrollCallback(window,scroll);
    glfwGetFramebufferSize(window,&framebufferWidth,&framebufferHeight);
    glEnable(GL_DEPTH_TEST); glEnable(GL_MULTISAMPLE);

    buildScene();
    std::cout<<"SWC scene: "<<mesh.size()/3<<" triangles\n";
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,gloss)));
    glEnableVertexAttribArray(3);

    GLuint program=makeProgram(sceneVertexSource,sceneFragmentSource);
    GLuint depthProgram=makeProgram(depthVertexSource,depthFragmentSource);
    GLuint skyProgram=makeProgram(skyVertexSource,skyFragmentSource);
    GLuint imageProgram=makeProgram(skyVertexSource,imageFragmentSource);
    if(!program||!depthProgram||!skyProgram||!imageProgram) return EXIT_FAILURE;
    // Texture holding the last ray-traced frame; shown until the view changes.
    GLuint rayTexture=0; glGenTextures(1,&rayTexture); glBindTexture(GL_TEXTURE_2D,rayTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    bool showRayTraced=false; RayCamera rayCamera{}; int rayWidth=0,rayHeight=0;
    GLint viewProjectionLocation=glGetUniformLocation(program,"viewProjection");
    GLint modelLocation=glGetUniformLocation(program,"model");
    GLint normalLocation=glGetUniformLocation(program,"normalMatrix");
    GLint lightSpaceLocation=glGetUniformLocation(program,"lightSpace");
    GLint cameraLocation=glGetUniformLocation(program,"cameraPosition");
    GLint sunLocation=glGetUniformLocation(program,"sunDirection");
    GLint shadowMapLocation=glGetUniformLocation(program,"shadowMap");
    GLint shadowsLocation=glGetUniformLocation(program,"shadowsEnabled");
    GLint rolloffLocation=glGetUniformLocation(program,"highlightRolloff");
    constexpr int MAX_LIGHTS=24;
    GLint lightCountLocation=glGetUniformLocation(program,"lightCount");
    GLint lightPositionLocation[MAX_LIGHTS],lightColorLocation[MAX_LIGHTS],lightRangeLocation[MAX_LIGHTS];
    for(int i=0;i<MAX_LIGHTS;++i) {
        std::string index="["+std::to_string(i)+"]";
        lightPositionLocation[i]=glGetUniformLocation(program,("lightPositions"+index).c_str());
        lightColorLocation[i]=glGetUniformLocation(program,("lightColors"+index).c_str());
        lightRangeLocation[i]=glGetUniformLocation(program,("lightRanges"+index).c_str());
    }
    GLint depthLightLocation=glGetUniformLocation(depthProgram,"lightSpace");
    GLint depthModelLocation=glGetUniformLocation(depthProgram,"model");
    GLint skyForward=glGetUniformLocation(skyProgram,"forwardDirection");
    GLint skyRight=glGetUniformLocation(skyProgram,"rightDirection");
    GLint skyUp=glGetUniformLocation(skyProgram,"upDirection");
    GLint skyProjection=glGetUniformLocation(skyProgram,"projectionInfo");

    // Shadow map: one consistent sun from the front-right, high in the sky.
    constexpr int SHADOW_SIZE=4096;
    GLuint shadowTexture=0,shadowFramebuffer=0;
    glGenTextures(1,&shadowTexture); glBindTexture(GL_TEXTURE_2D,shadowTexture);
    glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,SHADOW_SIZE,SHADOW_SIZE,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER); glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
    const float border[4]={1,1,1,1}; glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
    glGenFramebuffers(1,&shadowFramebuffer); glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture,0);
    glDrawBuffer(GL_NONE); glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) { std::cerr<<"Shadow map unavailable.\n"; shadowsOn=false; }
    glBindFramebuffer(GL_FRAMEBUFFER,0);
    const Vec3 sun=sunDirection(),sceneCenter{6,0,-20};
    const Mat4 lightSpace=multiply(orthographic(-95,95,-95,95,1,320),lookAt(sceneCenter+sun*150.0f,sceneCenter,{0,1,0}));

    applyPreset(captureView>=0&&captureView<presetCount ? captureView : 0);
    std::cout<<"Keys 1-9: original views; 0: shops; V: cafeteria; H: connecting corridor;\n"
             "F1-F5: elevations; B: shadows. F6: walk inside/outside (WASD, drag/arrow look);\nF9: base objects; F7/F8: room views; P: pause fans; F6/R: leave walking mode.\n"
             "Shift+1..9, Shift+0, Shift+H: interior views from the photos; Y: ray trace the current view.\n";
    double previous=glfwGetTime(); int frame=0;
    std::vector<MovingInstance> movers;
    while(!glfwWindowShouldClose(window)) {
        double now=glfwGetTime(); float delta=static_cast<float>(std::min(now-previous,.05)); previous=now;
        if(!capturePath) input(window,delta);
        if(fansRunning&&!capturePath) fanAngle=std::fmod(fanAngle+delta*radians(240),2*PI);
        if(fansRunning&&!capturePath) trafficTime+=delta;   // P pauses fans, traffic and pedestrians together
        Vec3 eye=cameraEye();
        float aspect=static_cast<float>(framebufferWidth)/framebufferHeight,half=cameraOrthoRatio*cameraDistance;
        Mat4 projection=cameraOrthoRatio>0 ? orthographic(-half*aspect,half*aspect,-half,half,.15f,450.0f)
                                           : perspective(radians(cameraFov),aspect,.15f,450.0f);
        // Looking straight down, "up" on screen is the building's rear (-Z) so the front faces the viewer.
        float yaw=radians(cameraYaw);
        Vec3 up=cameraPitch>89.5f ? Vec3{-std::sin(yaw),0,-std::cos(yaw)} : Vec3{0,1,0};
        Mat4 viewProjection=multiply(projection,lookAt(eye,cameraTarget,up));
        // Composite object transformation: translation * rotation * scaling.
        Mat4 model=multiply(translation(modelTranslation),multiply(rotationY(radians(modelRotation)),scaling(modelScale)));
        Mat3 normals=normalMatrix(model);
        glBindVertexArray(vao);
        collectMovers(trafficTime,movers);

        // Y: ray trace this view on the CPU, then keep showing it until the camera or model moves.
        if(rayTraceRequested) {
            rayTraceRequested=false;
            std::cout<<"Ray tracing "<<framebufferWidth<<"x"<<framebufferHeight<<" ..."<<std::flush;
            std::vector<unsigned char> image; RayTraceOptions options; options.samples=1; options.occlusionRays=3;
            rayCamera=currentRayCamera(); rayWidth=framebufferWidth; rayHeight=framebufferHeight;
            double started=glfwGetTime();
            rayTrace(rayCamera,rayWidth,rayHeight,options,image);
            std::cout<<" done in "<<glfwGetTime()-started<<" s (move the view to return)\n";
            glBindTexture(GL_TEXTURE_2D,rayTexture); glPixelStorei(GL_UNPACK_ALIGNMENT,1);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,rayWidth,rayHeight,0,GL_RGB,GL_UNSIGNED_BYTE,image.data());
            showRayTraced=true; previous=glfwGetTime();
        }
        if(showRayTraced) {
            RayCamera now=currentRayCamera();
            bool same=length(now.eye-rayCamera.eye)+length(now.target-rayCamera.target)<1e-4f&&now.fovDegrees==rayCamera.fovDegrees
                      &&rayWidth==framebufferWidth&&rayHeight==framebufferHeight&&now.baseObjects==rayCamera.baseObjects;
            for(int i=0;i<16;++i) same=same&&now.model.m[i]==rayCamera.model.m[i];
            if(!same) showRayTraced=false;
        }
        if(showRayTraced) {
            glViewport(0,0,framebufferWidth,framebufferHeight); glDisable(GL_DEPTH_TEST);
            glUseProgram(imageProgram); glBindTexture(GL_TEXTURE_2D,rayTexture);
            glDrawArrays(GL_TRIANGLES,0,3); glEnable(GL_DEPTH_TEST);
            glfwSwapBuffers(window); glfwPollEvents();
            continue;
        }

        if(shadowsOn) {
            glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer); glViewport(0,0,SHADOW_SIZE,SHADOW_SIZE);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(depthProgram);
            glUniformMatrix4fv(depthLightLocation,1,GL_FALSE,lightSpace.m);
            glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,model.m);
            glDrawArrays(GL_TRIANGLES,showingBaseObjects ? baseObjectsFirst : 0,showingBaseObjects ? baseObjectsCount : opaqueCount);
            if(!showingBaseObjects) for(Vec3 hub:fanPositions) {
                Mat4 fan=multiply(model,multiply(translation(hub),rotationY(fanAngle)));
                glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,fan.m);
                glDrawArrays(GL_TRIANGLES,fanFirst,fanCount);
            }
            if(!showingBaseObjects) for(const MovingInstance& mover:movers) {
                Mat4 place=multiply(model,mover.transform);
                glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,place.m);
                glDrawArrays(GL_TRIANGLES,mover.first,mover.count);
            }
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }
        glViewport(0,0,framebufferWidth,framebufferHeight);
        glClearColor(.55f,.73f,.92f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        if(cameraOrthoRatio==0&&!showingBaseObjects) {
            Vec3 forward=normalize(cameraTarget-eye),right=normalize(cross(forward,up)),skyVertical=cross(right,forward);
            glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE); glUseProgram(skyProgram);
            glUniform3f(skyForward,forward.x,forward.y,forward.z);
            glUniform3f(skyRight,right.x,right.y,right.z);
            glUniform3f(skyUp,skyVertical.x,skyVertical.y,skyVertical.z);
            glUniform3f(skyProjection,aspect,std::tan(radians(cameraFov)*.5f),0);
            glDrawArrays(GL_TRIANGLES,0,3);
            glDepthMask(GL_TRUE); glEnable(GL_DEPTH_TEST);
        }
        glUseProgram(program);
        glUniformMatrix4fv(viewProjectionLocation,1,GL_FALSE,viewProjection.m);
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.m);
        glUniformMatrix3fv(normalLocation,1,GL_FALSE,normals.m);
        glUniformMatrix4fv(lightSpaceLocation,1,GL_FALSE,lightSpace.m);
        glUniform3f(cameraLocation,eye.x,eye.y,eye.z);
        glUniform3f(sunLocation,sun.x,sun.y,sun.z);
        glUniform1i(shadowMapLocation,0);
        glUniform1i(shadowsLocation,shadowsOn ? 1 : 0);
        glUniform1i(rolloffLocation,cameraOrthoRatio>0 ? 1 : 0);
        glBindTexture(GL_TEXTURE_2D,shadowTexture);
        // Point lights: only the fixtures nearest the eye are sent to the shader.
        std::sort(sceneLights.begin(),sceneLights.end(),[&](const PointLight& a,const PointLight& b) {
            return length(transformPoint(model,a.position)-eye)<length(transformPoint(model,b.position)-eye);
        });
        int lights=showingBaseObjects ? 0 : std::min(MAX_LIGHTS,static_cast<int>(sceneLights.size()));
        glUniform1i(lightCountLocation,lights);
        for(int i=0;i<lights;++i) {
            Vec3 p=transformPoint(model,sceneLights[i].position); Color c=sceneLights[i].color;
            glUniform3f(lightPositionLocation[i],p.x,p.y,p.z);
            glUniform3f(lightColorLocation[i],c.r,c.g,c.b);
            glUniform3f(lightRangeLocation[i],sceneLights[i].radius*modelScale,0,0);
        }
        glDrawArrays(GL_TRIANGLES,showingBaseObjects ? baseObjectsFirst : 0,showingBaseObjects ? baseObjectsCount : opaqueCount);
        if(!showingBaseObjects) for(Vec3 hub:fanPositions) {
            Mat4 fan=multiply(model,multiply(translation(hub),rotationY(fanAngle)));
            Mat3 fanNormals=normalMatrix(fan);
            glUniformMatrix4fv(modelLocation,1,GL_FALSE,fan.m);
            glUniformMatrix3fv(normalLocation,1,GL_FALSE,fanNormals.m);
            glDrawArrays(GL_TRIANGLES,fanFirst,fanCount);
        }
        // Traffic and pedestrians: a shared mesh per kind, drawn once per instance with M = M_scene T(p(t)) R_y.
        if(!showingBaseObjects) for(const MovingInstance& mover:movers) {
            Mat4 place=multiply(model,mover.transform);
            Mat3 placeNormals=normalMatrix(place);
            glUniformMatrix4fv(modelLocation,1,GL_FALSE,place.m);
            glUniformMatrix3fv(normalLocation,1,GL_FALSE,placeNormals.m);
            glDrawArrays(GL_TRIANGLES,mover.first,mover.count);
        }
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.m);
        glUniformMatrix3fv(normalLocation,1,GL_FALSE,normals.m);
        std::sort(roomGlass.begin(),roomGlass.end(),[&](const GlassPane& a,const GlassPane& b) {
            return length(transformPoint(model,a.center)-eye)>length(transformPoint(model,b.center)-eye);
        });
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
        if(!showingBaseObjects) for(const auto& pane:roomGlass) glDrawArrays(GL_TRIANGLES,pane.first,6);
        glDepthMask(GL_TRUE); glDisable(GL_BLEND);

        if(capturePath&&++frame==3) {
            bool ok=writeScreenshot(capturePath,framebufferWidth,framebufferHeight);
            std::cout<<(ok ? "Saved " : "Could not save ")<<capturePath<<'\n';
            glfwSetWindowShouldClose(window,GLFW_TRUE);
        }
        glfwSwapBuffers(window); glfwPollEvents();
    }
    glDeleteFramebuffers(1,&shadowFramebuffer); glDeleteTextures(1,&shadowTexture);
    glDeleteTextures(1,&rayTexture); glDeleteProgram(imageProgram);
    glDeleteProgram(skyProgram); glDeleteProgram(program); glDeleteProgram(depthProgram); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
    glfwDestroyWindow(window); glfwTerminate(); return EXIT_SUCCESS;
}

}} // namespace swc::app

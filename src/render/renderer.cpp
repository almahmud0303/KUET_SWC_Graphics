#include "render/renderer.h"

#include "geometry/mesh.h"
#include "input/camera.h"
#include "render/screenshot.h"
#include "render/shaders.h"
#include "room_navigation.h"
#include "scene/scene.h"
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
    if(!program||!depthProgram||!skyProgram) return EXIT_FAILURE;
    GLint viewProjectionLocation=glGetUniformLocation(program,"viewProjection");
    GLint modelLocation=glGetUniformLocation(program,"model");
    GLint normalLocation=glGetUniformLocation(program,"normalMatrix");
    GLint lightSpaceLocation=glGetUniformLocation(program,"lightSpace");
    GLint cameraLocation=glGetUniformLocation(program,"cameraPosition");
    GLint sunLocation=glGetUniformLocation(program,"sunDirection");
    GLint shadowMapLocation=glGetUniformLocation(program,"shadowMap");
    GLint shadowsLocation=glGetUniformLocation(program,"shadowsEnabled");
    GLint rolloffLocation=glGetUniformLocation(program,"highlightRolloff");
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
    const Vec3 sun=normalize({.42f,1.6f,.62f}),sceneCenter{6,0,-20};
    const Mat4 lightSpace=multiply(orthographic(-95,95,-95,95,1,320),lookAt(sceneCenter+sun*150.0f,sceneCenter,{0,1,0}));

    applyPreset(captureView>=0&&captureView<presetCount ? captureView : 0);
    std::cout<<"Keys 1-9: original views; 0: shops; V: cafeteria; H: connecting corridor;\n"
             "F1-F5: elevations; B: shadows. F6: walk inside/outside (WASD, drag/arrow look);\nF9: base objects; F7/F8: room views; P: pause fans; F6/R: leave walking mode.\n";
    double previous=glfwGetTime(); int frame=0;
    while(!glfwWindowShouldClose(window)) {
        double now=glfwGetTime(); float delta=static_cast<float>(std::min(now-previous,.05)); previous=now;
        if(!capturePath) input(window,delta);
        if(fansRunning&&!capturePath) fanAngle=std::fmod(fanAngle+delta*radians(240),2*PI);
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

        if(shadowsOn) {
            glBindFramebuffer(GL_FRAMEBUFFER,shadowFramebuffer); glViewport(0,0,SHADOW_SIZE,SHADOW_SIZE);
            glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(depthProgram);
            glUniformMatrix4fv(depthLightLocation,1,GL_FALSE,lightSpace.m);
            glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,model.m);
            glDrawArrays(GL_TRIANGLES,showingBaseObjects ? baseObjectsFirst : 0,showingBaseObjects ? baseObjectsCount : opaqueCount);
            if(!showingBaseObjects) for(float x:rooms::centers) {
                Mat4 fan=multiply(model,rooms::fanTransform(x,fanAngle));
                glUniformMatrix4fv(depthModelLocation,1,GL_FALSE,fan.m);
                glDrawArrays(GL_TRIANGLES,fanFirst,fanCount);
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
        glDrawArrays(GL_TRIANGLES,showingBaseObjects ? baseObjectsFirst : 0,showingBaseObjects ? baseObjectsCount : opaqueCount);
        if(!showingBaseObjects) for(float x:rooms::centers) {
            Mat4 fan=multiply(model,rooms::fanTransform(x,fanAngle));
            Mat3 fanNormals=normalMatrix(fan);
            glUniformMatrix4fv(modelLocation,1,GL_FALSE,fan.m);
            glUniformMatrix3fv(normalLocation,1,GL_FALSE,fanNormals.m);
            glDrawArrays(GL_TRIANGLES,fanFirst,fanCount);
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
    glDeleteProgram(skyProgram); glDeleteProgram(program); glDeleteProgram(depthProgram); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
    glfwDestroyWindow(window); glfwTerminate(); return EXIT_SUCCESS;
}

}} // namespace swc::app

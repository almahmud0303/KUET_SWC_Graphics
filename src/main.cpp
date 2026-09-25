#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include "gl_loader.h"

namespace {

constexpr float PI = 3.14159265358979323846f;

struct Vec3 { float x, y, z; };
struct Color { float r, g, b; };
struct Vertex { Vec3 position, normal; Color color; };
struct Mat4 { float m[16]; };

std::vector<Vertex> mesh;
float cameraYaw = 35.0f;
float cameraPitch = 20.0f;
float cameraDistance = 43.0f;
float modelRotation = 0.0f;
float modelScale = 1.0f;
bool dragging = false;
double lastMouseX = 0.0, lastMouseY = 0.0;
int framebufferWidth = 1280, framebufferHeight = 800;

Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
}
float dot(Vec3 a, Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
Vec3 normalize(Vec3 v) {
    float length=std::sqrt(dot(v,v));
    return length>0.0f ? Vec3{v.x/length,v.y/length,v.z/length} : Vec3{0,1,0};
}
float radians(float degrees) { return degrees*PI/180.0f; }
float limit(float value,float low,float high) { return std::max(low,std::min(value,high)); }

Mat4 multiply(const Mat4& a,const Mat4& b) {
    Mat4 result{};
    for(int column=0;column<4;++column)
        for(int row=0;row<4;++row)
            for(int k=0;k<4;++k)
                result.m[column*4+row]+=a.m[k*4+row]*b.m[column*4+k];
    return result;
}
Mat4 rotationY(float angle) {
    Mat4 result{}; float c=std::cos(angle),s=std::sin(angle);
    result.m[0]=c; result.m[2]=-s; result.m[5]=1.0f;
    result.m[8]=s; result.m[10]=c; result.m[15]=1.0f;
    return result;
}
Mat4 scaling(float amount) {
    Mat4 result{};
    result.m[0]=amount; result.m[5]=amount; result.m[10]=amount; result.m[15]=1.0f;
    return result;
}
Mat4 perspective(float fov,float aspect,float nearPlane,float farPlane) {
    Mat4 result{};
    float f=1.0f/std::tan(fov*0.5f);
    result.m[0]=f/aspect; result.m[5]=f;
    result.m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);
    result.m[11]=-1.0f;
    result.m[14]=(2.0f*farPlane*nearPlane)/(nearPlane-farPlane);
    return result;
}
Mat4 lookAt(Vec3 eye,Vec3 target,Vec3 up) {
    Vec3 forward=normalize(target-eye);
    Vec3 side=normalize(cross(forward,up));
    Vec3 correctedUp=cross(side,forward);
    Mat4 result{};
    result.m[0]=side.x; result.m[1]=correctedUp.x; result.m[2]=-forward.x;
    result.m[4]=side.y; result.m[5]=correctedUp.y; result.m[6]=-forward.y;
    result.m[8]=side.z; result.m[9]=correctedUp.z; result.m[10]=-forward.z;
    result.m[12]=-dot(side,eye); result.m[13]=-dot(correctedUp,eye);
    result.m[14]=dot(forward,eye); result.m[15]=1.0f;
    return result;
}

void addTriangle(Vec3 a,Vec3 b,Vec3 c,Vec3 normal,Color color) {
    mesh.push_back({a,normal,color}); mesh.push_back({b,normal,color}); mesh.push_back({c,normal,color});
}
void addQuad(Vec3 a,Vec3 b,Vec3 c,Vec3 d,Vec3 normal,Color color) {
    addTriangle(a,b,c,normal,color); addTriangle(a,c,d,normal,color);
}
void box(float x,float y,float z,float width,float height,float depth,Color color) {
    float x0=x-width/2,x1=x+width/2,y0=y-height/2,y1=y+height/2,z0=z-depth/2,z1=z+depth/2;
    addQuad({x0,y0,z1},{x1,y0,z1},{x1,y1,z1},{x0,y1,z1},{0,0,1},color);
    addQuad({x1,y0,z0},{x0,y0,z0},{x0,y1,z0},{x1,y1,z0},{0,0,-1},color);
    addQuad({x1,y0,z1},{x1,y0,z0},{x1,y1,z0},{x1,y1,z1},{1,0,0},color);
    addQuad({x0,y0,z0},{x0,y0,z1},{x0,y1,z1},{x0,y1,z0},{-1,0,0},color);
    addQuad({x0,y1,z1},{x1,y1,z1},{x1,y1,z0},{x0,y1,z0},{0,1,0},color);
    addQuad({x0,y0,z0},{x1,y0,z0},{x1,y0,z1},{x0,y0,z1},{0,-1,0},color);
}
void cylinder(float x,float y,float z,float rx,float height,float rz,Color color,int segments=36) {
    float bottom=y-height/2,top=y+height/2;
    for(int i=0;i<segments;++i) {
        float a=2*PI*i/segments,b=2*PI*(i+1)/segments;
        Vec3 p0{x+rx*std::cos(a),bottom,z+rz*std::sin(a)};
        Vec3 p1{x+rx*std::cos(b),bottom,z+rz*std::sin(b)};
        Vec3 p2{x+rx*std::cos(b),top,z+rz*std::sin(b)};
        Vec3 p3{x+rx*std::cos(a),top,z+rz*std::sin(a)};
        Vec3 normal=normalize({std::cos((a+b)/2)/rx,0,std::sin((a+b)/2)/rz});
        addQuad(p1,p0,p3,p2,normal,color);
        addTriangle({x,top,z},p2,p3,{0,1,0},color);
        addTriangle({x,bottom,z},p0,p1,{0,-1,0},color);
    }
}
void sphere(float x,float y,float z,float radius,Color color,int rings=8,int slices=14) {
    for(int r=0;r<rings;++r) {
        float p0=-PI/2+PI*r/rings,p1=-PI/2+PI*(r+1)/rings;
        for(int s=0;s<slices;++s) {
            float a0=2*PI*s/slices,a1=2*PI*(s+1)/slices;
            Vec3 n00{std::cos(p0)*std::cos(a0),std::sin(p0),std::cos(p0)*std::sin(a0)};
            Vec3 n01{std::cos(p0)*std::cos(a1),std::sin(p0),std::cos(p0)*std::sin(a1)};
            Vec3 n11{std::cos(p1)*std::cos(a1),std::sin(p1),std::cos(p1)*std::sin(a1)};
            Vec3 n10{std::cos(p1)*std::cos(a0),std::sin(p1),std::cos(p1)*std::sin(a0)};
            Vec3 p00{x+n00.x*radius,y+n00.y*radius,z+n00.z*radius};
            Vec3 p01{x+n01.x*radius,y+n01.y*radius,z+n01.z*radius};
            Vec3 p11{x+n11.x*radius,y+n11.y*radius,z+n11.z*radius};
            Vec3 p10{x+n10.x*radius,y+n10.y*radius,z+n10.z*radius};
            mesh.push_back({p00,n00,color}); mesh.push_back({p01,n01,color}); mesh.push_back({p11,n11,color});
            mesh.push_back({p00,n00,color}); mesh.push_back({p11,n11,color}); mesh.push_back({p10,n10,color});
        }
    }
}
void annulus(float x,float y,float z,float inner,float outer,Color color) {
    constexpr int n=64;
    for(int i=0;i<n;++i) {
        float a=2*PI*i/n,b=2*PI*(i+1)/n;
        addQuad({x+inner*std::cos(a),y,z+inner*std::sin(a)},{x+outer*std::cos(a),y,z+outer*std::sin(a)},
                {x+outer*std::cos(b),y,z+outer*std::sin(b)},{x+inner*std::cos(b),y,z+inner*std::sin(b)},
                {0,1,0},color);
    }
}

const Color white{0.86f,0.89f,0.85f}, brightWhite{0.96f,0.97f,0.93f};
const Color brick{0.52f,0.20f,0.13f}, brickLight{0.62f,0.29f,0.20f};
const Color glass{0.12f,0.25f,0.28f}, frame{0.76f,0.80f,0.76f};
const Color concrete{0.70f,0.69f,0.64f}, grass{0.16f,0.48f,0.16f};

void frontWindow(float x,float y,float z,float w=1.35f,float h=1.55f) {
    box(x,y,z,w,h,.10f,glass); box(x,y-h*.43f,z+.07f,w,.20f,.12f,brickLight);
    box(x,y,z+.12f,.07f,h,.05f,frame); box(x,y,z+.12f,w,.06f,.05f,frame);
    box(x,y+h*.38f,z+.12f,w,.06f,.05f,frame);
}
void sideWindow(float x,float y,float z,float w=1.35f,float h=1.50f) {
    box(x,y,z,.10f,h,w,glass); box(x+.07f,y-h*.43f,z,.12f,.20f,w,brickLight);
    box(x+.12f,y,z,.05f,h,.07f,frame); box(x+.12f,y,z,.05f,.06f,w,frame);
}
void tree(float x,float z,float scale=1.0f) {
    cylinder(x,1.35f*scale,z,.16f*scale,2.7f*scale,.16f*scale,{.28f,.16f,.08f},12);
    sphere(x,3.0f*scale,z,.85f*scale,{.08f,.34f,.10f});
    sphere(x-.55f*scale,2.75f*scale,z,.60f*scale,{.10f,.42f,.12f});
    sphere(x+.50f*scale,2.82f*scale,z+.18f*scale,.65f*scale,{.07f,.38f,.10f});
}
void shrub(float x,float z,Color color={.10f,.42f,.10f}) {
    cylinder(x,.35f,z,.08f,.7f,.08f,{.25f,.14f,.06f},8); sphere(x,.85f,z,.55f,color,6,12);
}
void steps(float x,float z,float width) {
    for(int i=0;i<4;++i) box(x,.08f+i*.10f,z-i*.18f,width-i*.25f,.16f,.38f,brightWhite);
}

const char* glyph(char c) {
    switch(c) {
    case 'A': return "0111010001111111000110001";
    case 'C': return "0111110000100001000001111";
    case 'D': return "1111010001100011000111110";
    case 'E': return "1111110000111101000011111";
    case 'F': return "1111110000111101000010000";
    case 'K': return "1000110010111001001010001";
    case 'L': return "1000010000100001000011111";
    case 'N': return "1000111001101011001110001";
    case 'R': return "1111010001111101010010010";
    case 'S': return "0111110000011100000111110";
    case 'T': return "1111100100001000010000100";
    case 'U': return "1000110001100011000101110";
    case 'W': return "1000110001101011101110001";
    default: return "0000000000000000000000000";
    }
}
void textOnFront(const std::string& text,float centerX,float y,float z,float scale) {
    float total=(text.size()*6-1)*scale,start=centerX-total/2;
    for(std::size_t k=0;k<text.size();++k) {
        const char* bits=glyph(text[k]);
        for(int row=0;row<5;++row) for(int col=0;col<5;++col)
            if(bits[row*5+col]=='1')
                box(start+(k*6+col)*scale,y+(4-row)*scale,z,scale*.82f,scale*.82f,.09f,brightWhite);
    }
}

void buildScene() {
    // Site, front plaza, driveway, and courtyard lawn.
    box(0,-.22f,-5,42,.40f,42,grass);
    box(0,.01f,8,30,.16f,12,{.55f,.28f,.20f});
    for(int i=-6;i<=6;++i) box(i*2.0f,.105f,8.0f,.07f,.02f,12,{.70f,.45f,.32f});
    box(0,.03f,-10,18,.15f,12,{.20f,.55f,.20f});
    annulus(0,.12f,9,0,1.2f,{.76f,.62f,.19f}); annulus(0,.125f,9,1.2f,2.1f,{.28f,.52f,.28f});
    annulus(0,.13f,9,2.1f,3.0f,{.66f,.28f,.22f}); annulus(0,.135f,9,3.0f,3.25f,brightWhite);

    // Front-left classroom block and its deep roof overhang.
    box(-9,3.35f,-1.7f,10,6.3f,4.4f,white);
    box(-9,6.65f,.15f,11.0f,.38f,7.1f,brightWhite);
    for(int floor=0;floor<2;++floor) for(int i=0;i<5;++i)
        frontWindow(-13.0f+i*2.05f,1.65f+floor*2.75f,.56f,1.45f,1.45f);
    for(float x : {-13.6f,-10.8f,-8.0f,-5.2f}) box(x,3.35f,1.65f,.38f,6.5f,.45f,brightWhite);

    // Monumental entrance canopy, columns, brick lobby, doors, and stairs.
    box(1.0f,6.65f,.25f,11.5f,.42f,7.4f,brightWhite);
    box(1.0f,3.0f,-2.7f,9.0f,5.7f,.45f,brick);
    box(1.0f,2.0f,-2.38f,3.4f,3.4f,.18f,{.20f,.12f,.09f});
    for(float x : {-3.9f,-1.2f,1.6f,4.3f,6.0f}) box(x,3.25f,1.9f,.38f,6.5f,.38f,brightWhite);
    steps(1.0f,4.15f,10.5f);

    // Distinctive curved red-brick SWC volume and white horizontal bands.
    cylinder(10.4f,3.55f,-.7f,4.25f,7.1f,4.1f,brick,52);
    cylinder(10.4f,2.05f,-.7f,4.29f,.16f,4.14f,brightWhite,52);
    cylinder(10.4f,4.90f,-.7f,4.29f,.16f,4.14f,brightWhite,52);
    box(7.75f,3.55f,3.10f,.15f,5.2f,.18f,brightWhite);
    box(13.05f,3.55f,3.10f,.15f,5.2f,.18f,brightWhite);
    textOnFront("STUDENT WELFARE",10.4f,4.25f,3.43f,.075f);
    textOnFront("CENTER KUET",10.4f,3.65f,3.44f,.085f);

    // Courtyard side wings.
    box(-12,3.25f,-11.0f,4.0f,6.1f,16.0f,white);
    box(12,3.25f,-11.0f,4.0f,6.1f,16.0f,white);
    box(-9.75f,6.5f,-11,1.0f,.35f,17.0f,brightWhite);
    box(9.75f,6.5f,-11,1.0f,.35f,17.0f,brightWhite);
    for(int floor=0;floor<2;++floor) for(int i=0;i<6;++i) {
        sideWindow(-9.93f,1.55f+floor*2.75f,-5.4f-i*2.35f);
        sideWindow(9.93f,1.55f+floor*2.75f,-5.4f-i*2.35f);
    }
    for(int i=0;i<7;++i) {
        box(-9.35f,3.2f,-4.3f-i*2.35f,.30f,6.2f,.30f,brightWhite);
        box(9.35f,3.2f,-4.3f-i*2.35f,.30f,6.2f,.30f,brightWhite);
    }
    // Gallery floors and brick-filled railings.
    box(-9.1f,3.15f,-11,.55f,.22f,16,brightWhite); box(9.1f,3.15f,-11,.55f,.22f,16,brightWhite);
    box(-9.15f,3.85f,-11,.18f,.75f,15.2f,brickLight); box(9.15f,3.85f,-11,.18f,.75f,15.2f,brickLight);

    // Rear wing enclosing the U-shaped court.
    box(0,3.2f,-19,24,6.0f,3.8f,white); box(0,6.4f,-18.6f,25,.36f,5,brightWhite);
    for(int floor=0;floor<2;++floor) for(int i=0;i<9;++i)
        frontWindow(-8.8f+i*2.2f,1.5f+floor*2.7f,-16.98f,1.4f,1.35f);
    for(int i=0;i<10;++i) box(-10.5f+i*2.3f,3.2f,-16.55f,.30f,6.1f,.35f,brightWhite);

    // Brick outdoor stage photographed inside the courtyard.
    box(0,.35f,-14.7f,8.0f,.7f,2.4f,brickLight);
    box(0,2.15f,-15.65f,7.0f,3.6f,.45f,brick);
    box(0,.25f,-13.15f,9.2f,.5f,.8f,brickLight);

    // Window AC boxes and vents on the outer frontage.
    for(float x : {-11.8f,-9.3f,-6.8f}) {
        box(x,3.85f,.78f,.70f,.62f,.35f,{.67f,.68f,.62f});
        cylinder(x,3.85f,1.0f,.22f,.05f,.22f,{.20f,.22f,.20f},16);
    }

    // Landscaping based on the photos.
    tree(-16,1.8f,1.15f); tree(16,1.5f,1.25f); tree(-15,-8,1.0f); tree(15,-11,1.05f);
    tree(-6,-10,.85f); tree(6,-12,.90f); tree(-3,-7,.70f);
    for(float x=-13;x<=13;x+=2.6f) shrub(x,4.1f,(static_cast<int>(x)&1)?Color{.34f,.16f,.08f}:Color{.08f,.38f,.10f});
    shrub(-7,-7); shrub(7,-8); shrub(-6,-14,{.42f,.12f,.08f}); shrub(6,-14,{.42f,.12f,.08f});
}

GLuint compileShader(GLenum type,const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=GL_FALSE; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
        std::cerr<<"Shader compilation failed:\n"<<log<<'\n'; glDeleteShader(shader); return 0; }
    return shader;
}
GLuint makeProgram() {
    const char* vertexSource=R"(
        #version 330 core
        layout(location=0) in vec3 position;
        layout(location=1) in vec3 normal;
        layout(location=2) in vec3 color;
        uniform mat4 viewProjection;
        uniform mat4 model;
        out vec3 surfaceNormal;
        out vec3 surfaceColor;
        out vec3 worldPosition;
        void main(){
            vec4 world=model*vec4(position,1.0);
            gl_Position=viewProjection*world;
            surfaceNormal=mat3(model)*normal;
            surfaceColor=color;
            worldPosition=world.xyz;
        }
    )";
    const char* fragmentSource=R"(
        #version 330 core
        in vec3 surfaceNormal;
        in vec3 surfaceColor;
        in vec3 worldPosition;
        uniform vec3 cameraPosition;
        out vec4 pixel;
        void main(){
            vec3 n=normalize(surfaceNormal);
            vec3 light=normalize(vec3(0.45,1.0,0.65));
            float ambient=0.28;
            float diffuse=max(dot(n,light),0.0);
            vec3 viewDirection=normalize(cameraPosition-worldPosition);
            vec3 reflection=reflect(-light,n);
            float specular=pow(max(dot(viewDirection,reflection),0.0),32.0)*0.35;
            vec3 phongColor=surfaceColor*(ambient+0.72*diffuse)+vec3(specular);
            pixel=vec4(phongColor,1.0);
        }
    )";
    GLuint vertex=compileShader(GL_VERTEX_SHADER,vertexSource),fragment=compileShader(GL_FRAGMENT_SHADER,fragmentSource);
    if(!vertex||!fragment) return 0;
    GLuint result=glCreateProgram(); glAttachShader(result,vertex); glAttachShader(result,fragment); glLinkProgram(result);
    glDeleteShader(vertex); glDeleteShader(fragment); GLint ok=GL_FALSE; glGetProgramiv(result,GL_LINK_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetProgramInfoLog(result,sizeof(log),nullptr,log);
        std::cerr<<"Shader linking failed:\n"<<log<<'\n'; glDeleteProgram(result); return 0; }
    return result;
}

void resize(GLFWwindow*,int width,int height) {
    framebufferWidth=std::max(width,1); framebufferHeight=std::max(height,1);
    glViewport(0,0,framebufferWidth,framebufferHeight);
}
void mouseButton(GLFWwindow* window,int button,int action,int) {
    if(button==GLFW_MOUSE_BUTTON_LEFT) {
        dragging=action==GLFW_PRESS;
        glfwGetCursorPos(window,&lastMouseX,&lastMouseY);
    }
}
void mouseMove(GLFWwindow*,double x,double y) {
    if(dragging) {
        cameraYaw+=static_cast<float>(x-lastMouseX)*.25f;
        cameraPitch=limit(cameraPitch-static_cast<float>(y-lastMouseY)*.20f,-10.0f,75.0f);
    }
    lastMouseX=x; lastMouseY=y;
}
void scroll(GLFWwindow*,double,double offset) { cameraDistance=limit(cameraDistance-static_cast<float>(offset)*2.0f,18.0f,75.0f); }
void input(GLFWwindow* window,float delta) {
    if(glfwGetKey(window,GLFW_KEY_ESCAPE)==GLFW_PRESS) glfwSetWindowShouldClose(window,GLFW_TRUE);
    if(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS) cameraYaw-=42*delta;
    if(glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS) cameraYaw+=42*delta;
    if(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS) cameraPitch=limit(cameraPitch+32*delta,-10,75);
    if(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS||glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS) cameraPitch=limit(cameraPitch-32*delta,-10,75);
    if(glfwGetKey(window,GLFW_KEY_Q)==GLFW_PRESS) cameraDistance=limit(cameraDistance+18*delta,18,75);
    if(glfwGetKey(window,GLFW_KEY_E)==GLFW_PRESS) cameraDistance=limit(cameraDistance-18*delta,18,75);
    if(glfwGetKey(window,GLFW_KEY_J)==GLFW_PRESS) modelRotation-=45*delta;
    if(glfwGetKey(window,GLFW_KEY_L)==GLFW_PRESS) modelRotation+=45*delta;
    if(glfwGetKey(window,GLFW_KEY_Z)==GLFW_PRESS) modelScale=limit(modelScale-0.5f*delta,.65f,1.35f);
    if(glfwGetKey(window,GLFW_KEY_X)==GLFW_PRESS) modelScale=limit(modelScale+0.5f*delta,.65f,1.35f);
    if(glfwGetKey(window,GLFW_KEY_C)==GLFW_PRESS) { modelRotation=0; modelScale=1; }
    if(glfwGetKey(window,GLFW_KEY_R)==GLFW_PRESS) { cameraYaw=35; cameraPitch=20; cameraDistance=43; }
}

} // namespace

int main() {
    if(!glfwInit()) { std::cerr<<"Could not initialize GLFW.\n"; return EXIT_FAILURE; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3); glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE); glfwWindowHint(GLFW_SAMPLES,4);
    GLFWwindow* window=glfwCreateWindow(1280,800,"KUET Student Welfare Centre - Interactive 3D View",nullptr,nullptr);
    if(!window) { glfwTerminate(); return EXIT_FAILURE; }
    glfwMakeContextCurrent(window); glfwSwapInterval(1);
    if(!load_opengl_functions(reinterpret_cast<void*(*)(const char*)>(glfwGetProcAddress))) {
        std::cerr<<"Could not load OpenGL functions.\n"; glfwDestroyWindow(window); glfwTerminate(); return EXIT_FAILURE;
    }
    glfwSetFramebufferSizeCallback(window,resize); glfwSetMouseButtonCallback(window,mouseButton);
    glfwSetCursorPosCallback(window,mouseMove); glfwSetScrollCallback(window,scroll);
    glfwGetFramebufferSize(window,&framebufferWidth,&framebufferHeight); resize(window,framebufferWidth,framebufferHeight);
    glEnable(GL_DEPTH_TEST);

    buildScene();
    GLuint vao=0,vbo=0; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER,vbo);
    glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(mesh.size()*sizeof(Vertex)),mesh.data(),GL_STATIC_DRAW);
    glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,position)));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,normal)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,color)));
    glEnableVertexAttribArray(2);
    GLuint program=makeProgram(); if(!program) return EXIT_FAILURE;
    GLint matrixLocation=glGetUniformLocation(program,"viewProjection");
    GLint modelLocation=glGetUniformLocation(program,"model");
    GLint cameraLocation=glGetUniformLocation(program,"cameraPosition");

    double previous=glfwGetTime();
    while(!glfwWindowShouldClose(window)) {
        double now=glfwGetTime(); float delta=static_cast<float>(std::min(now-previous,.05)); previous=now;
        input(window,delta);
        float yaw=radians(cameraYaw),pitch=radians(cameraPitch);
        Vec3 target{0,2.7f,-5.0f};
        Vec3 eye{target.x+cameraDistance*std::cos(pitch)*std::sin(yaw),
                 target.y+cameraDistance*std::sin(pitch),
                 target.z+cameraDistance*std::cos(pitch)*std::cos(yaw)};
        Mat4 projection=perspective(radians(48.0f),static_cast<float>(framebufferWidth)/framebufferHeight,.1f,160.0f);
        Mat4 view=lookAt(eye,target,{0,1,0}); Mat4 viewProjection=multiply(projection,view);
        Mat4 model=multiply(rotationY(radians(modelRotation)),scaling(modelScale));

        glClearColor(.43f,.70f,.91f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(program); glUniformMatrix4fv(matrixLocation,1,GL_FALSE,viewProjection.m);
        glUniformMatrix4fv(modelLocation,1,GL_FALSE,model.m);
        glUniform3f(cameraLocation,eye.x,eye.y,eye.z);
        glBindVertexArray(vao); glDrawArrays(GL_TRIANGLES,0,static_cast<GLsizei>(mesh.size()));
        glfwSwapBuffers(window); glfwPollEvents();
    }
    glDeleteProgram(program); glDeleteBuffers(1,&vbo); glDeleteVertexArrays(1,&vao);
    glfwDestroyWindow(window); glfwTerminate(); return EXIT_SUCCESS;
}

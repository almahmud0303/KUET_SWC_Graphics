#pragma once

#include <algorithm>
#include <cmath>

// Shared column-major math for canonical geometry and the scene.
namespace swc {

constexpr float PI = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Custom vector and matrix operations (column-major, OpenGL convention)
// ---------------------------------------------------------------------------
struct Vec3 { float x, y, z; };
struct Color { float r, g, b; };
struct Mat4 { float m[16]; };
struct Mat3 { float m[9]; };

inline Vec3 operator+(Vec3 a,Vec3 b) { return {a.x+b.x,a.y+b.y,a.z+b.z}; }
inline Vec3 operator-(Vec3 a,Vec3 b) { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
inline Vec3 operator*(Vec3 a,float s) { return {a.x*s,a.y*s,a.z*s}; }
inline Vec3 cross(Vec3 a,Vec3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline float dot(Vec3 a,Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline float length(Vec3 v) { return std::sqrt(dot(v,v)); }
inline Vec3 normalize(Vec3 v) { float l=length(v); return l>1e-8f ? v*(1.0f/l) : Vec3{0,1,0}; }
inline Vec3 lerp(Vec3 a,Vec3 b,float t) { return a+(b-a)*t; }
inline float radians(float degrees) { return degrees*PI/180.0f; }
inline float limit(float value,float low,float high) { return std::max(low,std::min(value,high)); }
inline Color mix(Color a,Color b,float t) { return {a.r+(b.r-a.r)*t,a.g+(b.g-a.g)*t,a.b+(b.b-a.b)*t}; }
inline float hash1(float n) { float s=std::sin(n*12.9898f+78.233f)*43758.5453f; return s-std::floor(s); }

inline Mat4 identity() { Mat4 r{}; r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f; return r; }
inline Mat4 multiply(const Mat4& a,const Mat4& b) {
    Mat4 result{};
    for(int column=0;column<4;++column)
        for(int row=0;row<4;++row)
            for(int k=0;k<4;++k)
                result.m[column*4+row]+=a.m[k*4+row]*b.m[column*4+k];
    return result;
}
inline Mat4 translation(Vec3 offset) {
    Mat4 r=identity(); r.m[12]=offset.x; r.m[13]=offset.y; r.m[14]=offset.z; return r;
}
inline Mat4 rotationX(float angle) {
    Mat4 r=identity(); float c=std::cos(angle),s=std::sin(angle);
    r.m[5]=c; r.m[6]=s; r.m[9]=-s; r.m[10]=c; return r;
}
inline Mat4 rotationY(float angle) {
    Mat4 r=identity(); float c=std::cos(angle),s=std::sin(angle);
    r.m[0]=c; r.m[2]=-s; r.m[8]=s; r.m[10]=c; return r;
}
inline Mat4 scaling(Vec3 amount) {
    Mat4 r=identity(); r.m[0]=amount.x; r.m[5]=amount.y; r.m[10]=amount.z; return r;
}
inline Mat4 scaling(float amount) { return scaling(Vec3{amount,amount,amount}); }
// Composite local transform T * R * S: scale first, then yaw about +Y, then translate.
inline Mat4 trs(Vec3 position,float yawDegrees,Vec3 scale={1,1,1}) {
    return multiply(translation(position),multiply(rotationY(radians(yawDegrees)),scaling(scale)));
}
inline Mat4 perspective(float fov,float aspect,float nearPlane,float farPlane) {
    Mat4 result{};
    float f=1.0f/std::tan(fov*0.5f);
    result.m[0]=f/aspect; result.m[5]=f;
    result.m[10]=(farPlane+nearPlane)/(nearPlane-farPlane);
    result.m[11]=-1.0f;
    result.m[14]=(2.0f*farPlane*nearPlane)/(nearPlane-farPlane);
    return result;
}
inline Mat4 orthographic(float left,float right,float bottom,float top,float nearPlane,float farPlane) {
    Mat4 r=identity();
    r.m[0]=2.0f/(right-left); r.m[5]=2.0f/(top-bottom); r.m[10]=-2.0f/(farPlane-nearPlane);
    r.m[12]=-(right+left)/(right-left); r.m[13]=-(top+bottom)/(top-bottom);
    r.m[14]=-(farPlane+nearPlane)/(farPlane-nearPlane);
    return r;
}
inline Mat4 lookAt(Vec3 eye,Vec3 target,Vec3 up) {
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
inline Vec3 transformPoint(const Mat4& m,Vec3 p) {
    return {m.m[0]*p.x+m.m[4]*p.y+m.m[8]*p.z+m.m[12],
            m.m[1]*p.x+m.m[5]*p.y+m.m[9]*p.z+m.m[13],
            m.m[2]*p.x+m.m[6]*p.y+m.m[10]*p.z+m.m[14]};
}
// Inverse-transpose of the upper 3x3, so normals stay perpendicular under non-uniform scaling.
inline Mat3 normalMatrix(const Mat4& m) {
    float a=m.m[0],b=m.m[4],c=m.m[8],d=m.m[1],e=m.m[5],f=m.m[9],g=m.m[2],h=m.m[6],i=m.m[10];
    float c00=e*i-f*h,c01=-(d*i-f*g),c02=d*h-e*g;
    float c10=-(b*i-c*h),c11=a*i-c*g,c12=-(a*h-b*g);
    float c20=b*f-c*e,c21=-(a*f-c*d),c22=a*e-b*d;
    float det=a*c00+b*c01+c*c02; float k=std::fabs(det)>1e-12f ? 1.0f/det : 0.0f;
    Mat3 n{};
    n.m[0]=c00*k; n.m[3]=c01*k; n.m[6]=c02*k;
    n.m[1]=c10*k; n.m[4]=c11*k; n.m[7]=c12*k;
    n.m[2]=c20*k; n.m[5]=c21*k; n.m[8]=c22*k;
    return n;
}
inline Vec3 transformNormal(const Mat3& n,Vec3 v) {
    return {n.m[0]*v.x+n.m[3]*v.y+n.m[6]*v.z,n.m[1]*v.x+n.m[4]*v.y+n.m[7]*v.z,n.m[2]*v.x+n.m[5]*v.y+n.m[8]*v.z};
}

} // namespace swc

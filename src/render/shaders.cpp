#include "render/shaders.h"

#include <iostream>

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Shaders: per-fragment Phong lighting (ambient + diffuse + specular) with
// shadow mapping and procedural surface detail.
// ---------------------------------------------------------------------------
const char* sceneVertexSource=R"(
    #version 330 core
    layout(location=0) in vec3 position;
    layout(location=1) in vec3 normal;
    layout(location=2) in vec3 color;
    layout(location=3) in vec2 material;
    uniform mat4 viewProjection;
    uniform mat4 model;
    uniform mat3 normalMatrix;
    uniform mat4 lightSpace;
    out vec3 surfaceNormal;
    out vec3 surfaceColor;
    out vec3 worldPosition;
    out vec2 surfaceMaterial;
    out vec4 lightPosition;
    void main(){
        vec4 world=model*vec4(position,1.0);
        gl_Position=viewProjection*world;
        surfaceNormal=normalMatrix*normal;
        surfaceColor=color;
        surfaceMaterial=material;
        worldPosition=world.xyz;
        lightPosition=lightSpace*vec4(world.xyz+normalize(surfaceNormal)*0.05,1.0);
    }
)";
const char* sceneFragmentSource=R"(
    #version 330 core
    in vec3 surfaceNormal;
    in vec3 surfaceColor;
    in vec3 worldPosition;
    in vec2 surfaceMaterial;
    in vec4 lightPosition;
    uniform vec3 cameraPosition;
    uniform vec3 sunDirection;
    uniform sampler2D shadowMap;
    uniform int shadowsEnabled;
    uniform int highlightRolloff;
    // Nearest ceiling fixtures: position, colour and (radius,0,0).
    const int MAX_LIGHTS=24;
    uniform int lightCount;
    uniform vec3 lightPositions[MAX_LIGHTS];
    uniform vec3 lightColors[MAX_LIGHTS];
    uniform vec3 lightRanges[MAX_LIGHTS];
    out vec4 pixel;

    const vec3 sunColor=vec3(1.0,0.96,0.88);
    const vec3 skyColor=vec3(0.62,0.74,0.90);
    const vec3 groundBounce=vec3(0.50,0.50,0.44);
    const vec3 fogColor=vec3(0.74,0.84,0.94);

    float hash(vec2 p){
        uvec2 q=uvec2(ivec2(p)); uint h=q.x*1597334677u ^ q.y*3812015801u;
        h=(h^(h>>16))*2246822519u; h^=h>>13;
        return float(h&16777215u)/16777215.0;
    }
    float noise(vec2 p){
        vec2 i=floor(p),f=fract(p); f=f*f*(3.0-2.0*f);
        return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);
    }
    // Running-bond units with mortar joints, faded out where they would alias.
    vec3 bonded(vec3 base,vec2 uv,vec2 size,vec3 joint,float jointWidth,float variation){
        float row=floor(uv.y/size.y);
        vec2 cell=vec2(uv.x/size.x+0.5*mod(row,2.0),uv.y/size.y);
        vec2 f=fract(cell)*size;
        float mortar=max(step(f.x,jointWidth),step(f.y,jointWidth));
        vec3 unit=base*(1.0-variation*0.5+variation*hash(floor(cell)));
        float detail=1.0-smoothstep(0.35,0.8,max(fwidth(cell.x),fwidth(cell.y)));
        return mix(mix(base,joint,0.12),mix(unit,joint,mortar),detail);
    }
    // Square floor tiles on a straight grid (no running-bond offset).
    vec3 gridded(vec3 base,vec2 uv,float size,vec3 joint,float jointWidth){
        vec2 cell=uv/size,f=fract(cell)*size;
        float mortar=max(step(f.x,jointWidth),step(f.y,jointWidth));
        float detail=1.0-smoothstep(0.35,0.8,max(fwidth(cell.x),fwidth(cell.y)));
        return mix(mix(base,joint,0.04),mix(base,joint,mortar),detail);
    }
    // Point lights with constant/linear/quadratic attenuation and a smooth
    // cut-off radius. Each fixture lights only the storey it hangs in.
    void pointLights(vec3 n,vec3 viewDirection,float shininess,out vec3 diffuse,out vec3 specular){
        diffuse=vec3(0.0); specular=vec3(0.0);
        for(int i=0;i<lightCount;++i){
            vec3 toLight=lightPositions[i]-worldPosition;
            float d=length(toLight),r=lightRanges[i].x;
            if(d>r||toLight.y<-0.45||toLight.y>4.3) continue;
            vec3 l=toLight/d;
            float window=1.0-pow(d/r,4.0);
            // Fade out one storey below the fixture instead of cutting off sharply.
            float storey=1.0-smoothstep(3.3,4.3,toLight.y);
            float attenuation=storey*window*window/(1.0+0.22*d+0.20*d*d);
            diffuse+=lightColors[i]*attenuation*(0.25+0.75*max(dot(n,l),0.0));
            specular+=lightColors[i]*attenuation*pow(max(dot(viewDirection,reflect(-l,n)),0.0),shininess);
        }
    }
    float shadowFactor(vec3 n){
        if(shadowsEnabled==0) return 1.0;
        vec3 p=lightPosition.xyz/lightPosition.w*0.5+0.5;
        if(p.z>1.0||p.x<0.0||p.x>1.0||p.y<0.0||p.y>1.0) return 1.0;
        float bias=0.0006+0.0012*(1.0-max(dot(n,sunDirection),0.0));
        vec2 texel=1.0/vec2(textureSize(shadowMap,0));
        float lit=0.0;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y)
            lit+=p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r ? 0.0 : 1.0;
        return lit/9.0;
    }
    void main(){
        vec3 n=normalize(surfaceNormal);
        vec3 viewDirection=normalize(cameraPosition-worldPosition);
        if(dot(n,viewDirection)<0.0) n=-n;                    // two-sided leaves, glass and thin panels
        vec3 base=surfaceColor;
        float gloss=surfaceMaterial.x;
        int pattern=int(surfaceMaterial.y+0.5);
        vec3 tangent=abs(n.y)>0.9 ? vec3(1,0,0) : normalize(cross(vec3(0,1,0),n));
        vec2 wallUV=vec2(dot(worldPosition,tangent),abs(n.y)>0.9 ? worldPosition.z : worldPosition.y);
        if(pattern==1) base=bonded(base,wallUV,vec2(0.24,0.075),vec3(0.56,0.49,0.39),0.006,0.25);
        else if(pattern==3) base*=0.82+0.36*noise(worldPosition.xz*1.7)*(0.7+0.3*noise(worldPosition.xz*9.0));
        else if(pattern==4) base=bonded(base,worldPosition.xz,vec2(0.22,0.11),base*0.72,0.012,0.35);
        else if(pattern==5) base*=0.72+0.5*noise(vec2((worldPosition.x+worldPosition.z)*14.0,worldPosition.y*2.5));
        else if(pattern==9) {
            float patches=noise(worldPosition.xz*.65);
            float blades=noise(worldPosition.xz*18.0);
            float detail=1.0-smoothstep(.25,1.0,length(fwidth(worldPosition.xz*18.0)));
            base*=.94+.12*patches;
            base*=1.0+detail*(blades-.5)*.65;
        }
        else if(pattern==6) base=gridded(base,worldPosition.xz,0.6,base*0.70,0.010);

        // Phong illumination: ambient + diffuse + specular.
        float shadow=shadowFactor(n);
        vec3 ambient=mix(groundBounce,skyColor,n.y*0.5+0.5)*0.62;
        float diffuse=max(dot(n,sunDirection),0.0)*shadow;
        vec3 reflection=reflect(-sunDirection,n);
        float shininess=mix(8.0,96.0,gloss);
        float specular=pow(max(dot(viewDirection,reflection),0.0),shininess)*gloss*shadow;
        if(pattern==8) ambient=vec3(0.44,0.44,0.42);          // indoor surfaces: neutral bounce light
        vec3 lampDiffuse,lampSpecular;
        pointLights(n,viewDirection,shininess,lampDiffuse,lampSpecular);
        vec3 color=base*(ambient+sunColor*diffuse*0.95+lampDiffuse)
                  +sunColor*specular*0.6+lampSpecular*gloss*0.5;
        if(pattern==10) color=base;                           // emissive: lamps, daylight glazing, lit signs
        if(pattern==2||pattern==7){
            vec3 r=reflect(-viewDirection,n);
            vec3 env=mix(groundBounce*0.6,skyColor*1.1,clamp(r.y*0.5+0.55,0.0,1.0));
            float fresnel=0.08+0.62*pow(1.0-max(dot(n,viewDirection),0.0),3.0);
            color=mix(color,env,fresnel*0.35);
        }
        // Orthographic elevations/plan: soften overexposed sunlit white roofs instead of clipping.
        if(highlightRolloff==1) color=mix(color,0.78+0.22*(1.0-exp(-(color-0.78)/0.22)),step(0.78,color));
        float distanceToEye=length(cameraPosition-worldPosition);
        float fog=1.0-exp(-max(distanceToEye-45.0,0.0)*0.006);
        pixel=vec4(mix(color,fogColor,fog*0.65),pattern==7 ? .20 : 1.0);
    }
)";
const char* depthVertexSource=R"(
    #version 330 core
    layout(location=0) in vec3 position;
    uniform mat4 lightSpace;
    uniform mat4 model;
    void main(){ gl_Position=lightSpace*model*vec4(position,1.0); }
)";
const char* depthFragmentSource=R"(
    #version 330 core
    void main(){}
)";

// Camera-oriented procedural sky: soft cumulus over a blue daylight gradient.
const char* skyVertexSource=R"(
    #version 330 core
    out vec2 screen;
    void main(){
        vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);
        screen=p*2.0-1.0; gl_Position=vec4(screen,0,1);
    }
)";
const char* skyFragmentSource=R"(
    #version 330 core
    in vec2 screen;
    uniform vec3 forwardDirection,rightDirection,upDirection,projectionInfo;
    out vec4 pixel;
    float hash(vec2 p){
        uvec2 q=uvec2(ivec2(p)); uint h=q.x*1597334677u ^ q.y*3812015801u;
        h=(h^(h>>16))*2246822519u; h^=h>>13;
        return float(h&16777215u)/16777215.0;
    }
    float noise(vec2 p){
        vec2 i=floor(p),f=fract(p); f=f*f*(3.0-2.0*f);
        return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1)),f.x),f.y);
    }
    float clouds(vec2 p){
        float n=0.0,a=.55;
        for(int i=0;i<5;++i){n+=a*noise(p);p=p*2.03+vec2(3.7,1.8);a*=.48;}
        return n;
    }
    void main(){
        vec3 ray=normalize(forwardDirection+rightDirection*screen.x*projectionInfo.x*projectionInfo.y
                          +upDirection*screen.y*projectionInfo.y);
        float elevation=max(ray.y,0.0);
        vec3 color=mix(vec3(.75,.85,.91),vec3(.23,.49,.78),pow(elevation,.32));
        vec2 uv=ray.xz/(elevation+.22)*1.7+vec2(8.2,3.5);
        float density=clouds(uv);
        float mask=smoothstep(.47,.67,density)*smoothstep(.015,.15,elevation);
        vec3 cloud=mix(vec3(.65,.74,.79),vec3(.98,.98,.95),smoothstep(.46,.70,clouds(uv+vec2(.08,.03))));
        pixel=vec4(mix(color,cloud,mask),1);
    }
)";

// Full-screen quad that shows a finished ray-traced frame (rows stored top-down).
const char* imageFragmentSource=R"(
    #version 330 core
    in vec2 screen;
    uniform sampler2D image;
    out vec4 pixel;
    void main(){ pixel=vec4(texture(image,vec2(screen.x*0.5+0.5,0.5-screen.y*0.5)).rgb,1.0); }
)";

GLuint compileShader(GLenum type,const char* source) {
    GLuint shader=glCreateShader(type); glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
    GLint ok=GL_FALSE; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
        std::cerr<<"Shader compilation failed:\n"<<log<<'\n'; glDeleteShader(shader); return 0; }
    return shader;
}
GLuint makeProgram(const char* vertexSource,const char* fragmentSource) {
    GLuint vertex=compileShader(GL_VERTEX_SHADER,vertexSource),fragment=compileShader(GL_FRAGMENT_SHADER,fragmentSource);
    if(!vertex||!fragment) return 0;
    GLuint result=glCreateProgram(); glAttachShader(result,vertex); glAttachShader(result,fragment); glLinkProgram(result);
    glDeleteShader(vertex); glDeleteShader(fragment); GLint ok=GL_FALSE; glGetProgramiv(result,GL_LINK_STATUS,&ok);
    if(!ok) { char log[2048]{}; glGetProgramInfoLog(result,sizeof(log),nullptr,log);
        std::cerr<<"Shader linking failed:\n"<<log<<'\n'; glDeleteProgram(result); return 0; }
    return result;
}

}} // namespace swc::app

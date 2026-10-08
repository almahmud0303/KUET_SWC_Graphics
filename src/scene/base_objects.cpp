#include "scene/base_objects.h"

#include "geometry/mesh.h"
#include "scene/components.h"
#include "scene/layout.h"
#include "scene/lettering.h"

namespace swc { namespace app {

// An isolated display range: never drawn together with the SWC building.
void buildBaseObjectGallery() {
    baseObjectsFirst=static_cast<GLint>(mesh.size());
    block(-7,-.25f,-27,7,0,-13,walkway);
    const char* names[]={"CUBE","CYLINDER","SPHERE","CONE","FRUSTUM","TUBE","RING","PLANE","TRIANGLE"};
    const Color colors[]={{.75f,.29f,.18f},{.20f,.54f,.74f},{.30f,.64f,.36f}};
    for(int i=0;i<9;++i) {
        float x=(i%3-1)*4.4f,z=-24.5f+(i/3)*4.4f;
        box(x,.06f,z,3.7f,.12f,3.7f,trim);
        Material material{colors[i%3],.25f};
        {
            Place object(translation({x,.15f,z}));
            auto out=[&](Vec3 p,Vec3 n) { emit(p,n,material); };
            switch(i) {
            case 0: { Place p(trs({0,.8f,0},0,{1.6f,1.6f,1.6f})); canonical::cube(out); break; }
            case 1: { Place p(trs({0,.9f,0},0,{.8f,1.8f,.8f})); canonical::cylinder(out,32); break; }
            case 2: { Place p(translation({0,1,0})); canonical::sphere(out,16,24); break; }
            case 3: { Place p(scaling({1,2,1})); canonical::cone(out,32); break; }
            case 4: { Place p(scaling({1,1.8f,1})); canonical::frustum(out,.5f,32); break; }
            case 5: { Place p(scaling({.85f,1.7f,.85f})); canonical::tube(out,32); break; }
            case 6: canonical::annulus(out,.55f); break;
            case 7: { Place p(scaling(2)); canonical::plane(out); break; }
            case 8: { Place p(trs({-.8f,0,0},0,{1.6f,1.6f,1.6f})); canonical::triangle(out); break; }
            }
        }
        // Supplement the building's small lettering alphabet only for gallery labels.
        auto letters=latinGlyphs(names[i]);
        for(std::size_t k=0;k<letters.size();++k) switch(names[i][k]) {
        case 'B': letters[k]="1111010001111101000111110"; break;
        case 'Y': letters[k]="1000110001010100010000100"; break;
        case 'P': letters[k]="1111010001111101000010000"; break;
        case 'H': letters[k]="1000110001111111000110001"; break;
        case 'O': letters[k]="0111010001100011000101110"; break;
        case 'M': letters[k]="1000111011101011000110001"; break;
        case 'G': letters[k]="0111110000101111000101111"; break;
        }
        Place label(multiply(translation({x,.135f,z+1.55f}),rotationX(-PI/2)));
        flatText(letters,0,0,0,.055f,.075f,.008f,signDark);
    }
    baseObjectsCount=static_cast<GLsizei>(mesh.size())-baseObjectsFirst;
}

}} // namespace swc::app

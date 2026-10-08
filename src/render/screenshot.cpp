#include "render/screenshot.h"

#include "geometry/mesh.h"
#include <cstdio>

namespace swc { namespace app {

bool writeScreenshot(const char* path,int width,int height) {
    std::vector<unsigned char> pixels(static_cast<std::size_t>(width)*height*3);
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadPixels(0,0,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels.data());
    FILE* file=std::fopen(path,"wb");
    if(!file) return false;
    std::fprintf(file,"P6\n%d %d\n255\n",width,height);
    for(int y=height-1;y>=0;--y) std::fwrite(&pixels[static_cast<std::size_t>(y)*width*3],1,static_cast<std::size_t>(width)*3,file);
    std::fclose(file);
    return true;
}

}} // namespace swc::app

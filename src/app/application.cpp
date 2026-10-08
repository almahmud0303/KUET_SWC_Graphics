#include "app/application.h"

#include "navigation/navigation.h"
#include "render/renderer.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace swc { namespace app {

int runApplication(int argc,char** argv) {
    // Optional: image_project.exe --capture <view 1-21> <file.ppm> [width height] renders one frame to a file.
    int captureView=-1; const char* capturePath=nullptr;
    int windowWidth=1280,windowHeight=800;
    if((argc==4||argc==6)&&std::strcmp(argv[1],"--capture")==0) { captureView=std::atoi(argv[2])-1; capturePath=argv[3]; }
    if(capturePath&&argc==6) { windowWidth=std::max(std::atoi(argv[4]),64); windowHeight=std::max(std::atoi(argv[5]),64); }

    if(argc==2&&std::strcmp(argv[1],"--check-walk")==0) return checkWalkRoutes();
    return runViewer(captureView,capturePath,windowWidth,windowHeight);
}

}} // namespace swc::app

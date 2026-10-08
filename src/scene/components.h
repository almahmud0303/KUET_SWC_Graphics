#pragma once

namespace swc { namespace app {

struct WindowWing {
    int bays; float bay; int levels; float storey; int perBay;
    float overhang; bool freeColumns; bool acUnits; bool skipGround; bool walkway;
};
struct Gallery {
    int bays; float bay; int levels; float storey; float depth;
    unsigned frames; unsigned passages; unsigned voids; bool tileAccents; float frameBase;
};

void windowBay(float x,float floorY,float w,float seed,bool transom=true);
void acUnit(float x,float y,float z);
void door(float x,float floorY,float seed,float w=1.0f,float h=2.3f);
void balconyPanel(float x0,float x1,float floorY);
void openFrame(float x0,float x1,float y0,float y1,float front);
void windowedWing(const WindowWing& s,float seed);
void galleryWing(const Gallery& s,float seed);

}} // namespace swc::app

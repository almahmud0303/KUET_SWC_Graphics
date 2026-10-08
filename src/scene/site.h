#pragma once

#include "scene/materials.h"

namespace swc { namespace app {

void petalRing(float cx,float cz,float y,float r0,float r1,int count,float halfAngle,const Material& a,const Material& b,float phase=0);
void alpona(float cx,float cz);
void site();

}} // namespace swc::app

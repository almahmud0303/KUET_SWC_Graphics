#pragma once
#include "walk_navigation.h"

namespace swc { namespace app {

extern WalkNavigation walkNavigation;
extern bool navigationReady;

void buildNavigation();
int checkWalkRoutes();

}} // namespace swc::app

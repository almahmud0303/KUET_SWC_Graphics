#pragma once

#include "geometry/geometry_math.h"

namespace swc { namespace app {

constexpr float PLAIN=0, BRICK=1, GLASS=2, NATURAL=3, PAVING=4, THATCH=5, TILE=6, TRANSPARENT_GLASS=7, INTERIOR=8, LAWN=9, EMISSIVE=10;
struct Material { Color color; float gloss=.08f; float pattern=PLAIN; };
// Base colours from section 12 of SWC_Reference.txt.
extern const Material concrete, trim, roofMembrane;
extern const Material brick, brickLight;
extern const Material brickDark, warmPanel;
extern const Material metal, metalDark;
extern const Material wood, woodGroove;
extern const Material paving, tanPaver;
extern const Material grass;
extern const Material walkway, darkTile;
extern const Material road, darkGap;
extern const Material lampBlack, lampWhite;
extern const Material bark, palmBark;
extern const Material whitePaint, redPaint;
extern const Material acBody, bamboo, thatch;
extern const Material clearGlass, signDark;
// Interior finishes from the project_3 walkthrough photographs.
extern const Material plaster, terracotta, timber, steel, daylight, tubeLight, seatBlue, potRed;

Material glassFor(float seed);
Material leafMaterial(float seed,float lightness=0.0f);

}} // namespace swc::app

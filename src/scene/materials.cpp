#include "scene/materials.h"



namespace swc { namespace app {

const Material concrete{{.89f,.91f,.88f}}, trim{{.95f,.96f,.93f}}, roofMembrane{{.60f,.61f,.58f},.04f,NATURAL};
const Material brick{{.50f,.20f,.13f},.05f,BRICK}, brickLight{{.62f,.31f,.22f},.05f,BRICK};
const Material brickDark{{.36f,.15f,.10f},.04f,BRICK}, warmPanel{{.66f,.36f,.22f},.06f,BRICK};
const Material metal{{.72f,.76f,.73f},.55f}, metalDark{{.20f,.22f,.22f},.45f};
const Material wood{{.29f,.12f,.06f},.25f}, woodGroove{{.20f,.08f,.04f},.2f};
const Material paving{{.53f,.27f,.20f},.06f,PAVING}, tanPaver{{.67f,.47f,.31f},.06f};
const Material grass{{.28f,.40f,.12f},.03f,LAWN};
const Material walkway{{.88f,.86f,.80f},.45f,TILE}, darkTile{{.10f,.10f,.11f},.50f};
const Material road{{.64f,.63f,.60f},.04f,NATURAL}, darkGap{{.05f,.06f,.06f},.05f};
const Material lampBlack{{.05f,.05f,.05f},.6f}, lampWhite{{1.0f,1.0f,.95f},.2f};
const Material bark{{.33f,.25f,.18f},.04f,NATURAL}, palmBark{{.45f,.40f,.33f},.04f,NATURAL};
const Material whitePaint{{.95f,.95f,.92f},.08f}, redPaint{{.75f,.10f,.08f},.1f};
const Material acBody{{.86f,.87f,.84f},.3f}, bamboo{{.72f,.56f,.32f},.15f}, thatch{{.63f,.49f,.30f},.02f,THATCH};
const Material clearGlass{{.60f,.76f,.78f},.9f,GLASS}, signDark{{.22f,.23f,.24f},.5f};
const Material plaster{{.90f,.90f,.86f},.08f,INTERIOR}, terracotta{{.66f,.30f,.15f},.06f,BRICK};
const Material timber{{.36f,.17f,.08f},.30f,INTERIOR}, steel{{.62f,.65f,.64f},.60f,INTERIOR};
const Material daylight{{.84f,.92f,.88f},.0f,EMISSIVE}, tubeLight{{1.0f,1.0f,.94f},.0f,EMISSIVE};
const Material seatBlue{{.10f,.30f,.70f},.35f,INTERIOR}, potRed{{.70f,.14f,.12f},.15f};

Material glassFor(float seed) {
    float h=hash1(seed);
    if(h<.10f) return {{.025f,.03f,.03f},.05f};                       // open pane: dark interior gap
    Color dark{.10f,.18f,.18f}, pale{.40f,.50f,.53f};
    return {mix(dark,pale,hash1(seed*1.37f+3.1f)*.65f),.9f,GLASS};
}
Material leafMaterial(float seed,float lightness) {
    Color deep{.07f,.25f,.07f}, fresh{.25f,.50f,.14f};
    return {mix(deep,fresh,limit(hash1(seed)*.8f+lightness,0,1)),.06f,NATURAL};
}

}} // namespace swc::app

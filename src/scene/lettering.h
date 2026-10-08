#pragma once
#include <vector>
#include <string>
#include "scene/materials.h"

namespace swc { namespace app {

const char* glyph(char c);
std::vector<std::string> latinGlyphs(const std::string& text);
std::vector<std::string> bengaliGlyphs(const std::vector<int>& wordLengths,float seed);
void flatText(const std::vector<std::string>& glyphs,float centerX,float y,float z,float sx,float sy,float depth,const Material& m);

}} // namespace swc::app

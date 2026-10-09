#include "scene/lettering.h"

#include "geometry/mesh.h"
#include "scene/layout.h"
#include <string>

namespace swc { namespace app {

// ---------------------------------------------------------------------------
// Lettering: 5x5 block glyphs, and a stylised Bengali line (continuous matra
// headstroke with hanging strokes) standing in for the Bengali signage.
// ---------------------------------------------------------------------------
const char* glyph(char c) {
    switch(c) {
    case 'A': return "0111010001111111000110001";
    case 'B': return "1111010001111101000111110";
    case 'G': return "0111110000101111000101111";
    case 'H': return "1000110001111111000110001";
    case 'M': return "1000111011101011000110001";
    case 'O': return "0111010001100011000101110";
    case 'P': return "1111010001111101000010000";
    case 'Y': return "1000101010001000010000100";
    case 'C': return "0111110000100001000001111";
    case 'D': return "1111010001100011000111110";
    case 'E': return "1111110000111101000011111";
    case 'F': return "1111110000111101000010000";
    case 'I': return "1111100100001000010011111";
    case 'K': return "1000110010111001001010001";
    case 'L': return "1000010000100001000011111";
    case 'N': return "1000111001101011001110001";
    case 'R': return "1111010001111101010010010";
    case 'S': return "0111110000011100000111110";
    case 'T': return "1111100100001000010000100";
    case 'U': return "1000110001100011000101110";
    case 'W': return "1000110001101011101110001";
    default: return "0000000000000000000000000";
    }
}
std::vector<std::string> latinGlyphs(const std::string& text) {
    std::vector<std::string> out;
    for(char c : text) out.push_back(glyph(c));
    return out;
}
std::vector<std::string> bengaliGlyphs(const std::vector<int>& wordLengths,float seed) {
    std::vector<std::string> out;
    for(std::size_t w=0;w<wordLengths.size();++w) {
        if(w>0) out.push_back(glyph(' '));
        for(int k=0;k<wordLengths[w];++k) {
            std::string g(25,'0');
            for(int c=0;c<5;++c) g[c]='1';                                  // matra headstroke
            float s=seed+w*17.0f+k*3.1f;
            int stem=hash1(s)<.6f ? 3 : 1;
            for(int r=1;r<5;++r) g[r*5+stem]='1';
            for(int r=1;r<5;++r) for(int c=0;c<5;++c)
                if(c!=stem && hash1(s+r*7.3f+c*1.9f)>.68f) g[r*5+c]='1';
            out.push_back(g);
        }
    }
    return out;
}
// Text on a local facade plane (+Z outward), centred on centerX.
void flatText(const std::vector<std::string>& glyphs,float centerX,float y,float z,float sx,float sy,float depth,const Material& m) {
    float start=centerX-(glyphs.size()*6-1)*sx/2;
    for(std::size_t k=0;k<glyphs.size();++k)
        for(int row=0;row<5;++row) for(int col=0;col<5;++col)
            if(glyphs[k][row*5+col]=='1') {
                float x=start+(k*6+col)*sx,yy=y+(4-row)*sy;
                block(x,yy,z,x+sx*.86f,yy+sy*.86f,z+depth,m);
            }
}

}} // namespace swc::app

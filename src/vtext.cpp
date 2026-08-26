#include "vtext.h"

#include "screen.h"

namespace {

struct Glyph {
  const signed char (*stroke)[4];
  int count;
};

#define STROKES(name, ...) \
  const signed char name[][4] = {__VA_ARGS__}

STROKES(kA, {0,6,2,0}, {2,0,4,6}, {1,3,3,3});
STROKES(kB, {0,0,0,6}, {0,0,3,0}, {3,0,4,1}, {4,1,4,2},
             {4,2,3,3}, {3,3,0,3}, {3,3,4,4}, {4,4,4,5},
             {4,5,3,6}, {3,6,0,6});
STROKES(kC, {4,0,0,0}, {0,0,0,6}, {0,6,4,6});
STROKES(kD, {0,0,0,6}, {0,0,3,0}, {3,0,4,1}, {4,1,4,5},
             {4,5,3,6}, {3,6,0,6});
STROKES(kE, {4,0,0,0}, {0,0,0,6}, {0,3,3,3}, {0,6,4,6});
STROKES(kF, {4,0,0,0}, {0,0,0,6}, {0,3,3,3});
STROKES(kG, {4,0,0,0}, {0,0,0,6}, {0,6,4,6}, {4,6,4,3},
             {4,3,2,3});
STROKES(kH, {0,0,0,6}, {4,0,4,6}, {0,3,4,3});
STROKES(kI, {0,0,4,0}, {2,0,2,6}, {0,6,4,6});
STROKES(kJ, {0,0,4,0}, {4,0,4,5}, {4,5,3,6}, {3,6,1,6},
             {1,6,0,5});
STROKES(kK, {0,0,0,6}, {4,0,0,4}, {1,3,4,6});
STROKES(kL, {0,0,0,6}, {0,6,4,6});
STROKES(kM, {0,6,0,0}, {0,0,2,3}, {2,3,4,0}, {4,0,4,6});
STROKES(kN, {0,6,0,0}, {0,0,4,6}, {4,6,4,0});
STROKES(kO, {1,0,3,0}, {3,0,4,1}, {4,1,4,5}, {4,5,3,6},
             {3,6,1,6}, {1,6,0,5}, {0,5,0,1}, {0,1,1,0});
STROKES(kP, {0,6,0,0}, {0,0,3,0}, {3,0,4,1}, {4,1,4,2},
             {4,2,3,3}, {3,3,0,3});
STROKES(kQ, {1,0,3,0}, {3,0,4,1}, {4,1,4,5}, {4,5,3,6},
             {3,6,1,6}, {1,6,0,5}, {0,5,0,1}, {0,1,1,0},
             {2,4,4,6});
STROKES(kR, {0,6,0,0}, {0,0,3,0}, {3,0,4,1}, {4,1,4,2},
             {4,2,3,3}, {3,3,0,3}, {2,3,4,6});
STROKES(kS, {4,0,0,0}, {0,0,0,3}, {0,3,4,3}, {4,3,4,6},
             {4,6,0,6});
STROKES(kT, {0,0,4,0}, {2,0,2,6});
STROKES(kU, {0,0,0,5}, {0,5,1,6}, {1,6,3,6}, {3,6,4,5},
             {4,5,4,0});
STROKES(kV, {0,0,2,6}, {2,6,4,0});
STROKES(kW, {0,0,1,6}, {1,6,2,4}, {2,4,3,6}, {3,6,4,0});
STROKES(kX, {0,0,4,6}, {4,0,0,6});
STROKES(kY, {0,0,2,3}, {4,0,2,3}, {2,3,2,6});
STROKES(kZ, {0,0,4,0}, {4,0,0,6}, {0,6,4,6});
STROKES(k0, {1,0,3,0}, {3,0,4,1}, {4,1,4,5}, {4,5,3,6},
             {3,6,1,6}, {1,6,0,5}, {0,5,0,1}, {0,1,1,0},
             {1,5,3,1});
STROKES(k1, {1,1,2,0}, {2,0,2,6}, {0,6,4,6});
STROKES(k2, {0,1,1,0}, {1,0,3,0}, {3,0,4,1}, {4,1,0,6},
             {0,6,4,6});
STROKES(k3, {0,0,4,0}, {4,0,4,6}, {1,3,4,3}, {0,6,4,6});
STROKES(k4, {0,0,0,3}, {0,3,4,3}, {4,0,4,6});
STROKES(k5, {4,0,0,0}, {0,0,0,3}, {0,3,4,3}, {4,3,4,6},
             {4,6,0,6});
STROKES(k6, {4,0,0,3}, {0,3,0,6}, {0,6,3,6}, {3,6,4,5},
             {4,5,4,4}, {4,4,3,3}, {3,3,0,3});
STROKES(k7, {0,0,4,0}, {4,0,1,6});
STROKES(k8, {1,0,3,0}, {3,0,4,1}, {4,1,4,2}, {4,2,3,3},
             {3,3,1,3}, {1,3,0,2}, {0,2,0,1}, {0,1,1,0},
             {1,3,0,4}, {0,4,0,5}, {0,5,1,6}, {1,6,3,6},
             {3,6,4,5}, {4,5,4,4}, {4,4,3,3});
STROKES(k9, {4,3,1,3}, {1,3,0,2}, {0,2,0,1}, {0,1,1,0},
             {1,0,3,0}, {3,0,4,1}, {4,1,4,6}, {4,6,0,6});
STROKES(kSlash, {0,6,4,0});

#undef STROKES

Glyph glyph(char c) {
  Glyph result = {0, 0};
#define GLYPH(ch, data) case ch: result.stroke = data; \
  result.count = sizeof(data) / sizeof(data[0]); break
  switch (c) {
    GLYPH('A', kA); GLYPH('B', kB); GLYPH('C', kC); GLYPH('D', kD);
    GLYPH('E', kE); GLYPH('F', kF); GLYPH('H', kH);
    GLYPH('G', kG); GLYPH('I', kI); GLYPH('J', kJ);
    GLYPH('K', kK); GLYPH('L', kL); GLYPH('M', kM);
    GLYPH('N', kN); GLYPH('O', kO); GLYPH('P', kP);
    GLYPH('Q', kQ); GLYPH('R', kR); GLYPH('S', kS);
    GLYPH('T', kT); GLYPH('U', kU); GLYPH('V', kV);
    GLYPH('W', kW); GLYPH('X', kX); GLYPH('Y', kY);
    GLYPH('Z', kZ);
    GLYPH('0', k0); GLYPH('1', k1); GLYPH('2', k2);
    GLYPH('3', k3); GLYPH('4', k4); GLYPH('5', k5);
    GLYPH('6', k6); GLYPH('7', k7); GLYPH('8', k8);
    GLYPH('9', k9);
    GLYPH('/', kSlash);
    default: break;
  }
#undef GLYPH
  return result;
}

int text_length(const char *text) {
  int length = 0;
  while (text[length]) ++length;
  return length;
}

int text_width(const char *text, int scale, int tracking) {
  const int length = text_length(text);
  if (!length) return 0;
  return (length - 1) * (5 * scale + tracking) + 4 * scale;
}

}  // namespace

void vector_text(const char *text, int x, int y, int scale,
                 int tracking, int slant, iocs_color_t color) {
  const int advance = 5 * scale + tracking;
  while (*text) {
    const Glyph data = glyph(*text++);
    for (int i = 0; i < data.count; ++i) {
      const int y0 = data.stroke[i][1];
      const int y1 = data.stroke[i][3];
      const int x0 = x + data.stroke[i][0] * scale + (6 - y0) * slant / 6;
      const int x1 = x + data.stroke[i][2] * scale + (6 - y1) * slant / 6;
      screen_line(x0, y + y0 * scale, x1, y + y1 * scale, color);
    }
    x += advance;
  }
}

void vector_centered(const char *text, int y, int scale,
                     int tracking, int slant, iocs_color_t color) {
  const int x = (512 - text_width(text, scale, tracking)) / 2;
  vector_text(text, x, y, scale, tracking, slant, color);
}

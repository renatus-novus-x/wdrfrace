#include "screen.h"
#include "telefont.h"

#include <stdint.h>

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;

const iocs_color_t kPalette[] = {
  0x0000, 0xffff, 0x07ff, 0xf83f, 0x39cf, 0x2109, 0x2108,
  0x7bef, 0x07c1, 0xf801, 0x003f, 0x67d9, 0x62bf, 0x1bc7,
  0x3dcf, 0xa7e9, 0xdff7, 0x211f, 0x422f, 0xa4bf, 0xde7f,
};

const uint8_t *glyph(char c) {
  if (c >= '0' && c <= '9') return kDigits[c - '0'];
  if (c >= 'A' && c <= 'Z') return kLetters[c - 'A'];
  return kSpace;
}

int text_length(const char *text) {
  int length = 0;
  while (text[length]) ++length;
  return length;
}

}  // namespace

void screen_palette_initialize() {
  const int count = sizeof(kPalette) / sizeof(kPalette[0]);
  for (int i = 0; i < count; ++i) _iocs_gpalet(i, kPalette[i]);
}

iocs_color_t screen_palette_color(iocs_color_t color) {
  switch (color) {
    case 0x0000: return 0;
    case 0xffff: return 1;
    case 0x07ff: return 2;
    case 0xf83f: return 3;
    case 0x39cf: return 4;
    case 0x2109: return 5;
    case 0x2108: return 6;
    case 0x7bef: return 7;
    case 0x07c1: return 8;
    case 0xf801: return 9;
    case 0x003f: return 10;
    case 0x67d9: return 11;
    case 0x62bf: return 12;
    case 0x1bc7: return 13;
    case 0x3dcf: return 14;
    case 0xa7e9: return 15;
    case 0xdff7: return 16;
    case 0x211f: return 17;
    case 0x422f: return 18;
    case 0xa4bf: return 19;
    case 0xde7f: return 20;
    default: return 1;
  }
}

void screen_clear(iocs_color_t color) {
  screen_fill(0, 0, FIELD_W, FIELD_H, color);
}

void screen_fill(int x, int y, int width, int height, iocs_color_t color) {
  if (width <= 0 || height <= 0) return;
  struct iocs_fillptr rect;
  rect.x1 = (short)x;
  rect.y1 = (short)y;
  rect.x2 = (short)(x + width - 1);
  rect.y2 = (short)(y + height - 1);
  rect.color = screen_palette_color(color);
  _iocs_fill(&rect);
}

void screen_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  struct iocs_lineptr line;
  line.x1 = (short)x0;
  line.y1 = (short)y0;
  line.x2 = (short)x1;
  line.y2 = (short)y1;
  line.color = screen_palette_color(color);
  line.linestyle = 0xffff;
  _iocs_line(&line);
}

void screen_text(int x, int y, const char *text, int scale,
                 iocs_color_t color) {
  screen_text_tracking(x, y, text, scale, 0, color);
}

void screen_text_tracking(int x, int y, const char *text, int scale,
                          int tracking, iocs_color_t color) {
  while (*text) {
    const uint8_t *rows = glyph(*text++);
    for (int row = 0; row < 7; ++row) {
      for (int column = 0; column < 5; ++column) {
        if (rows[row] & (1 << (4 - column))) {
          screen_fill(x + column * scale, y + row * scale,
                      scale, scale, color);
        }
      }
    }
    x += 6 * scale + tracking;
  }
}

void screen_centered(const char *text, int y, int scale,
                     iocs_color_t color) {
  screen_centered_tracking(text, y, scale, 0, color);
}

void screen_centered_tracking(const char *text, int y, int scale,
                              int tracking, iocs_color_t color) {
  const int length = text_length(text);
  const int width = length * 6 * scale - scale + (length - 1) * tracking;
  screen_text_tracking((FIELD_W - width) / 2, y, text, scale,
                       tracking, color);
}

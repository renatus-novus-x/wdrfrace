#include "screen.h"

#include <stdint.h>

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;

const uint8_t kDigits[10][7] = {
  {14, 17, 19, 21, 25, 17, 14},
  {4, 12, 4, 4, 4, 4, 14},
  {14, 17, 1, 2, 4, 8, 31},
  {30, 1, 1, 14, 1, 1, 30},
  {2, 6, 10, 18, 31, 2, 2},
  {31, 16, 16, 30, 1, 1, 30},
  {14, 16, 16, 30, 17, 17, 14},
  {31, 1, 2, 4, 8, 8, 8},
  {14, 17, 17, 14, 17, 17, 14},
  {14, 17, 17, 15, 1, 1, 14},
};

const uint8_t kLetters[26][7] = {
  {14, 17, 17, 31, 17, 17, 17},
  {30, 17, 17, 30, 17, 17, 30},
  {14, 17, 16, 16, 16, 17, 14},
  {30, 17, 17, 17, 17, 17, 30},
  {31, 16, 16, 30, 16, 16, 31},
  {31, 16, 16, 30, 16, 16, 16},
  {14, 17, 16, 23, 17, 17, 15},
  {17, 17, 17, 31, 17, 17, 17},
  {14, 4, 4, 4, 4, 4, 14},
  {7, 2, 2, 2, 18, 18, 12},
  {17, 18, 20, 24, 20, 18, 17},
  {16, 16, 16, 16, 16, 16, 31},
  {17, 27, 21, 21, 17, 17, 17},
  {17, 25, 21, 19, 17, 17, 17},
  {14, 17, 17, 17, 17, 17, 14},
  {30, 17, 17, 30, 16, 16, 16},
  {14, 17, 17, 17, 21, 18, 13},
  {30, 17, 17, 30, 20, 18, 17},
  {15, 16, 16, 14, 1, 1, 30},
  {31, 4, 4, 4, 4, 4, 4},
  {17, 17, 17, 17, 17, 17, 14},
  {17, 17, 17, 17, 17, 10, 4},
  {17, 17, 17, 21, 21, 21, 10},
  {17, 17, 10, 4, 10, 17, 17},
  {17, 17, 10, 4, 4, 4, 4},
  {31, 1, 2, 4, 8, 16, 31},
};

const uint8_t kSpace[7] = {0, 0, 0, 0, 0, 0, 0};

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
  rect.color = color;
  _iocs_fill(&rect);
}

void screen_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  struct iocs_lineptr line;
  line.x1 = (short)x0;
  line.y1 = (short)y0;
  line.x2 = (short)x1;
  line.y2 = (short)y1;
  line.color = color;
  line.linestyle = 0xffff;
  _iocs_line(&line);
}

void screen_text(int x, int y, const char *text, int scale,
                 iocs_color_t color) {
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
    x += 6 * scale;
  }
}

void screen_centered(const char *text, int y, int scale,
                     iocs_color_t color) {
  const int width = text_length(text) * 6 * scale - scale;
  screen_text((FIELD_W - width) / 2, y, text, scale, color);
}

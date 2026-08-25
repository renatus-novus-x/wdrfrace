#ifndef WDR_SCREEN_H
#define WDR_SCREEN_H

#include <x68k/iocs.h>

void screen_palette_initialize();
iocs_color_t screen_palette_color(iocs_color_t color);
void screen_clear(iocs_color_t color);
void screen_fill(int x, int y, int width, int height, iocs_color_t color);
void screen_line(int x0, int y0, int x1, int y1, iocs_color_t color);
void screen_text(int x, int y, const char *text, int scale,
                 iocs_color_t color);
void screen_text_tracking(int x, int y, const char *text, int scale,
                          int tracking, iocs_color_t color);
void screen_centered(const char *text, int y, int scale,
                     iocs_color_t color);
void screen_centered_tracking(const char *text, int y, int scale,
                              int tracking, iocs_color_t color);

#endif

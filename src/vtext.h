#ifndef WDR_VTEXT_H
#define WDR_VTEXT_H

#include <x68k/iocs.h>

void vector_text(const char *text, int x, int y, int scale,
                 int tracking, int slant, iocs_color_t color);
void vector_centered(const char *text, int y, int scale,
                     int tracking, int slant, iocs_color_t color);

#endif

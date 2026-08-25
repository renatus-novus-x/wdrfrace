# Font Data Origin

Wire Drift Racers does not embed or convert an external font file for its
compact telemetry text.

`src/mktele.py` defines A-Z and 0-9 as original monoline strokes on a 5 by 7
grid. During the build, the script rasterizes those strokes with an integer
line algorithm and generates `src/telefont.h`. `src/screen.cpp` renders the
generated bitmap.

The design goals are:

- match the Technical Monoline direction used by the vector headings;
- keep appearance and text metrics identical on hardware and emulators;
- use a small, predictable number of graphics-VRAM writes;
- make the construction and provenance of every glyph auditable in source.

The X68000 IOCS `_iocs_fntget()` API can retrieve the active machine font at
runtime. It is intentionally not used for the primary game UI because ROM and
emulator font differences would change the visual identity and layout. A future
system-message or Japanese-text renderer may retrieve glyphs once during
initialization, cache them, and draw the cached bits into the active graphics
page.

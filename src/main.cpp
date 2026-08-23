#include <math.h>
#include <stdint.h>
#include <x68k/iocs.h>

#define FIELD_W 512
#define FIELD_H 480

#define KEY_ESC 0x01

#define FPS_WINDOW_FRAMES 300
#define HUD_X 8
#define HUD_Y 8
#define HUD_W 170
#define HUD_H 28

#define CRTC_R04 ((void *)0x00E80008UL)
#define CRTC_R05 ((void *)0x00E8000AUL)
#define CRTC_R06 ((void *)0x00E8000CUL)
#define CRTC_R07 ((void *)0x00E8000EUL)
#define MFP_GPIP ((const void *)0x00E88001UL)

#define WAIT_VDISP_TIMEOUT_CS 100
#define CENTISEC_PER_DAY 8640000L
#define GPIP_VDISP 0x10

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xffff

typedef struct {
  float x;
  float y;
  float z;
} Vec3f;

typedef struct {
  int16_t x;
  int16_t y;
} Vec2s;

typedef struct {
  uint8_t a;
  uint8_t b;
} Edge;

typedef struct {
  int frame;
  iocs_color_t old_mode;
  Vec3f base[12];
  Vec3f curr[12];
  Vec2s prev[12];
  Vec2s next[12];
  uint8_t visible_prev[12];
  uint8_t visible_next[12];
  float angle;
  int have_prev;

  struct iocs_time fps_start;
  int fps_count;
  int fps_x100;
  uint8_t fps_ready;
} GameState;

static const Vec3f kCarModel[] = {
  {-1.4f, -0.45f, -2.0f}, {1.4f, -0.45f, -2.0f}, {1.4f, 0.05f, -2.0f},
  {-1.4f, 0.05f, -2.0f}, {-1.0f, -0.45f, 2.0f},  {1.0f, -0.45f, 2.0f},
  {1.0f, 0.05f, 2.0f},   {-1.0f, 0.05f, 2.0f},  {-0.9f, 0.65f, -1.1f},
  {0.9f, 0.65f, -1.1f},  {0.9f, 0.90f, 1.1f},   {-0.9f, 0.90f, 1.1f},
};

static const Edge kEdges[] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4},
  {0, 4}, {1, 5}, {2, 6}, {3, 7}, {0, 3}, {1, 2}, {4, 7}, {5, 6},
  {8, 9}, {9, 10}, {10, 11}, {11, 8}, {2, 8}, {3, 11}, {6, 10}, {7, 11},
};

// 4x5 bitmap font (bit 3..0)
static const uint8_t kDigits4x5[10][5] = {
  {0x0E, 0x0A, 0x0A, 0x0A, 0x0E}, // 0
  {0x04, 0x0C, 0x04, 0x04, 0x0E}, // 1
  {0x0E, 0x01, 0x0E, 0x08, 0x0E}, // 2
  {0x0E, 0x01, 0x06, 0x01, 0x0E}, // 3
  {0x08, 0x08, 0x0E, 0x01, 0x01}, // 4
  {0x0F, 0x08, 0x0E, 0x01, 0x0E}, // 5
  {0x06, 0x08, 0x0E, 0x0A, 0x06}, // 6
  {0x0F, 0x01, 0x02, 0x04, 0x08}, // 7
  {0x0E, 0x0A, 0x06, 0x0A, 0x0E}, // 8
  {0x06, 0x0A, 0x0E, 0x01, 0x06}, // 9
};

static const uint8_t kGlyphF[5] = {0x0F, 0x08, 0x0C, 0x08, 0x08};
static const uint8_t kGlyphP[5] = {0x0E, 0x0A, 0x0E, 0x08, 0x08};
static const uint8_t kGlyphS[5] = {0x07, 0x08, 0x0E, 0x01, 0x0E};

static inline long ontime_diff_cs(struct iocs_time start, struct iocs_time end) {
  return ((long)end.day - (long)start.day) * CENTISEC_PER_DAY
       + (long)end.sec - (long)start.sec;
}

static inline uint8_t read_gpip(void) {
  return (uint8_t)_iocs_b_bpeek(MFP_GPIP);
}

static inline void write_crtc(void *addr, uint16_t value) {
  _iocs_b_wpoke(addr, value);
}

static void draw_fill_block(int x1, int y1, int x2, int y2, iocs_color_t color) {
  struct iocs_fillptr rect;
  rect.x1 = (short)x1;
  rect.y1 = (short)y1;
  rect.x2 = (short)x2;
  rect.y2 = (short)y2;
  rect.color = color;
  _iocs_fill(&rect);
}

static void draw_pixel(int x, int y, iocs_color_t color) {
  if (x < 0 || y < 0 || x >= FIELD_W || y >= FIELD_H) return;
  draw_fill_block(x, y, x, y, color);
}

static void draw_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  int dx = x1 - x0;
  int sx = 1;
  int dy = y1 - y0;
  int sy = 1;

  if (dx < 0) {
    dx = -dx;
    sx = -1;
  }
  if (dy < 0) {
    dy = -dy;
    sy = -1;
  }

  int err = ((dx > dy) ? dx : -dy) / 2;
  int e2;

  while (1) {
    draw_pixel(x0, y0, color);
    if (x0 == x1 && y0 == y1) break;
    e2 = err;
    if (e2 > -dx) {
      err -= dy;
      x0 += sx;
    }
    if (e2 < dy) {
      err += dx;
      y0 += sy;
    }
  }
}

static void draw_wire(const Vec2s *pt, const uint8_t *visible, iocs_color_t color) {
  const int edge_count = (int)(sizeof(kEdges) / sizeof(kEdges[0]));
  for (int i = 0; i < edge_count; ++i) {
    const Edge e = kEdges[i];
    if (!visible[e.a] || !visible[e.b]) continue;
    draw_line(pt[e.a].x, pt[e.a].y, pt[e.b].x, pt[e.b].y, color);
  }
}

static const uint8_t *glyph_for_char(char c) {
  if (c >= '0' && c <= '9') return kDigits4x5[c - '0'];
  if (c == ' ') {
    static const uint8_t space[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
    return space;
  }
  if (c == '.') {
    static const uint8_t dot[5] = {0x00, 0x00, 0x00, 0x00, 0x04};
    return dot;
  }
  if (c == ':') {
    static const uint8_t colon[5] = {0x00, 0x04, 0x00, 0x04, 0x00};
    return colon;
  }
  if (c == 'F') return kGlyphF;
  if (c == 'P') return kGlyphP;
  if (c == 'S') return kGlyphS;
  static const uint8_t empty[5] = {0x00, 0x00, 0x00, 0x00, 0x00};
  return empty;
}

static void draw_glyph(int x, int y, char c, iocs_color_t color) {
  const uint8_t *g = glyph_for_char(c);
  for (int row = 0; row < 5; ++row) {
    uint8_t bits = g[row];
    for (int col = 0; col < 4; ++col) {
      if (bits & (1 << (3 - col))) draw_pixel(x + col, y + row, color);
    }
  }
}

static void draw_text4x5(int x, int y, const char *text, iocs_color_t color) {
  int cx = x;
  while (*text) {
    draw_glyph(cx, y, *text++, color);
    cx += 6;
  }
}

static void draw_fps_hud(const GameState *state) {
  draw_fill_block(HUD_X, HUD_Y, HUD_X + HUD_W, HUD_Y + HUD_H, COLOR_BLACK);

  draw_text4x5(HUD_X, HUD_Y, "FPS:", COLOR_WHITE);

  int x = HUD_X + 6 * 4;

  if (!state->fps_ready) {
    draw_text4x5(x, HUD_Y, "--.--", COLOR_WHITE);
    return;
  }

  int value = state->fps_x100;
  int i = value / 100;
  int f = value % 100;

  if (i >= 100) {
    draw_glyph(x + 6, HUD_Y, (char)('0' + (i / 100) % 10), COLOR_WHITE);
    draw_glyph(x + 12, HUD_Y, (char)('0' + (i / 10) % 10), COLOR_WHITE);
    draw_glyph(x + 18, HUD_Y, (char)('0' + i % 10), COLOR_WHITE);
    x += 24;
  } else if (i >= 10) {
    draw_glyph(x + 6, HUD_Y, (char)('0' + (i / 10)), COLOR_WHITE);
    draw_glyph(x + 12, HUD_Y, (char)('0' + i % 10), COLOR_WHITE);
    x += 18;
  } else {
    draw_glyph(x + 6, HUD_Y, (char)('0' + i), COLOR_WHITE);
    x += 12;
  }

  draw_glyph(x, HUD_Y, '.', COLOR_WHITE);
  draw_glyph(x + 6, HUD_Y, (char)('0' + (f / 10)), COLOR_WHITE);
  draw_glyph(x + 12, HUD_Y, (char)('0' + (f % 10)), COLOR_WHITE);
}

static void update_fps(GameState *state) {
  if (state->fps_count == 0) {
    state->fps_start = _iocs_ontime();
  }

  ++state->fps_count;
  if (state->fps_count < FPS_WINDOW_FRAMES) return;

  struct iocs_time now = _iocs_ontime();
  long dt_cs = ontime_diff_cs(state->fps_start, now);
  if (dt_cs > 0) {
    state->fps_x100 = (int)((FPS_WINDOW_FRAMES * 10000L) / dt_cs);
    state->fps_ready = 1;
  }
  state->fps_count = 0;
}

static int project(const Vec3f *in, float ax, float ay, Vec2s *out) {
  float cy = cosf(ay);
  float sy = sinf(ay);
  float cx = cosf(ax);
  float sx = sinf(ax);

  float x = in->x * cy + in->z * sy;
  float z = -in->x * sy + in->z * cy;
  float y = in->y * cx - z * sx;
  z = in->y * sx + z * cx + 12.0f;

  if (z < 1.0f) return 0;

  float p = 320.0f / z;
  out->x = (int16_t)(FIELD_W * 0.5f + x * p);
  out->y = (int16_t)(FIELD_H * 0.5f - y * p);
  return 1;
}

static int wait_vdisp(void) {
  struct iocs_time start = _iocs_ontime();
  int level = read_gpip() & GPIP_VDISP;
  for (;;) {
    int next = read_gpip() & GPIP_VDISP;
    if (ontime_diff_cs(start, _iocs_ontime()) > WAIT_VDISP_TIMEOUT_CS) return -1;
    if (next != level) {
      level = next;
      if (level != 0) return 0;
    }
  }
}

static int set_60hz(void) {
  if (wait_vdisp() != 0) return -1;
  write_crtc(CRTC_R05, 0x0001);
  write_crtc(CRTC_R06, 0x0022);
  write_crtc(CRTC_R07, 0x0202);
  write_crtc(CRTC_R04, 0x020c);
  return 0;
}

static int key_down(int scan) {
  return (_iocs_bitsns(scan >> 3) & (1 << (scan & 7))) != 0;
}

static void init_state(GameState *s) {
  s->angle = 0.0f;
  s->frame = 0;
  s->have_prev = 0;
  s->fps_count = 0;
  s->fps_x100 = 0;
  s->fps_ready = 0;
  s->old_mode = _iocs_crtmod(-1);
  for (int i = 0; i < 12; ++i) s->base[i] = kCarModel[i];
  _iocs_crtmod(12);
  _iocs_g_clr_on();
  _iocs_b_curoff();
}

static void shutdown(iocs_color_t old_mode) {
  _iocs_b_curon();
  _iocs_crtmod(old_mode);
}

int main(void) {
  GameState state;
  init_state(&state);

  if (set_60hz() != 0) {
    shutdown(state.old_mode);
    return 0;
  }

  draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);

  for (;;) {
    if (wait_vdisp() != 0) break;
    if (key_down(KEY_ESC)) break;

    update_fps(&state);

    float ax = state.angle * 0.7f;
    float ay = state.angle;

    for (int i = 0; i < 12; ++i) {
      Vec3f p = state.base[i];
      state.curr[i] = p;
      state.visible_next[i] = (uint8_t)project(&p, ax, ay, &state.next[i]);
    }

    if (state.have_prev) {
      draw_wire(state.prev, state.visible_prev, COLOR_BLACK);
    }
    draw_wire(state.next, state.visible_next, COLOR_WHITE);
    draw_fps_hud(&state);

    for (int i = 0; i < 12; ++i) {
      state.prev[i] = state.next[i];
      state.visible_prev[i] = state.visible_next[i];
    }
    state.have_prev = 1;

    state.angle += 0.015f;
    if (state.angle >= 6.2831853f) state.angle -= 6.2831853f;
    ++state.frame;
  }

  draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
  shutdown(state.old_mode);
  return 0;
}

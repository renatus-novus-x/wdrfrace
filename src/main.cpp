#include <stdint.h>
#include <x68k/iocs.h>

#define FIELD_W 512
#define FIELD_H 480

#define KEY_ESC 0x01

#define FPS_WINDOW_FRAMES 300
#define CAR_VERTEX_COUNT 4
#define TRIG_TABLE_SIZE 256
#define TRIG_TABLE_MASK (TRIG_TABLE_SIZE - 1)
#define TRIG_QUARTER (TRIG_TABLE_SIZE / 4)
#define TRIG_INDEX_SCALE 40.74366543f
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
#define COLOR_RED 0x07c1
#define COLOR_GREEN 0xf801
#define COLOR_BLUE 0x003f

#define CAMERA_RADIUS 12.0f
#define CAMERA_HEIGHT 5.0f
#define CAMERA_DISTANCE 13.0f
#define CAMERA_RADIUS_NORMALIZED 0.923076923f
#define CAMERA_HEIGHT_NORMALIZED 0.384615385f
#define DEBUG_AXIS_POINT_COUNT 4

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
  Vec3f base[CAR_VERTEX_COUNT];
  Vec3f curr[CAR_VERTEX_COUNT];
  Vec2s prev[CAR_VERTEX_COUNT];
  Vec2s next[CAR_VERTEX_COUNT];
  uint8_t visible_prev[CAR_VERTEX_COUNT];
  uint8_t visible_next[CAR_VERTEX_COUNT];
  float camera_angle;
  int have_prev;

  struct iocs_time fps_start;
  int fps_count;
  int fps_x100;
  uint8_t fps_ready;
} GameState;

static const Vec3f kCarModel[] = {
  {-1.8f, -0.65f, 0.0f}, {1.8f, -0.65f, 0.0f},
  {1.35f, 0.65f, 0.0f}, {-1.35f, 0.65f, 0.0f},
};

static const Edge kEdges[] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0},
};

static const Vec3f kDebugAxes[DEBUG_AXIS_POINT_COUNT] = {
  {0.0f, 0.0f, 0.0f},
  {1.0f, 0.0f, 0.0f},
  {0.0f, 1.0f, 0.0f},
  {0.0f, 0.0f, 1.0f},
};

static float g_sin_table[TRIG_TABLE_SIZE];

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

static void init_trig_table(void) {
  const float step_sin = 0.024541229f;
  const float step_cos = 0.999698819f;
  float s = 0.0f;
  float c = 1.0f;

  for (int i = 0; i < TRIG_TABLE_SIZE; ++i) {
    g_sin_table[i] = s;
    float next_s = s * step_cos + c * step_sin;
    c = c * step_cos - s * step_sin;
    s = next_s;
  }
}

static inline int trig_index(float angle) {
  return ((int)(angle * TRIG_INDEX_SCALE)) & TRIG_TABLE_MASK;
}

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
  struct iocs_lineptr line;
  line.x1 = (short)x0;
  line.y1 = (short)y0;
  line.x2 = (short)x1;
  line.y2 = (short)y1;
  line.color = color;
  line.linestyle = 0xffff;
  _iocs_line(&line);
}

static void draw_wire(const Vec2s *pt, const uint8_t *visible, iocs_color_t color) {
  const int edge_count = (int)(sizeof(kEdges) / sizeof(kEdges[0]));
  for (int i = 0; i < edge_count; ++i) {
    const Edge e = kEdges[i];
    if (!visible[e.a] || !visible[e.b]) continue;
    draw_line(pt[e.a].x, pt[e.a].y, pt[e.b].x, pt[e.b].y, color);
  }
}

static void draw_debug_axes(const Vec2s *pt, const uint8_t *visible) {
  if (!visible[0]) return;
  if (visible[1]) draw_line(pt[0].x, pt[0].y, pt[1].x, pt[1].y, COLOR_RED);
  if (visible[2]) draw_line(pt[0].x, pt[0].y, pt[2].x, pt[2].y, COLOR_GREEN);
  if (visible[3]) draw_line(pt[0].x, pt[0].y, pt[3].x, pt[3].y, COLOR_BLUE);
}

#ifdef USE_DIRTY_RECT_CLEAR
static void clear_previous_frame(const Vec2s *car, const uint8_t *car_visible,
                                 const Vec2s *axis,
                                 const uint8_t *axis_visible) {
  int min_x = 32767;
  int min_y = 32767;
  int max_x = -32768;
  int max_y = -32768;

  for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
    if (!car_visible[i]) continue;
    if (car[i].x < min_x) min_x = car[i].x;
    if (car[i].y < min_y) min_y = car[i].y;
    if (car[i].x > max_x) max_x = car[i].x;
    if (car[i].y > max_y) max_y = car[i].y;
  }
  for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    if (!axis_visible[i]) continue;
    if (axis[i].x < min_x) min_x = axis[i].x;
    if (axis[i].y < min_y) min_y = axis[i].y;
    if (axis[i].x > max_x) max_x = axis[i].x;
    if (axis[i].y > max_y) max_y = axis[i].y;
  }

  if (max_x < 0 || max_y < 0 || min_x >= FIELD_W || min_y >= FIELD_H) return;
  if (min_x > 0) --min_x;
  if (min_y > 0) --min_y;
  if (max_x < FIELD_W - 1) ++max_x;
  if (max_y < FIELD_H - 1) ++max_y;
  if (min_x < 0) min_x = 0;
  if (min_y < 0) min_y = 0;
  if (max_x >= FIELD_W) max_x = FIELD_W - 1;
  if (max_y >= FIELD_H) max_y = FIELD_H - 1;

  draw_fill_block(min_x, min_y, max_x, max_y, COLOR_BLACK);
}
#else
static void erase_debug_axes(const Vec2s *pt, const uint8_t *visible) {
  if (!visible[0]) return;
  for (int i = 1; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    if (visible[i]) {
      draw_line(pt[0].x, pt[0].y, pt[i].x, pt[i].y, COLOR_BLACK);
    }
  }
}
#endif

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

static int update_fps(GameState *state) {
  if (state->fps_count == 0) {
    state->fps_start = _iocs_ontime();
  }

  ++state->fps_count;
  if (state->fps_count < FPS_WINDOW_FRAMES) return 0;

  struct iocs_time now = _iocs_ontime();
  long dt_cs = ontime_diff_cs(state->fps_start, now);
  if (dt_cs > 0) {
    state->fps_x100 = (int)((FPS_WINDOW_FRAMES * 10000L) / dt_cs);
    state->fps_ready = 1;
  }
  state->fps_count = 0;
  return 1;
}

static int project_world(const Vec3f *in, float cz, float sz, Vec2s *out) {
  float x = in->x * cz - in->z * sz;
  float y = -in->x * sz * CAMERA_HEIGHT_NORMALIZED
          + in->y * CAMERA_RADIUS_NORMALIZED
          - in->z * cz * CAMERA_HEIGHT_NORMALIZED;
  float z = in->x * sz * CAMERA_RADIUS_NORMALIZED
          + in->y * CAMERA_HEIGHT_NORMALIZED
          + in->z * cz * CAMERA_RADIUS_NORMALIZED
          - CAMERA_DISTANCE;

  if (z > -1.0f) return 0;

  float p = 320.0f / -z;
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
  init_trig_table();
  s->camera_angle = 0.0f;
  s->frame = 0;
  s->have_prev = 0;
  s->fps_count = 0;
  s->fps_x100 = 0;
  s->fps_ready = 0;
  s->old_mode = _iocs_crtmod(-1);
  for (int i = 0; i < CAR_VERTEX_COUNT; ++i) s->base[i] = kCarModel[i];
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
  Vec2s debug_axis_points[DEBUG_AXIS_POINT_COUNT];
  Vec2s debug_axis_prev[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible_prev[DEBUG_AXIS_POINT_COUNT];
  init_state(&state);

  if (set_60hz() != 0) {
    shutdown(state.old_mode);
    return 0;
  }

  draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
  draw_fps_hud(&state);

  for (;;) {
    if (wait_vdisp() != 0) break;
    if (key_down(KEY_ESC)) break;

    int fps_updated = update_fps(&state);

    int angle_index = trig_index(state.camera_angle);
    float sz = g_sin_table[angle_index];
    float cz =
        g_sin_table[(angle_index + TRIG_QUARTER) & TRIG_TABLE_MASK];

    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_visible[i] = (uint8_t)project_world(
          &kDebugAxes[i], cz, sz, &debug_axis_points[i]);
    }

    for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
      Vec3f p = state.base[i];
      state.curr[i] = p;
      state.visible_next[i] =
          (uint8_t)project_world(&p, cz, sz, &state.next[i]);
    }

    if (state.have_prev) {
#ifdef USE_DIRTY_RECT_CLEAR
      clear_previous_frame(state.prev, state.visible_prev,
                           debug_axis_prev, debug_axis_visible_prev);
#else
      draw_wire(state.prev, state.visible_prev, COLOR_BLACK);
      erase_debug_axes(debug_axis_prev, debug_axis_visible_prev);
#endif
    }
    draw_wire(state.next, state.visible_next, COLOR_WHITE);
    draw_debug_axes(debug_axis_points, debug_axis_visible);
    if (fps_updated) draw_fps_hud(&state);

    for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
      state.prev[i] = state.next[i];
      state.visible_prev[i] = state.visible_next[i];
    }
    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_prev[i] = debug_axis_points[i];
      debug_axis_visible_prev[i] = debug_axis_visible[i];
    }
    state.have_prev = 1;

    state.camera_angle += 0.015f;
    if (state.camera_angle >= 6.2831853f) {
      state.camera_angle -= 6.2831853f;
    }
    ++state.frame;
  }

  draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
  shutdown(state.old_mode);
  return 0;
}

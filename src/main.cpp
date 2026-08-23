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
  Vec3f base[CAR_VERTEX_COUNT];
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
  {-7.0f, -5.0f, 0.0f}, {7.0f, -5.0f, 0.0f},
  {5.5f, 5.0f, 0.0f}, {-5.5f, 5.0f, 0.0f},
};

static const Edge kEdges[] = {
  {0, 1}, {1, 2}, {2, 3}, {3, 0},
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

enum GameModeId {
  GAME_MODE_TEST,
  GAME_MODE_EXIT,
};

class GameMode {
 public:
  virtual int initialize() { return 1; }
  virtual GameModeId update() { return GAME_MODE_EXIT; }
  virtual void finalize() {}
};

class GameModeTest : public GameMode {
 private:
  GameState state_;
  Vec3f debug_axis_model_[DEBUG_AXIS_POINT_COUNT];
  Vec2s debug_axis_points_[DEBUG_AXIS_POINT_COUNT];
  Vec2s debug_axis_prev_[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible_[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible_prev_[DEBUG_AXIS_POINT_COUNT];

  void initialize_debug_axis() {
    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_model_[i].x = 0.0f;
      debug_axis_model_[i].y = 0.0f;
      debug_axis_model_[i].z = 0.0f;
    }
    debug_axis_model_[1].x = 1.0f;
    debug_axis_model_[2].y = 1.0f;
    debug_axis_model_[3].z = 1.0f;
  }

  void project_debug_axis(float cz, float sz) {
    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_visible_[i] = (uint8_t)project_world(
          &debug_axis_model_[i], cz, sz, &debug_axis_points_[i]);
    }
  }

  void draw_debug_axis() const {
    if (!debug_axis_visible_[0]) return;
    if (debug_axis_visible_[1]) {
      draw_line(debug_axis_points_[0].x, debug_axis_points_[0].y,
                debug_axis_points_[1].x, debug_axis_points_[1].y, COLOR_RED);
    }
    if (debug_axis_visible_[2]) {
      draw_line(debug_axis_points_[0].x, debug_axis_points_[0].y,
                debug_axis_points_[2].x, debug_axis_points_[2].y, COLOR_GREEN);
    }
    if (debug_axis_visible_[3]) {
      draw_line(debug_axis_points_[0].x, debug_axis_points_[0].y,
                debug_axis_points_[3].x, debug_axis_points_[3].y, COLOR_BLUE);
    }
  }

  void erase_previous_frame() {
    if (!state_.have_prev) return;
    draw_wire(state_.prev, state_.visible_prev, COLOR_BLACK);
    if (debug_axis_visible_prev_[0]) {
      for (int i = 1; i < DEBUG_AXIS_POINT_COUNT; ++i) {
        if (debug_axis_visible_prev_[i]) {
          draw_line(debug_axis_prev_[0].x, debug_axis_prev_[0].y,
                    debug_axis_prev_[i].x, debug_axis_prev_[i].y, COLOR_BLACK);
        }
      }
    }
  }

  void save_previous_frame() {
    for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
      state_.prev[i] = state_.next[i];
      state_.visible_prev[i] = state_.visible_next[i];
    }
    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_prev_[i] = debug_axis_points_[i];
      debug_axis_visible_prev_[i] = debug_axis_visible_[i];
    }
    state_.have_prev = 1;
  }

 public:
  virtual int initialize() {
    init_trig_table();
    state_.camera_angle = 0.0f;
    state_.frame = 0;
    state_.have_prev = 0;
    state_.fps_count = 0;
    state_.fps_x100 = 0;
    state_.fps_ready = 0;
    for (int i = 0; i < CAR_VERTEX_COUNT; ++i) state_.base[i] = kCarModel[i];
    initialize_debug_axis();
    draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
    draw_fps_hud(&state_);
    return 1;
  }

  virtual GameModeId update() {
    if (key_down(KEY_ESC)) return GAME_MODE_EXIT;

    int fps_updated = update_fps(&state_);
    int angle_index = trig_index(state_.camera_angle);
    float sz = g_sin_table[angle_index];
    float cz =
        g_sin_table[(angle_index + TRIG_QUARTER) & TRIG_TABLE_MASK];

    project_debug_axis(cz, sz);
    for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
      state_.visible_next[i] = (uint8_t)project_world(
          &state_.base[i], cz, sz, &state_.next[i]);
    }

    erase_previous_frame();
    draw_wire(state_.next, state_.visible_next, COLOR_WHITE);
    draw_debug_axis();
    if (fps_updated) draw_fps_hud(&state_);
    save_previous_frame();

    state_.camera_angle += 0.015f;
    if (state_.camera_angle >= 6.2831853f) {
      state_.camera_angle -= 6.2831853f;
    }
    ++state_.frame;
    return GAME_MODE_TEST;
  }

  virtual void finalize() {}
};

class Application {
 private:
  int old_mode_;
  GameModeId current_mode_id_;
  GameMode *current_mode_;
  GameModeTest test_mode_;

  GameMode *mode_for(GameModeId id) {
    if (id == GAME_MODE_TEST) return &test_mode_;
    return 0;
  }

 public:
  int application_initialize() {
    current_mode_ = 0;
    old_mode_ = _iocs_crtmod(-1);
    _iocs_crtmod(12);
    _iocs_g_clr_on();
    _iocs_b_curoff();
    if (set_60hz() != 0) {
      application_finalize();
      return 0;
    }

    current_mode_id_ = GAME_MODE_TEST;
    current_mode_ = mode_for(current_mode_id_);
    if (!current_mode_ || !current_mode_->initialize()) {
      application_finalize();
      return 0;
    }
    return 1;
  }

  int application_update() {
    if (wait_vdisp() != 0 || !current_mode_) return 0;
    GameModeId next = current_mode_->update();
    if (next == current_mode_id_) return 1;

    current_mode_->finalize();
    current_mode_ = 0;
    current_mode_id_ = next;
    if (next == GAME_MODE_EXIT) return 0;

    current_mode_ = mode_for(next);
    return current_mode_ && current_mode_->initialize();
  }

  void application_finalize() {
    if (current_mode_) {
      current_mode_->finalize();
      current_mode_ = 0;
    }
    draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
    _iocs_b_curon();
    int mode = old_mode_;
    if (mode < 0 || mode > 0x7f) mode = 12;
    _iocs_crtmod(mode);
  }
};

int main(void) {
  Application application;
  if (!application.application_initialize()) return 0;
  while (application.application_update()) {}
  application.application_finalize();
  return 0;
}

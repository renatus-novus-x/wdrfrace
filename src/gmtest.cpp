#include "gmtest.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const int KEY_ESC = 0x01;
const int KEY_D = 0x20;
const int FPS_WINDOW_FRAMES = 300;
const int HUD_X = 8;
const int HUD_Y = 8;
const int HUD_W = 170;
const int HUD_H = 28;
const long CENTISEC_PER_DAY = 8640000L;

const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_RED = 0x07c1;
const iocs_color_t COLOR_GREEN = 0xf801;
const iocs_color_t COLOR_BLUE = 0x003f;

const float CAMERA_DISTANCE = 13.0f;
const float CAMERA_RADIUS_NORMALIZED = 0.923076923f;
const float CAMERA_HEIGHT_NORMALIZED = 0.384615385f;
const float TRIG_INDEX_SCALE = 40.74366543f;
const float TWO_PI = 6.2831853f;

const uint8_t kDigits4x5[10][5] = {
  {0x0E, 0x0A, 0x0A, 0x0A, 0x0E},
  {0x04, 0x0C, 0x04, 0x04, 0x0E},
  {0x0E, 0x01, 0x0E, 0x08, 0x0E},
  {0x0E, 0x01, 0x06, 0x01, 0x0E},
  {0x08, 0x08, 0x0E, 0x01, 0x01},
  {0x0F, 0x08, 0x0E, 0x01, 0x0E},
  {0x06, 0x08, 0x0E, 0x0A, 0x06},
  {0x0F, 0x01, 0x02, 0x04, 0x08},
  {0x0E, 0x0A, 0x06, 0x0A, 0x0E},
  {0x06, 0x0A, 0x0E, 0x01, 0x06},
};

const uint8_t kGlyphF[5] = {0x0F, 0x08, 0x0C, 0x08, 0x08};
const uint8_t kGlyphP[5] = {0x0E, 0x0A, 0x0E, 0x08, 0x08};
const uint8_t kGlyphS[5] = {0x07, 0x08, 0x0E, 0x01, 0x0E};

}  // namespace

void GameModeTest::initialize_trig_table() {
  const float step_sin = 0.024541229f;
  const float step_cos = 0.999698819f;
  float s = 0.0f;
  float c = 1.0f;
  for (int i = 0; i < TRIG_TABLE_SIZE; ++i) {
    sin_table_[i] = s;
    float next_s = s * step_cos + c * step_sin;
    c = c * step_cos - s * step_sin;
    s = next_s;
  }
}

int GameModeTest::trig_index(float angle) const {
  return ((int)(angle * TRIG_INDEX_SCALE)) & (TRIG_TABLE_SIZE - 1);
}

long GameModeTest::ontime_diff_cs(struct iocs_time start,
                                  struct iocs_time end) const {
  return ((long)end.day - (long)start.day) * CENTISEC_PER_DAY
       + (long)end.sec - (long)start.sec;
}

void GameModeTest::initialize_car() {
  state_.base[0].x = -7.0f; state_.base[0].y = -5.0f; state_.base[0].z = 0.0f;
  state_.base[1].x =  7.0f; state_.base[1].y = -5.0f; state_.base[1].z = 0.0f;
  state_.base[2].x =  5.5f; state_.base[2].y =  5.0f; state_.base[2].z = 0.0f;
  state_.base[3].x = -5.5f; state_.base[3].y =  5.0f; state_.base[3].z = 0.0f;
}

void GameModeTest::initialize_debug_axis() {
  for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    debug_axis_model_[i].x = 0.0f;
    debug_axis_model_[i].y = 0.0f;
    debug_axis_model_[i].z = 0.0f;
  }
  debug_axis_model_[1].x = 1.0f;
  debug_axis_model_[2].y = 1.0f;
  debug_axis_model_[3].z = 1.0f;
}

int GameModeTest::project_world(const Vec3f &in, float cz, float sz,
                                Vec2s &out) const {
  float x = in.x * cz - in.z * sz;
  float y = -in.x * sz * CAMERA_HEIGHT_NORMALIZED
          + in.y * CAMERA_RADIUS_NORMALIZED
          - in.z * cz * CAMERA_HEIGHT_NORMALIZED;
  float z = in.x * sz * CAMERA_RADIUS_NORMALIZED
          + in.y * CAMERA_HEIGHT_NORMALIZED
          + in.z * cz * CAMERA_RADIUS_NORMALIZED
          - CAMERA_DISTANCE;
  if (z > -1.0f) return 0;
  float p = 320.0f / -z;
  out.x = (int16_t)(FIELD_W * 0.5f + x * p);
  out.y = (int16_t)(FIELD_H * 0.5f - y * p);
  return 1;
}

void GameModeTest::project_debug_axis(float cz, float sz) {
  for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    debug_axis_visible_[i] = (uint8_t)project_world(
        debug_axis_model_[i], cz, sz, debug_axis_points_[i]);
  }
}

int GameModeTest::key_down(int scan) const {
  return (_iocs_bitsns(scan >> 3) & (1 << (scan & 7))) != 0;
}

void GameModeTest::draw_fill_block(int x1, int y1, int x2, int y2,
                                   iocs_color_t color) const {
  struct iocs_fillptr rect;
  rect.x1 = (short)x1;
  rect.y1 = (short)y1;
  rect.x2 = (short)x2;
  rect.y2 = (short)y2;
  rect.color = color;
  _iocs_fill(&rect);
}

void GameModeTest::draw_pixel(int x, int y, iocs_color_t color) const {
  if (x < 0 || y < 0 || x >= FIELD_W || y >= FIELD_H) return;
  draw_fill_block(x, y, x, y, color);
}

void GameModeTest::draw_line(int x0, int y0, int x1, int y1,
                             iocs_color_t color) const {
  struct iocs_lineptr line;
  line.x1 = (short)x0;
  line.y1 = (short)y0;
  line.x2 = (short)x1;
  line.y2 = (short)y1;
  line.color = color;
  line.linestyle = 0xffff;
  _iocs_line(&line);
}

void GameModeTest::draw_wire(const Vec2s *points, const uint8_t *visible,
                             iocs_color_t color) const {
  static const uint8_t edge_a[4] = {0, 1, 2, 3};
  static const uint8_t edge_b[4] = {1, 2, 3, 0};
  for (int i = 0; i < 4; ++i) {
    int a = edge_a[i];
    int b = edge_b[i];
    if (visible[a] && visible[b]) {
      draw_line(points[a].x, points[a].y, points[b].x, points[b].y, color);
    }
  }
}

void GameModeTest::draw_debug_axis() const {
  if (!debug_axis_visible_[0]) return;
  const iocs_color_t colors[3] = {COLOR_RED, COLOR_GREEN, COLOR_BLUE};
  for (int i = 1; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    if (debug_axis_visible_[i]) {
      draw_line(debug_axis_points_[0].x, debug_axis_points_[0].y,
                debug_axis_points_[i].x, debug_axis_points_[i].y,
                colors[i - 1]);
    }
  }
}

void GameModeTest::erase_previous_frame() {
  if (!state_.have_prev) return;
  draw_wire(state_.prev, state_.visible_prev, COLOR_BLACK);
  if (!debug_axis_visible_prev_[0]) return;
  for (int i = 1; i < DEBUG_AXIS_POINT_COUNT; ++i) {
    if (debug_axis_visible_prev_[i]) {
      draw_line(debug_axis_prev_[0].x, debug_axis_prev_[0].y,
                debug_axis_prev_[i].x, debug_axis_prev_[i].y, COLOR_BLACK);
    }
  }
}

void GameModeTest::save_previous_frame() {
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

const uint8_t *GameModeTest::glyph_for_char(char c) const {
  static const uint8_t space[5] = {0, 0, 0, 0, 0};
  static const uint8_t dot[5] = {0, 0, 0, 0, 4};
  static const uint8_t colon[5] = {0, 4, 0, 4, 0};
  if (c >= '0' && c <= '9') return kDigits4x5[c - '0'];
  if (c == ' ') return space;
  if (c == '.') return dot;
  if (c == ':') return colon;
  if (c == 'F') return kGlyphF;
  if (c == 'P') return kGlyphP;
  if (c == 'S') return kGlyphS;
  return space;
}

void GameModeTest::draw_glyph(int x, int y, char c,
                              iocs_color_t color) const {
  const uint8_t *glyph = glyph_for_char(c);
  for (int row = 0; row < 5; ++row) {
    for (int col = 0; col < 4; ++col) {
      if (glyph[row] & (1 << (3 - col))) draw_pixel(x + col, y + row, color);
    }
  }
}

void GameModeTest::draw_text4x5(int x, int y, const char *text,
                                iocs_color_t color) const {
  while (*text) {
    draw_glyph(x, y, *text++, color);
    x += 6;
  }
}

void GameModeTest::draw_fps_hud() const {
  draw_fill_block(HUD_X, HUD_Y, HUD_X + HUD_W, HUD_Y + HUD_H, COLOR_BLACK);
  draw_text4x5(HUD_X, HUD_Y, "FPS:", COLOR_WHITE);
  int x = HUD_X + 24;
  if (!state_.fps_ready) {
    draw_text4x5(x, HUD_Y, "--.--", COLOR_WHITE);
    return;
  }
  int value = state_.fps_x100;
  int integer = value / 100;
  int fraction = value % 100;
  if (integer >= 100) {
    draw_glyph(x + 6, HUD_Y, (char)('0' + (integer / 100) % 10), COLOR_WHITE);
    draw_glyph(x + 12, HUD_Y, (char)('0' + (integer / 10) % 10), COLOR_WHITE);
    draw_glyph(x + 18, HUD_Y, (char)('0' + integer % 10), COLOR_WHITE);
    x += 24;
  } else if (integer >= 10) {
    draw_glyph(x + 6, HUD_Y, (char)('0' + integer / 10), COLOR_WHITE);
    draw_glyph(x + 12, HUD_Y, (char)('0' + integer % 10), COLOR_WHITE);
    x += 18;
  } else {
    draw_glyph(x + 6, HUD_Y, (char)('0' + integer), COLOR_WHITE);
    x += 12;
  }
  draw_glyph(x, HUD_Y, '.', COLOR_WHITE);
  draw_glyph(x + 6, HUD_Y, (char)('0' + fraction / 10), COLOR_WHITE);
  draw_glyph(x + 12, HUD_Y, (char)('0' + fraction % 10), COLOR_WHITE);
}

int GameModeTest::update_fps() {
  if (state_.fps_count == 0) state_.fps_start = _iocs_ontime();
  if (++state_.fps_count < FPS_WINDOW_FRAMES) return 0;
  struct iocs_time now = _iocs_ontime();
  long dt_cs = ontime_diff_cs(state_.fps_start, now);
  if (dt_cs > 0) {
    state_.fps_x100 = (int)((FPS_WINDOW_FRAMES * 10000L) / dt_cs);
    state_.fps_ready = 1;
  }
  state_.fps_count = 0;
  return 1;
}

int GameModeTest::initialize() {
  initialize_trig_table();
  initialize_car();
  initialize_debug_axis();
  state_.camera_angle = 0.0f;
  state_.frame = 0;
  state_.have_prev = 0;
  state_.fps_count = 0;
  state_.fps_x100 = 0;
  state_.fps_ready = 0;
  debug_visible_ = 1;
  debug_toggle_down_ = 0;
  debug_visibility_changed_ = 0;
  fps_updated_ = 0;
  draw_fill_block(0, 0, FIELD_W - 1, FIELD_H - 1, COLOR_BLACK);
  draw_fps_hud();
  return 1;
}

GameModeId GameModeTest::update() {
  if (key_down(KEY_ESC)) return GAME_MODE_EXIT;

  int debug_toggle = key_down(KEY_D);
  debug_visibility_changed_ = 0;
  if (debug_toggle && !debug_toggle_down_) {
    debug_visible_ = !debug_visible_;
    debug_visibility_changed_ = 1;
  }
  debug_toggle_down_ = debug_toggle;

  fps_updated_ = update_fps();
  int angle_index = trig_index(state_.camera_angle);
  float sz = sin_table_[angle_index];
  float cz = sin_table_[(angle_index + TRIG_TABLE_SIZE / 4)
                        & (TRIG_TABLE_SIZE - 1)];
  if (debug_visible_) {
    project_debug_axis(cz, sz);
  } else {
    for (int i = 0; i < DEBUG_AXIS_POINT_COUNT; ++i) {
      debug_axis_visible_[i] = 0;
    }
  }
  for (int i = 0; i < CAR_VERTEX_COUNT; ++i) {
    state_.visible_next[i] = (uint8_t)project_world(
        state_.base[i], cz, sz, state_.next[i]);
  }
  state_.camera_angle += 0.015f;
  if (state_.camera_angle >= TWO_PI) state_.camera_angle -= TWO_PI;
  ++state_.frame;
  return GAME_MODE_TEST;
}

void GameModeTest::render() {
  erase_previous_frame();
  draw_wire(state_.next, state_.visible_next, COLOR_WHITE);
  if (debug_visible_) draw_debug_axis();
  if (debug_visibility_changed_) {
    if (debug_visible_) {
      draw_fps_hud();
    } else {
      draw_fill_block(HUD_X, HUD_Y, HUD_X + HUD_W, HUD_Y + HUD_H,
                      COLOR_BLACK);
    }
  } else if (debug_visible_ && fps_updated_) {
    draw_fps_hud();
  }
  save_previous_frame();
  fps_updated_ = 0;
  debug_visibility_changed_ = 0;
}

void GameModeTest::finalize() {}

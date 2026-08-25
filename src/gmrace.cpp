#include "gmrace.h"

#include "introdat.h"
#include "screen.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_TRACK = 0xffff;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;

void draw_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  screen_line(x0, y0, x1, y1, color);
}

}  // namespace

void GameModeRace::initialize_trig_table() {
  const float step_sin = 0.024541229f;
  const float step_cos = 0.999698819f;
  float s = 0.0f;
  float c = 1.0f;
  for (int i = 0; i < TRIG_TABLE_SIZE; ++i) {
    sin_table_[i] = s;
    const float next_s = s * step_cos + c * step_sin;
    c = c * step_cos - s * step_sin;
    s = next_s;
  }
}

void GameModeRace::prepare_intro_frame(int frame) {
  for (int i = 0; i < PLAYER_COUNT; ++i) {
    cars_[i].prepare_screen(kIntroFrames[frame].cars[i]);
  }
}

void GameModeRace::draw_ring(const Vec2s *ring,
                             iocs_color_t color) const {
  for (int i = 0; i < TRACK_SEGMENTS; ++i) {
    const int next = (i + 1) % TRACK_SEGMENTS;
    draw_line(ring[i].x, ring[i].y, ring[next].x, ring[next].y, color);
  }
}

void GameModeRace::draw_track(
    const Vec2s track[2][TRACK_SEGMENTS], iocs_color_t color) const {
  draw_ring(track[0], color);
  draw_ring(track[1], color);
  draw_start_line(track, color);
}

void GameModeRace::draw_start_line(
    const Vec2s track[2][TRACK_SEGMENTS], iocs_color_t color) const {
  draw_line(track[0][0].x, track[0][0].y,
            track[1][0].x, track[1][0].y, color);
}

int GameModeRace::edge_intersects(const ScreenRect &bounds,
                                  const Vec2s &a, const Vec2s &b) const {
  if (!bounds.valid) return 0;
  const int min_x = a.x < b.x ? a.x : b.x;
  const int max_x = a.x > b.x ? a.x : b.x;
  const int min_y = a.y < b.y ? a.y : b.y;
  const int max_y = a.y > b.y ? a.y : b.y;
  return max_x >= bounds.min_x && min_x <= bounds.max_x &&
         max_y >= bounds.min_y && min_y <= bounds.max_y;
}

void GameModeRace::repair_track(
    const Vec2s track[2][TRACK_SEGMENTS],
    const ScreenRect *damage, int damage_count) const {
  for (int ring = 0; ring < 2; ++ring) {
    for (int i = 0; i < TRACK_SEGMENTS; ++i) {
      const int next = (i + 1) % TRACK_SEGMENTS;
      int repair = 0;
      for (int car = 0; car < damage_count; ++car) {
        if (edge_intersects(damage[car], track[ring][i],
                            track[ring][next])) {
          repair = 1;
          break;
        }
      }
      if (repair) {
        draw_line(track[ring][i].x, track[ring][i].y,
                  track[ring][next].x, track[ring][next].y, COLOR_TRACK);
      }
    }
  }

  for (int car = 0; car < damage_count; ++car) {
    if (edge_intersects(damage[car], track[0][0], track[1][0])) {
      draw_start_line(track, COLOR_TRACK);
      break;
    }
  }
}

void GameModeRace::clear_screen() const {
  screen_clear(COLOR_BLACK);
}

int GameModeRace::initialize() {
  initialize_trig_table();
  intro_frame_ = 0;
  intro_drawn_frame_[0] = -1;
  intro_drawn_frame_[1] = -1;

  const Vec3f eye = {0.0f, 11.0f, 14.0f};
  const Vec3f target = {0.0f, 0.0f, 0.0f};
  const Vec3f up = {0.0f, 1.0f, 0.0f};
  if (!camera_.look_at(eye, target, up)) return 0;

  cars_[0].initialize(0, -42);
  cars_[1].initialize(0, 42);
  return 1;
}

GameModeId GameModeRace::update() {
  input_.update();
  if (input_.quit()) return GAME_MODE_TITLE;
  if (intro_frame_ + 1 < INTRO_FRAME_COUNT) {
    ++intro_frame_;
    return GAME_MODE_RACE;
  }
  for (int i = 0; i < PLAYER_COUNT; ++i) {
    cars_[i].update(input_.car_input(i));
  }
  return GAME_MODE_RACE;
}

void GameModeRace::render(int page) {
  if (intro_drawn_frame_[page] < 0) {
    clear_screen();
    draw_track(kIntroFrames[intro_frame_].track, COLOR_TRACK);
    prepare_intro_frame(intro_frame_);
    cars_[0].render(page, COLOR_P1);
    cars_[1].render(page, COLOR_P2);
    intro_drawn_frame_[page] = intro_frame_;
    return;
  }

  if (intro_drawn_frame_[page] != intro_frame_) {
    const IntroFrame &next = kIntroFrames[intro_frame_];

    clear_screen();
    draw_track(next.track, COLOR_TRACK);
    prepare_intro_frame(intro_frame_);
    cars_[0].render(page, COLOR_P1);
    cars_[1].render(page, COLOR_P2);
    intro_drawn_frame_[page] = intro_frame_;
    return;
  }

  if (intro_frame_ + 1 < INTRO_FRAME_COUNT) return;

  ScreenRect damage[PLAYER_COUNT];
  for (int i = 0; i < PLAYER_COUNT; ++i) {
    damage[i] = cars_[i].previous_bounds(page);
    cars_[i].prepare_render(camera_, sin_table_);
  }
  for (int i = 0; i < PLAYER_COUNT; ++i) cars_[i].clear_previous(page);
  repair_track(kIntroFrames[INTRO_FRAME_COUNT - 1].track,
               damage, PLAYER_COUNT);
  cars_[0].render(page, COLOR_P1);
  cars_[1].render(page, COLOR_P2);
}

void GameModeRace::finalize() {}

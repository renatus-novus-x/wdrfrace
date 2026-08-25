#include "gmdemo.h"

#include "demodat.h"
#include "screen.h"
#include "vtext.h"

namespace {

const int REPLAY_X = 22;
const int REPLAY_Y = 70;
const int REPLAY_W = 468;
const int REPLAY_H = 342;
const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_CYAN = 0xf83f;
const iocs_color_t COLOR_TRACK = 0x7bef;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;

}  // namespace

void GameModeDemo::draw_scene() const {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 8, "AUTO REPLAY", 1, 1, COLOR_CYAN);
  screen_text(466, 8, "02", 1, COLOR_WHITE);
  screen_line(24, 22, 488, 22, COLOR_TRACK);
  vector_centered("DEMO REPLAY", 32, 6, 2, 2, COLOR_CYAN);
  screen_line(176, 82, 336, 82, COLOR_TRACK);
  screen_centered_tracking("PRESS ANY KEY", 452, 1, 3, COLOR_WHITE);
}

void GameModeDemo::clear_replay() const {
  screen_fill(REPLAY_X, REPLAY_Y, REPLAY_W, REPLAY_H, COLOR_BLACK);
}

void GameModeDemo::draw_ring(const Vec2s *ring,
                             iocs_color_t color) const {
  for (int i = 0; i < DEMO_TRACK_SEGMENTS; ++i) {
    const int next = (i + 1) % DEMO_TRACK_SEGMENTS;
    screen_line(ring[i].x, ring[i].y,
                ring[next].x, ring[next].y, color);
  }
}

void GameModeDemo::draw_car(const Vec2s *car,
                            iocs_color_t color) const {
  for (int i = 0; i < 4; ++i) {
    const int next = (i + 1) & 3;
    screen_line(car[i].x, car[i].y,
                car[next].x, car[next].y, color);
  }
}

void GameModeDemo::draw_replay_frame(int previous_frame,
                                     int next_frame) const {
  const DemoFrame &previous = kDemoFrames[previous_frame];
  const DemoFrame &next = kDemoFrames[next_frame];
  const int crossed_cut =
      previous_frame / DEMO_SHOT_LENGTH != next_frame / DEMO_SHOT_LENGTH;
  if ((next.flags & DEMO_FLAG_CUT) || crossed_cut) {
    clear_replay();
    draw_ring(next.track[0], COLOR_TRACK);
    draw_ring(next.track[1], COLOR_TRACK);
  } else {
    draw_car(previous.cars[0], COLOR_BLACK);
    draw_car(previous.cars[1], COLOR_BLACK);
    if (next.flags & DEMO_FLAG_TRACK_MOVED) {
      draw_ring(previous.track[0], COLOR_BLACK);
      draw_ring(next.track[0], COLOR_TRACK);
      draw_ring(previous.track[1], COLOR_BLACK);
      draw_ring(next.track[1], COLOR_TRACK);
    } else {
      draw_ring(next.track[0], COLOR_TRACK);
      draw_ring(next.track[1], COLOR_TRACK);
    }
  }
  draw_car(next.cars[0], COLOR_P1);
  draw_car(next.cars[1], COLOR_P2);
}

int GameModeDemo::initialize() {
  input_.update();
  input_released_ = !input_.any_key();
  frame_ = 0;
  drawn_frame_[0] = -1;
  drawn_frame_[1] = -1;
  return 1;
}

GameModeId GameModeDemo::update() {
  input_.update();
  if (!input_released_) {
    if (!input_.any_key()) input_released_ = 1;
  } else if (input_.any_key()) {
    return GAME_MODE_TITLE;
  }

  if (frame_ + 1 >= DEMO_FRAME_COUNT) return GAME_MODE_TITLE;
  ++frame_;
  return GAME_MODE_DEMO;
}

void GameModeDemo::render(int page) {
  if (drawn_frame_[page] < 0) {
    draw_scene();
    clear_replay();
    draw_ring(kDemoFrames[frame_].track[0], COLOR_TRACK);
    draw_ring(kDemoFrames[frame_].track[1], COLOR_TRACK);
    draw_car(kDemoFrames[frame_].cars[0], COLOR_P1);
    draw_car(kDemoFrames[frame_].cars[1], COLOR_P2);
  } else if (drawn_frame_[page] != frame_) {
    draw_replay_frame(drawn_frame_[page], frame_);
  }
  drawn_frame_[page] = frame_;
}

void GameModeDemo::finalize() {}

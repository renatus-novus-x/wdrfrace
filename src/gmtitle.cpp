#include "gmtitle.h"

#include <x68k/iocs.h>

#include "herodat.h"
#include "screen.h"

namespace {

const int COLOR_BLACK = 0x0000;
const int COLOR_WHITE = 0xffff;
const int COLOR_CYAN = 0x07ff;
const int COLOR_FLOOR = 0x39cf;
const int COLOR_LIGHT_DIM = 0x2109;
const int COLOR_LIGHT_BRIGHT = 0xf83f;
const int HIDDEN_X = -32768;
const int DEMO_IDLE_FRAMES = 200;

const int kCarColors[2][HERO_PULSE_COUNT] = {
  {0x1bc7, 0x3dcf, 0x67d9, 0xa7e9, 0xdff7, 0xa7e9, 0x67d9, 0x3dcf},
  {0x211f, 0x422f, 0x62bf, 0xa4bf, 0xde7f, 0xa4bf, 0x62bf, 0x422f},
};

void line(const Vec2s &a, const Vec2s &b, int color) {
  if (a.x == HIDDEN_X || b.x == HIDDEN_X) {
    return;
  }
  struct iocs_lineptr p;
  p.x1 = a.x;
  p.y1 = a.y;
  p.x2 = b.x;
  p.y2 = b.y;
  p.color = color;
  p.linestyle = 0xffff;
  _iocs_line(&p);
}

}  // namespace

GameModeTitle::GameModeTitle()
    : confirm_down_(0),
      prompt_frame_(0),
      prompt_visible_(1),
      prompt_changed_(1),
      idle_frames_(0),
      frame_(0),
      drawn_frame_(-1),
      frame_changed_(1),
      cut_markers_visible_(0) {}

int GameModeTitle::initialize() {
  confirm_down_ = 0;
  prompt_frame_ = 0;
  prompt_visible_ = 1;
  prompt_changed_ = 1;
  idle_frames_ = 0;
  frame_ = 0;
  drawn_frame_ = -1;
  frame_changed_ = 1;
  cut_markers_visible_ = 0;
  input_.update();
  draw_scene();
  return 1;
}

GameModeId GameModeTitle::update() {
  input_.update();
  const int confirm = input_.confirm();
  if (confirm && !confirm_down_) {
    return GAME_MODE_HOW_TO_PLAY;
  }
  confirm_down_ = confirm;

  if (input_.any_key()) {
    idle_frames_ = 0;
  } else if (++idle_frames_ >= DEMO_IDLE_FRAMES) {
    return GAME_MODE_DEMO;
  }

  ++prompt_frame_;
  if (prompt_frame_ >= 10) {
    prompt_frame_ = 0;
    prompt_visible_ = !prompt_visible_;
    prompt_changed_ = 1;
  }

  if (drawn_frame_ == frame_) {
    frame_ = (frame_ + 1) % HERO_FRAME_COUNT;
    frame_changed_ = 1;
  }
  return GAME_MODE_TITLE;
}

void GameModeTitle::render() {
  if (frame_changed_) {
    const int previous_index = drawn_frame_ < 0 ? frame_ : drawn_frame_;
    draw_garage_frame(kHeroFrames[previous_index], kHeroFrames[frame_]);
    drawn_frame_ = frame_;
    frame_changed_ = 0;
  }
  if (prompt_changed_) {
    draw_prompt(prompt_visible_ ? COLOR_WHITE : COLOR_BLACK);
    prompt_changed_ = 0;
  }
}

void GameModeTitle::finalize() {
}

void GameModeTitle::draw_scene() {
  screen_clear(COLOR_BLACK);
  screen_centered("WIRE DRIFT", 22, 5, 0x2108);
  screen_centered("WIRE DRIFT", 20, 5, COLOR_WHITE);
  screen_centered("RACERS", 66, 5, 0x2108);
  screen_centered("RACERS", 64, 5, COLOR_CYAN);
}

void GameModeTitle::draw_prompt(int color) {
  screen_centered("PRESS SPACE", 442, 2, color);
}

void GameModeTitle::clear_garage() {
  for (int y = 124; y <= 393; ++y) {
    Vec2s a = {16, (short)y};
    Vec2s b = {495, (short)y};
    line(a, b, COLOR_BLACK);
  }
}

void GameModeTitle::draw_edges(const Vec2s (*edges)[2], int count,
                               int color) {
  for (int i = 0; i < count; ++i) {
    line(edges[i][0], edges[i][1], color);
  }
}

void GameModeTitle::draw_one_edge(const Vec2s (*edges)[2], int index,
                                  int color) {
  line(edges[index][0], edges[index][1], color);
}

int GameModeTitle::car_color(int car, int pulse) const {
  return kCarColors[car][pulse];
}

int GameModeTitle::car_edge_index(int car, int phase) const {
  return car == 0 ? phase : HERO_CAR_EDGES - 1 - phase;
}

void GameModeTitle::draw_static_shot(const HeroFrame &frame) {
  const HeroShot &shot = kHeroShots[frame.shot];
  draw_edges(shot.floor[0], HERO_FLOOR_EDGES, COLOR_FLOOR);
  draw_edges(shot.floor[1], HERO_FLOOR_EDGES, COLOR_FLOOR);
  draw_edges(shot.lights, HERO_LIGHT_COUNT, COLOR_LIGHT_DIM);
  for (int car = 0; car < 2; ++car) {
    draw_edges(shot.cars[car], HERO_CAR_EDGES,
               car_color(car, frame.pulse_phase));
    draw_one_edge(shot.cars[car], car_edge_index(car, frame.edge_phase),
                  COLOR_WHITE);
  }
  draw_one_edge(shot.lights, frame.light_phase, COLOR_LIGHT_BRIGHT);
}

void GameModeTitle::draw_cut_markers(int color) {
  Vec2s marker[8][2] = {
    {{16, 136}, {31, 136}}, {{16, 136}, {16, 151}},
    {{480, 136}, {495, 136}}, {{495, 136}, {495, 151}},
    {{16, 366}, {16, 381}}, {{16, 381}, {31, 381}},
    {{495, 366}, {495, 381}}, {{480, 381}, {495, 381}},
  };
  draw_edges(marker, 8, color);
}

void GameModeTitle::draw_garage_frame(const HeroFrame &previous,
                                      const HeroFrame &next) {
  if (cut_markers_visible_) {
    draw_cut_markers(COLOR_BLACK);
    cut_markers_visible_ = 0;
  }

  if (next.flags & HERO_FLAG_CUT) {
    clear_garage();
    draw_static_shot(next);
    draw_cut_markers(COLOR_CYAN);
    cut_markers_visible_ = 1;
    return;
  }

  const HeroShot &shot = kHeroShots[next.shot];
  const int pulse_changed = previous.pulse_phase != next.pulse_phase;
  const int edge_changed = previous.edge_phase != next.edge_phase;
  if (pulse_changed) {
    for (int car = 0; car < 2; ++car) {
      draw_edges(shot.cars[car], HERO_CAR_EDGES,
                 car_color(car, next.pulse_phase));
    }
  } else if (edge_changed) {
    for (int car = 0; car < 2; ++car) {
      draw_one_edge(shot.cars[car],
                    car_edge_index(car, previous.edge_phase),
                    car_color(car, next.pulse_phase));
    }
  }

  if (previous.light_phase != next.light_phase) {
    draw_one_edge(shot.lights, previous.light_phase, COLOR_LIGHT_DIM);
    draw_one_edge(shot.lights, next.light_phase, COLOR_LIGHT_BRIGHT);
  }

  if (pulse_changed || edge_changed) {
    for (int car = 0; car < 2; ++car) {
      draw_one_edge(shot.cars[car], car_edge_index(car, next.edge_phase),
                    COLOR_WHITE);
    }
  }
}

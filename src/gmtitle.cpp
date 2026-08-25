#include "gmtitle.h"

#include <x68k/iocs.h>

#include "herodat.h"
#include "screen.h"
#include "vtext.h"

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
  screen_line(a.x, a.y, b.x, b.y, color);
}

}  // namespace

GameModeTitle::GameModeTitle()
    : confirm_down_(0),
      direction_down_(0),
      level_direction_down_(0),
      selected_players_(1),
      selected_cpu_level_(3),
      prompt_frame_(0),
      prompt_visible_(1),
      idle_frames_(0),
      frame_(0) {
  for (int page = 0; page < 2; ++page) {
    drawn_frame_[page] = -1;
    prompt_drawn_visible_[page] = -1;
    menu_drawn_players_[page] = 0;
    menu_drawn_cpu_level_[page] = 0;
    cut_markers_visible_[page] = 0;
  }
}

int GameModeTitle::initialize() {
  confirm_down_ = 0;
  direction_down_ = 0;
  level_direction_down_ = 0;
  prompt_frame_ = 0;
  prompt_visible_ = 1;
  idle_frames_ = 0;
  frame_ = 0;
  for (int page = 0; page < 2; ++page) {
    drawn_frame_[page] = -1;
    prompt_drawn_visible_[page] = -1;
    menu_drawn_players_[page] = 0;
    menu_drawn_cpu_level_[page] = 0;
    cut_markers_visible_[page] = 0;
  }
  input_.update();
  return 1;
}

GameModeId GameModeTitle::update() {
  input_.update();
  const int direction = input_.menu_up() || input_.menu_down();
  if (direction && !direction_down_) {
    if (input_.menu_up()) selected_players_ = 1;
    if (input_.menu_down()) selected_players_ = 2;
  }
  direction_down_ = direction;
  const int level_direction = input_.menu_left() || input_.menu_right();
  if (selected_players_ == 1 && level_direction &&
      !level_direction_down_) {
    if (input_.menu_left() && selected_cpu_level_ > 1) {
      --selected_cpu_level_;
    }
    if (input_.menu_right() && selected_cpu_level_ < 5) {
      ++selected_cpu_level_;
    }
  }
  level_direction_down_ = level_direction;
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
  }

  frame_ = (frame_ + 1) % HERO_FRAME_COUNT;
  return GAME_MODE_TITLE;
}

void GameModeTitle::render(int page) {
  if (drawn_frame_[page] < 0) {
    draw_scene();
    draw_garage_frame(page, kHeroFrames[frame_], kHeroFrames[frame_]);
    drawn_frame_[page] = frame_;
  } else if (drawn_frame_[page] != frame_) {
    draw_garage_frame(page, kHeroFrames[drawn_frame_[page]],
                      kHeroFrames[frame_]);
    drawn_frame_[page] = frame_;
  }
  if (prompt_drawn_visible_[page] != prompt_visible_) {
    draw_prompt(prompt_visible_ ? COLOR_WHITE : COLOR_BLACK);
    prompt_drawn_visible_[page] = prompt_visible_;
  }
  if (menu_drawn_players_[page] != selected_players_ ||
      menu_drawn_cpu_level_[page] != selected_cpu_level_) {
    draw_player_menu(selected_players_, selected_cpu_level_,
                     menu_drawn_players_[page],
                     menu_drawn_cpu_level_[page]);
    menu_drawn_players_[page] = selected_players_;
    menu_drawn_cpu_level_[page] = selected_cpu_level_;
  }
}

void GameModeTitle::finalize() {
}

int GameModeTitle::player_count() const { return selected_players_; }

int GameModeTitle::cpu_level() const { return selected_cpu_level_; }

void GameModeTitle::draw_scene() {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 8, "VECTOR RACING", 1, 1, COLOR_CYAN);
  screen_text(466, 8, "01", 1, COLOR_WHITE);
  screen_line(24, 22, 488, 22, COLOR_FLOOR);
  vector_centered("WIRE DRIFT", 30, 6, 2, 2, COLOR_WHITE);
  vector_centered("RACERS", 78, 7, 3, 2, COLOR_CYAN);
  screen_line(152, 122, 360, 122, COLOR_FLOOR);
  screen_centered_tracking("UP DOWN MODE  LEFT RIGHT LEVEL", 382, 1, 2,
                           COLOR_WHITE);
}

void GameModeTitle::draw_prompt(int color) {
  screen_centered_tracking("SPACE START", 452, 1, 3, color);
}

void GameModeTitle::draw_player_menu(int players, int cpu_level,
                                     int previous_players,
                                     int previous_cpu_level) {
  if (previous_players > 0) {
    char previous_one_player[] = "1 PLAYER CPU LEVEL 3";
    previous_one_player[19] = (char)('0' + previous_cpu_level);
    screen_centered_tracking(previous_one_player, 404, 1, 2, COLOR_BLACK);
    screen_centered_tracking("2 PLAYERS", 426, 1, 4, COLOR_BLACK);
  }
  char one_player[] = "1 PLAYER CPU LEVEL 3";
  one_player[19] = (char)('0' + cpu_level);
  screen_centered_tracking(one_player, 404, 1, 2,
                           players == 1 ? COLOR_CYAN : COLOR_FLOOR);
  screen_centered_tracking("2 PLAYERS", 426, 1, 4,
                           players == 2 ? COLOR_CYAN : COLOR_FLOOR);
}

void GameModeTitle::clear_garage() {
  screen_fill(16, 124, 480, 270, COLOR_BLACK);
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

void GameModeTitle::draw_garage_frame(int page, const HeroFrame &previous,
                                      const HeroFrame &next) {
  if (cut_markers_visible_[page]) {
    draw_cut_markers(COLOR_BLACK);
    cut_markers_visible_[page] = 0;
  }

  if ((next.flags & HERO_FLAG_CUT) || previous.shot != next.shot) {
    clear_garage();
    draw_static_shot(next);
    draw_cut_markers(COLOR_CYAN);
    cut_markers_visible_[page] = 1;
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

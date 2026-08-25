#include "gmrace.h"

#include "introdat.h"
#include "screen.h"
#include "vtext.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_TRACK = 0xffff;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;
const iocs_color_t COLOR_P1_BOOST = 0xdff7;
const iocs_color_t COLOR_P2_BOOST = 0xde7f;
const iocs_color_t COLOR_GATE = 0xf83f;
const iocs_color_t COLOR_HUD_DIM = 0x2109;
const int ANGLE_LIMIT = 65536;
const int GATE_COOLDOWN_FRAMES = 60;
const int GATE_BOOST_REWARD = 300;
const int BOOST_MAX = 1000;
const int BOOST_GAUGE_SEGMENTS = 8;
const int TACKLE_CONTACT_ANGLE = 1400;
const int TACKLE_CONTACT_OFFSET = 30;
const int TACKLE_PUSH = 18;
const int TACKLE_RECOIL = 4;
const int TACKLE_COOLDOWN_FRAMES = 6;
const int COUNTDOWN_STEP_FRAMES = 15;
const int COUNTDOWN_GO_FRAME = COUNTDOWN_STEP_FRAMES * 4;
const int COUNTDOWN_END_FRAME = COUNTDOWN_GO_FRAME + 10;
const int CATCHUP_GAP_SMALL = ANGLE_LIMIT / 16;
const int CATCHUP_GAP_MEDIUM = ANGLE_LIMIT / 8;
const int CATCHUP_GAP_LARGE = ANGLE_LIMIT / 4;

const char *countdown_label(int stage) {
  static const char *labels[] = {"READY", "3", "2", "1", "START"};
  return labels[stage];
}

int crossed_angle(int previous, int current, int target) {
  if (previous <= current) return target > previous && target <= current;
  return target > previous || target <= current;
}

int lane_for_offset(int offset) {
  if (offset < -21) return 0;
  if (offset > 21) return 2;
  return 1;
}

int forward_distance(int from, int to) {
  return (to - from + ANGLE_LIMIT) & (ANGLE_LIMIT - 1);
}

int angle_distance(int a, int b) {
  int distance = a > b ? a - b : b - a;
  if (distance > ANGLE_LIMIT / 2) distance = ANGLE_LIMIT - distance;
  return distance;
}

void draw_line(int x0, int y0, int x1, int y1, iocs_color_t color) {
  screen_line(x0, y0, x1, y1, color);
}

}  // namespace

GameModeRace::GameModeRace()
    : player_count_(1),
      winner_(RACE_WINNER_NONE),
      tackle_cooldown_(0),
      countdown_frame_(0) {
  for (int page = 0; page < 2; ++page) {
    for (int player = 0; player < PLAYER_COUNT; ++player) {
      lap_drawn_[page][player] = -1;
      boost_drawn_[page][player] = -1;
    }
    for (int gate = 0; gate < GATE_COUNT; ++gate) {
      gate_drawn_active_[page][gate] = 0;
      gate_drawn_lane_[page][gate] = 0;
    }
    countdown_drawn_stage_[page] = -1;
  }
  boost_ready_[0] = 0;
  boost_ready_[1] = 0;
}

void GameModeRace::set_player_count(int players) {
  player_count_ = players == 2 ? 2 : 1;
}

int GameModeRace::player_count() const { return player_count_; }

RaceWinner GameModeRace::winner() const { return winner_; }

int GameModeRace::lap(int player) const {
  return player >= 0 && player < PLAYER_COUNT ? cars_[player].lap() : 0;
}

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

CarInput GameModeRace::cpu_input() const {
  static const int lane_offsets[3] = {-42, 0, 42};
  CarInput result = {1, 0, 0, 0, 0, 0};
  int best_distance = ANGLE_LIMIT;
  int target = 0;
  for (int gate = 0; gate < GATE_COUNT; ++gate) {
    if (!gates_[gate].active) continue;
    const int distance = forward_distance(cars_[1].angle(),
                                          gates_[gate].angle);
    if (distance < best_distance) {
      best_distance = distance;
      target = lane_offsets[gates_[gate].lane];
    }
  }
  if (cars_[1].offset() > target + 4) result.left = 1;
  if (cars_[1].offset() < target - 4) result.right = 1;
  result.boost = cars_[1].boost() > 0 && cars_[1].speed() < 500;
  return result;
}

void GameModeRace::update_gates(const int *previous_angles) {
  for (int gate = 0; gate < GATE_COUNT; ++gate) {
    ActiveGate &state = gates_[gate];
    if (!state.active) {
      if (--state.cooldown <= 0) {
        state.cooldown = 0;
        state.lane = (state.lane + 1) % 3;
        state.active = 1;
      }
      continue;
    }

    int claim[PLAYER_COUNT] = {0, 0};
    for (int player = 0; player < PLAYER_COUNT; ++player) {
      claim[player] = crossed_angle(previous_angles[player],
                                    cars_[player].angle(), state.angle) &&
                      lane_for_offset(cars_[player].offset()) == state.lane;
    }
    if (!claim[0] && !claim[1]) continue;

    if (claim[0] && claim[1]) {
      const int p1_over = forward_distance(state.angle, cars_[0].angle());
      const int p2_over = forward_distance(state.angle, cars_[1].angle());
      if (p1_over == p2_over) {
        cars_[0].add_boost(GATE_BOOST_REWARD / 2);
        cars_[1].add_boost(GATE_BOOST_REWARD / 2);
      } else {
        cars_[p1_over > p2_over ? 0 : 1].add_boost(GATE_BOOST_REWARD);
      }
    } else {
      cars_[claim[0] ? 0 : 1].add_boost(GATE_BOOST_REWARD);
    }
    state.active = 0;
    state.cooldown = GATE_COOLDOWN_FRAMES;
  }
}

void GameModeRace::update_catchup_boost() {
  const int progress[PLAYER_COUNT] = {
    cars_[0].lap() * ANGLE_LIMIT + cars_[0].angle(),
    cars_[1].lap() * ANGLE_LIMIT + cars_[1].angle()
  };
  if (progress[0] == progress[1]) return;

  const int trailing = progress[0] < progress[1] ? 0 : 1;
  int gap = progress[0] - progress[1];
  if (gap < 0) gap = -gap;
  int recovery = 0;
  if (gap >= CATCHUP_GAP_LARGE) recovery = 12;
  else if (gap >= CATCHUP_GAP_MEDIUM) recovery = 8;
  else if (gap >= CATCHUP_GAP_SMALL) recovery = 4;
  if (recovery > 0) cars_[trailing].add_boost(recovery);
}

void GameModeRace::resolve_tackle() {
  if (tackle_cooldown_ > 0) {
    --tackle_cooldown_;
    return;
  }
  if (angle_distance(cars_[0].angle(), cars_[1].angle()) >
      TACKLE_CONTACT_ANGLE) {
    return;
  }
  int offset_distance = cars_[0].offset() - cars_[1].offset();
  if (offset_distance < 0) offset_distance = -offset_distance;
  if (offset_distance > TACKLE_CONTACT_OFFSET) return;

  const int direction_0_to_1 =
      cars_[1].offset() >= cars_[0].offset() ? 1 : -1;
  const int p1_attacks = cars_[0].drift() == direction_0_to_1;
  const int p2_attacks = cars_[1].drift() == -direction_0_to_1;

  if (p1_attacks && p2_attacks) {
    cars_[0].apply_impact(-direction_0_to_1 * TACKLE_PUSH, 75);
    cars_[1].apply_impact(direction_0_to_1 * TACKLE_PUSH, 75);
  } else if (p1_attacks) {
    cars_[0].apply_impact(-direction_0_to_1 * TACKLE_RECOIL, 90);
    cars_[1].apply_impact(direction_0_to_1 * TACKLE_PUSH, 75);
  } else if (p2_attacks) {
    cars_[1].apply_impact(direction_0_to_1 * TACKLE_RECOIL, 90);
    cars_[0].apply_impact(-direction_0_to_1 * TACKLE_PUSH, 75);
  } else {
    cars_[0].apply_impact(-direction_0_to_1 * TACKLE_RECOIL, 92);
    cars_[1].apply_impact(direction_0_to_1 * TACKLE_RECOIL, 92);
  }
  tackle_cooldown_ = TACKLE_COOLDOWN_FRAMES;
}

void GameModeRace::draw_gate(
    const Vec2s track[2][TRACK_SEGMENTS], int gate, int lane,
    iocs_color_t color) const {
  const int segment = gates_[gate].angle * TRACK_SEGMENTS / ANGLE_LIMIT;
  const Vec2s &inner = track[0][segment];
  const Vec2s &outer = track[1][segment];
  const int low = lane * 4 + 1;
  const int high = lane * 4 + 3;
  const int x0 = inner.x + (outer.x - inner.x) * low / 12;
  const int y0 = inner.y + (outer.y - inner.y) * low / 12;
  const int x1 = inner.x + (outer.x - inner.x) * high / 12;
  const int y1 = inner.y + (outer.y - inner.y) * high / 12;
  draw_line(x0, y0, x1, y1, color);
}

void GameModeRace::draw_gates(
    int page, const Vec2s track[2][TRACK_SEGMENTS]) {
  for (int gate = 0; gate < GATE_COUNT; ++gate) {
    if (gate_drawn_active_[page][gate] &&
        (!gates_[gate].active ||
         gate_drawn_lane_[page][gate] != gates_[gate].lane)) {
      draw_gate(track, gate, gate_drawn_lane_[page][gate], COLOR_BLACK);
    }
    if (gates_[gate].active) {
      draw_gate(track, gate, gates_[gate].lane, COLOR_GATE);
    }
    gate_drawn_active_[page][gate] = gates_[gate].active;
    gate_drawn_lane_[page][gate] = gates_[gate].lane;
  }
}

int GameModeRace::countdown_stage() const {
  if (intro_frame_ + 1 < INTRO_FRAME_COUNT) return -1;
  if (countdown_frame_ < COUNTDOWN_STEP_FRAMES) return 0;
  if (countdown_frame_ < COUNTDOWN_STEP_FRAMES * 2) return 1;
  if (countdown_frame_ < COUNTDOWN_STEP_FRAMES * 3) return 2;
  if (countdown_frame_ < COUNTDOWN_GO_FRAME) return 3;
  if (countdown_frame_ < COUNTDOWN_END_FRAME) return 4;
  return -1;
}

void GameModeRace::draw_countdown(int page) {
  const int stage = countdown_stage();
  const int previous = countdown_drawn_stage_[page];
  if (stage == previous) return;
  if (previous >= 0) {
    const int scale = previous >= 1 && previous <= 3 ? 8 : 5;
    vector_centered(countdown_label(previous), 44, scale, 2, 1,
                    COLOR_BLACK);
  }
  if (stage >= 0) {
    const int scale = stage >= 1 && stage <= 3 ? 8 : 5;
    vector_centered(countdown_label(stage), 44, scale, 2, 1,
                    stage == 4 ? COLOR_GATE : COLOR_TRACK);
  }
  countdown_drawn_stage_[page] = stage;
}

void GameModeRace::draw_hud(int page) {
  for (int player = 0; player < PLAYER_COUNT; ++player) {
    const int current_lap = cars_[player].lap();
    if (lap_drawn_[page][player] != current_lap) {
      const int x = player == 0 ? 20 : 320;
      char label[] = "P1 LAP 0 OF 3";
      label[1] = (char)('1' + player);
      label[7] = (char)('0' + (current_lap > 3 ? 3 : current_lap));
      screen_fill(x - 4, 6, 176, 18, COLOR_BLACK);
      if (player == 1 && player_count_ == 1) {
        char cpu_label[] = "CPU LAP 0 OF 3";
        cpu_label[8] = label[7];
        screen_text_tracking(x, 10, cpu_label, 1, 1, COLOR_P2);
      } else {
        screen_text_tracking(x, 10, label, 1, 1,
                             player == 0 ? COLOR_P1 : COLOR_P2);
      }
      lap_drawn_[page][player] = current_lap;
    }
    draw_boost_gauge(page, player);
  }
}

void GameModeRace::draw_boost_gauge(int page, int player) {
  const int x = player == 0 ? 20 : 320;
  const int bar_x = x + 42;
  const int y = 31;
  int level = (cars_[player].boost() * BOOST_GAUGE_SEGMENTS +
               BOOST_MAX - 1) / BOOST_MAX;
  if (level < 0) level = 0;
  if (level > BOOST_GAUGE_SEGMENTS) level = BOOST_GAUGE_SEGMENTS;
  const int previous = boost_drawn_[page][player];
  if (previous == level) return;

  const iocs_color_t active = player == 0 ? COLOR_P1 : COLOR_P2;
  if (previous < 0) {
    screen_text_tracking(x, 27, "BOOST", 1, 1, COLOR_HUD_DIM);
  }
  for (int segment = 0; segment < BOOST_GAUGE_SEGMENTS; ++segment) {
    const int was_active = previous >= 0 && segment < previous;
    const int is_active = segment < level;
    if (previous >= 0 && was_active == is_active) continue;
    const int start = bar_x + segment * 12;
    screen_line(start, y, start + 8, y,
                is_active ? active : COLOR_HUD_DIM);
  }
  boost_drawn_[page][player] = level;
}

int GameModeRace::initialize() {
  initialize_trig_table();
  intro_frame_ = 0;
  intro_drawn_frame_[0] = -1;
  intro_drawn_frame_[1] = -1;
  winner_ = RACE_WINNER_NONE;
  tackle_cooldown_ = 0;
  countdown_frame_ = 0;
  boost_ready_[0] = 0;
  boost_ready_[1] = player_count_ == 1;
  for (int gate = 0; gate < GATE_COUNT; ++gate) {
    gates_[gate].angle = (gate + 1) * ANGLE_LIMIT / 4;
    gates_[gate].lane = gate;
    gates_[gate].active = 1;
    gates_[gate].cooldown = 0;
  }
  for (int page = 0; page < 2; ++page) {
    for (int player = 0; player < PLAYER_COUNT; ++player) {
      lap_drawn_[page][player] = -1;
      boost_drawn_[page][player] = -1;
    }
    for (int gate = 0; gate < GATE_COUNT; ++gate) {
      gate_drawn_active_[page][gate] = 0;
      gate_drawn_lane_[page][gate] = 0;
    }
    countdown_drawn_stage_[page] = -1;
  }

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
  CarInput player_input[PLAYER_COUNT] = {
    input_.car_input(0), input_.car_input(1)
  };
  for (int player = 0; player < PLAYER_COUNT; ++player) {
    if (!player_input[player].boost) boost_ready_[player] = 1;
    if (!boost_ready_[player]) player_input[player].boost = 0;
  }
  if (countdown_frame_ < COUNTDOWN_GO_FRAME) {
    ++countdown_frame_;
    return GAME_MODE_RACE;
  }
  if (countdown_frame_ < COUNTDOWN_END_FRAME) ++countdown_frame_;
  const int previous_angles[PLAYER_COUNT] = {
    cars_[0].angle(), cars_[1].angle()
  };
  cars_[0].update(player_input[0]);
  cars_[1].update(player_count_ == 1 ? cpu_input() : player_input[1]);
  resolve_tackle();
  update_gates(previous_angles);
  update_catchup_boost();
  const int p1_finished = cars_[0].lap() >= 3;
  const int p2_finished = cars_[1].lap() >= 3;
  if (p1_finished || p2_finished) {
    if (p1_finished && p2_finished) winner_ = RACE_WINNER_DRAW;
    else if (p1_finished) winner_ = RACE_WINNER_PLAYER_1;
    else winner_ = RACE_WINNER_PLAYER_2;
    return GAME_MODE_RESULT;
  }
  return GAME_MODE_RACE;
}

void GameModeRace::render(int page) {
  if (intro_drawn_frame_[page] < 0) {
    clear_screen();
    draw_track(kIntroFrames[intro_frame_].track, COLOR_TRACK);
    prepare_intro_frame(intro_frame_);
    cars_[0].render(page, cars_[0].boosting() ? COLOR_P1_BOOST : COLOR_P1);
    cars_[1].render(page, cars_[1].boosting() ? COLOR_P2_BOOST : COLOR_P2);
    if (intro_frame_ + 1 >= INTRO_FRAME_COUNT) {
      draw_gates(page, kIntroFrames[intro_frame_].track);
    }
    draw_hud(page);
    draw_countdown(page);
    intro_drawn_frame_[page] = intro_frame_;
    return;
  }

  if (intro_drawn_frame_[page] != intro_frame_) {
    const IntroFrame &previous = kIntroFrames[intro_drawn_frame_[page]];
    const IntroFrame &next = kIntroFrames[intro_frame_];

    prepare_intro_frame(intro_frame_);
    cars_[0].clear_previous(page);
    cars_[1].clear_previous(page);
    draw_track(previous.track, COLOR_BLACK);
    draw_track(next.track, COLOR_TRACK);
    cars_[0].render(page, cars_[0].boosting() ? COLOR_P1_BOOST : COLOR_P1);
    cars_[1].render(page, cars_[1].boosting() ? COLOR_P2_BOOST : COLOR_P2);
    if (intro_frame_ + 1 >= INTRO_FRAME_COUNT) {
      draw_gates(page, next.track);
    }
    draw_hud(page);
    draw_countdown(page);
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
  draw_gates(page, kIntroFrames[INTRO_FRAME_COUNT - 1].track);
  cars_[0].render(page, cars_[0].boosting() ? COLOR_P1_BOOST : COLOR_P1);
  cars_[1].render(page, cars_[1].boosting() ? COLOR_P2_BOOST : COLOR_P2);
  draw_hud(page);
  draw_countdown(page);
}

void GameModeRace::finalize() {}

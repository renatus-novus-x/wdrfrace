#include "gmctrl.h"

#include "screen.h"
#include "vtext.h"

namespace {

const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_CYAN = 0xf83f;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;

}  // namespace

GameModeControls::GameModeControls() : player_count_(1), cpu_level_(3) {}

void GameModeControls::set_player_count(int players) {
  player_count_ = players == 2 ? 2 : 1;
}

void GameModeControls::set_cpu_level(int level) {
  if (level < 1) level = 1;
  if (level > 5) level = 5;
  cpu_level_ = level;
}

void GameModeControls::draw_screen() const {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 10, "RACE CONTROL", 1, 1, COLOR_CYAN);
  screen_text(466, 10, player_count_ == 1 ? "1P" : "2P", 1,
              COLOR_WHITE);
  screen_line(24, 26, 488, 26, COLOR_CYAN);
  vector_centered("HOW TO PLAY", 40, 6, 2, 2, COLOR_WHITE);
  screen_line(176, 88, 336, 88, COLOR_CYAN);

  vector_centered("PLAYER 1", 108, 4, 2, 1, COLOR_P1);
  screen_centered_tracking("W S  SPEED   A D  DRIFT", 150, 1, 3,
                           COLOR_WHITE);
  screen_centered_tracking("Q BOOST   E BRAKE", 176, 1, 3,
                           COLOR_WHITE);
  screen_centered_tracking("PAD 1 BUTTONS  BOOST BRAKE", 198, 1, 2,
                           COLOR_WHITE);
  screen_line(64, 210, 448, 210, 0x2109);
  vector_centered("PLAYER 2", 226, 4, 2, 1, COLOR_P2);
  if (player_count_ == 1) {
    screen_centered_tracking("CPU DRIVER", 272, 1, 3, COLOR_WHITE);
    char cpu_level[] = "CPU LEVEL 3";
    cpu_level[10] = (char)('0' + cpu_level_);
    screen_centered_tracking(cpu_level, 298, 1, 3, COLOR_WHITE);
  } else {
    screen_centered_tracking("CURSOR KEYS  SPEED DRIFT", 268, 1, 3,
                             COLOR_WHITE);
    screen_centered_tracking("N BOOST   M BRAKE", 294, 1, 3,
                             COLOR_WHITE);
    screen_centered_tracking("PAD 2 BUTTONS  BOOST BRAKE", 316, 1, 2,
                             COLOR_WHITE);
  }
  screen_line(64, 330, 448, 330, 0x2109);
  vector_centered("FIRST TO 3 LAPS", 348, 4, 1, 1, COLOR_CYAN);
  screen_centered_tracking("SPACE START", 404, 1, 4, COLOR_WHITE);
  screen_centered_tracking("ESC COURSE", 434, 1, 3, COLOR_WHITE);
  screen_line(24, 458, 488, 458, COLOR_CYAN);
}

int GameModeControls::initialize() {
  input_.update();
  input_released_ = !input_.confirm();
  drawn_[0] = 0;
  drawn_[1] = 0;
  return 1;
}

GameModeId GameModeControls::update() {
  input_.update();
  if (input_.cancel()) return GAME_MODE_COURSE_SELECT;
  if (!input_released_) {
    if (!input_.confirm()) input_released_ = 1;
    return GAME_MODE_HOW_TO_PLAY;
  }
  if (input_.confirm()) return GAME_MODE_RACE;
  return GAME_MODE_HOW_TO_PLAY;
}

void GameModeControls::render(int page) {
  if (!drawn_[page]) {
    draw_screen();
    drawn_[page] = 1;
  }
}

void GameModeControls::finalize() {}

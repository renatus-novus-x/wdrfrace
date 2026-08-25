#include "gmctrl.h"

#include "screen.h"

namespace {

const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_CYAN = 0xf83f;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;

}  // namespace

void GameModeControls::draw_screen() const {
  screen_clear(COLOR_BLACK);
  screen_line(16, 16, 495, 16, COLOR_CYAN);
  screen_line(495, 16, 495, 463, COLOR_CYAN);
  screen_line(495, 463, 16, 463, COLOR_CYAN);
  screen_line(16, 463, 16, 16, COLOR_CYAN);

  screen_centered("HOW TO PLAY", 40, 5, COLOR_WHITE);
  screen_centered("PLAYER 1", 108, 4, COLOR_P1);
  screen_centered("W S  SPEED   A D  DRIFT", 150, 2, COLOR_WHITE);
  screen_centered("Q  BOOST   E  BRAKE", 178, 2, COLOR_WHITE);
  screen_centered("PLAYER 2", 226, 4, COLOR_P2);
  screen_centered("CURSOR KEYS  SPEED DRIFT", 268, 2, COLOR_WHITE);
  screen_centered("N  BOOST   M  BRAKE", 296, 2, COLOR_WHITE);
  screen_centered("FIRST TO 3 LAPS", 350, 3, COLOR_CYAN);
  screen_centered("SPACE START", 398, 3, COLOR_WHITE);
  screen_centered("ESC TITLE", 430, 2, COLOR_WHITE);
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
  if (input_.quit()) return GAME_MODE_TITLE;
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

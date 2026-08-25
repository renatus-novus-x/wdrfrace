#include "gmresult.h"

#include "screen.h"
#include "vtext.h"

namespace {

const iocs_color_t COLOR_BLACK = 0x0000;
const iocs_color_t COLOR_WHITE = 0xffff;
const iocs_color_t COLOR_CYAN = 0xf83f;
const iocs_color_t COLOR_P1 = 0x67d9;
const iocs_color_t COLOR_P2 = 0x62bf;
const int RESULT_MIN_FRAMES = 20;
const int RESULT_FRAMES = 100;

}  // namespace

GameModeResult::GameModeResult()
    : player_count_(1),
      winner_(RACE_WINNER_NONE),
      elapsed_frames_(0),
      input_released_(0) {
  laps_[0] = 0;
  laps_[1] = 0;
  drawn_[0] = 0;
  drawn_[1] = 0;
}

void GameModeResult::set_result(int players, RaceWinner winner,
                                int p1_lap, int p2_lap) {
  player_count_ = players == 2 ? 2 : 1;
  winner_ = winner;
  laps_[0] = p1_lap;
  laps_[1] = p2_lap;
}

int GameModeResult::initialize() {
  elapsed_frames_ = 0;
  input_.update();
  input_released_ = !input_.any_key();
  drawn_[0] = 0;
  drawn_[1] = 0;
  return 1;
}

GameModeId GameModeResult::update() {
  input_.update();
  if (!input_released_) {
    if (!input_.any_key()) {
      input_released_ = 1;
      elapsed_frames_ = 0;
    }
    return GAME_MODE_RESULT;
  }

  ++elapsed_frames_;
  if (elapsed_frames_ >= RESULT_MIN_FRAMES && input_.any_key()) {
    return GAME_MODE_TITLE;
  }
  if (elapsed_frames_ >= RESULT_FRAMES) return GAME_MODE_TITLE;
  return GAME_MODE_RESULT;
}

void GameModeResult::render(int page) {
  if (!drawn_[page]) {
    draw_screen();
    drawn_[page] = 1;
  }
}

void GameModeResult::finalize() {}

void GameModeResult::draw_screen() const {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 10, "RACE RESULT", 1, 1, COLOR_CYAN);
  screen_text(466, 10, player_count_ == 1 ? "1P" : "2P", 1,
              COLOR_WHITE);
  screen_line(24, 26, 488, 26, COLOR_CYAN);
  vector_centered("RACE", 52, 7, 3, 2, COLOR_WHITE);
  screen_line(176, 108, 336, 108, COLOR_CYAN);

  const char *message = "DEAD HEAT";
  iocs_color_t message_color = COLOR_CYAN;
  if (winner_ == RACE_WINNER_PLAYER_1) {
    message = "PLAYER 1";
    message_color = COLOR_P1;
  } else if (winner_ == RACE_WINNER_PLAYER_2) {
    message = "PLAYER 2";
    message_color = COLOR_P2;
  }
  screen_centered_tracking(winner_ == RACE_WINNER_DRAW ?
                           "SAME FRAME FINISH" : "WINNER",
                           142, 1, 3, COLOR_CYAN);
  vector_centered(message, 168, 6, 2, 2, message_color);
  if (winner_ == RACE_WINNER_PLAYER_2 && player_count_ == 1) {
    screen_centered_tracking("CPU DRIVER", 228, 1, 3, COLOR_P2);
  }

  char p1[] = "P1 0 LAPS";
  char p2[] = "P2 0 LAPS";
  p1[3] = (char)('0' + (laps_[0] > 3 ? 3 : laps_[0]));
  p2[3] = (char)('0' + (laps_[1] > 3 ? 3 : laps_[1]));
  screen_centered_tracking(p1, 274, 1, 4, COLOR_P1);
  if (player_count_ == 1) {
    char cpu[] = "CPU 0 LAPS";
    cpu[4] = p2[3];
    screen_centered_tracking(cpu, 306, 1, 4, COLOR_P2);
  } else {
    screen_centered_tracking(p2, 306, 1, 4, COLOR_P2);
  }
  screen_line(96, 354, 416, 354, COLOR_CYAN);
  screen_centered_tracking("RETURNING TO TITLE", 420, 1, 3,
                           COLOR_WHITE);
}

#include "gmsetest.h"

#include "screen.h"
#include "sound.h"
#include "vtext.h"

namespace {

const int COLOR_BLACK = 0x0000;
const int COLOR_WHITE = 0xffff;
const int COLOR_CYAN = 0xf83f;
const int COLOR_DIM = 0x2109;

}  // namespace

GameModeSoundTest::GameModeSoundTest()
    : selected_(0),
      direction_down_(0),
      confirm_down_(0),
      pending_label_(0) {
  drawn_selected_[0] = -1;
  drawn_selected_[1] = -1;
}

int GameModeSoundTest::initialize() {
  input_.update();
  direction_down_ = input_.menu_left() || input_.menu_right();
  confirm_down_ = input_.confirm();
  pending_label_ = 0;
  drawn_selected_[0] = -1;
  drawn_selected_[1] = -1;
  return 1;
}

GameModeId GameModeSoundTest::update() {
  input_.update();
  if (input_.cancel()) return GAME_MODE_TITLE;

  const int direction = input_.menu_left() || input_.menu_right();
  if (direction && !direction_down_) {
    const int count = SoundEffect::label_count();
    if (input_.menu_left()) selected_ = (selected_ + count - 1) % count;
    else selected_ = (selected_ + 1) % count;
  }
  direction_down_ = direction;

  const int confirm = input_.confirm();
  if (confirm && !confirm_down_) {
    pending_label_ = SoundEffect::label_at(selected_);
  }
  confirm_down_ = confirm;
  return GAME_MODE_SE_TEST;
}

const char *GameModeSoundTest::consume_sound_label() {
  const char *label = pending_label_;
  pending_label_ = 0;
  return label;
}

void GameModeSoundTest::draw_scene() const {
  screen_clear(COLOR_BLACK);
  screen_text_tracking(24, 8, "DIAGNOSTIC", 1, 1, COLOR_CYAN);
  screen_text(466, 8, "SE", 1, COLOR_WHITE);
  screen_line(24, 22, 488, 22, COLOR_DIM);
  vector_centered("SOUND TEST", 50, 6, 2, 2, COLOR_WHITE);
  screen_line(156, 104, 356, 104, COLOR_CYAN);
  screen_centered_tracking("LEFT RIGHT SELECT", 382, 1, 3,
                           COLOR_WHITE);
  screen_centered_tracking("SPACE PLAY", 414, 1, 4, COLOR_CYAN);
  screen_centered_tracking("ESC TITLE", 448, 1, 3, COLOR_DIM);
}

void GameModeSoundTest::draw_selection(int selected, int color) const {
  const char *label = SoundEffect::label_at(selected);
  if (!label) return;
  vector_centered(label, 176, 5, 3, 1, color);
  char count[] = "01 OF 09";
  count[0] = (char)('0' + (selected + 1) / 10);
  count[1] = (char)('0' + (selected + 1) % 10);
  const int total = SoundEffect::label_count();
  count[6] = (char)('0' + total / 10);
  count[7] = (char)('0' + total % 10);
  screen_centered_tracking(count, 262, 1, 3, color);
}

void GameModeSoundTest::render(int page) {
  if (drawn_selected_[page] < 0) {
    draw_scene();
  } else if (drawn_selected_[page] != selected_) {
    draw_selection(drawn_selected_[page], COLOR_BLACK);
  }
  if (drawn_selected_[page] != selected_) {
    draw_selection(selected_, COLOR_CYAN);
    drawn_selected_[page] = selected_;
  }
}

void GameModeSoundTest::finalize() {}

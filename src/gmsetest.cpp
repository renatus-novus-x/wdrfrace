#include "gmsetest.h"

#include "bgm.h"
#include "screen.h"
#include "sound.h"
#include "vtext.h"

namespace {

const int COLOR_BLACK = 0x0000;
const int COLOR_WHITE = 0xffff;
const int COLOR_CYAN = 0xf83f;
const int COLOR_DIM = 0x2109;
const int BGM_LABEL_COUNT = 5;

const char *const kBgmLabels[BGM_LABEL_COUNT] = {
  "TITLE DEMO",
  "COURSE HOW TO",
  "GAME",
  "FINAL LAP",
  "RESULT",
};

BgmTrack bgm_track(int selected)
{
  return (BgmTrack)(BGM_TRACK_TITLE_DEMO + selected);
}

}  // namespace

GameModeSoundTest::GameModeSoundTest()
    : lane_(0),
      selected_bgm_(0),
      selected_se_(0),
      direction_down_(0),
      vertical_down_(0),
      confirm_down_(0),
      pending_label_(0) {
  for (int page = 0; page < 2; ++page) {
    drawn_lane_[page] = -1;
    drawn_bgm_[page] = -1;
    drawn_se_[page] = -1;
  }
}

int GameModeSoundTest::initialize() {
  input_.update();
  direction_down_ = input_.menu_left() || input_.menu_right();
  vertical_down_ = input_.menu_up() || input_.menu_down();
  confirm_down_ = input_.confirm();
  pending_label_ = 0;
  for (int page = 0; page < 2; ++page) {
    drawn_lane_[page] = -1;
    drawn_bgm_[page] = -1;
    drawn_se_[page] = -1;
  }
  return 1;
}

GameModeId GameModeSoundTest::update() {
  input_.update();
  if (input_.cancel()) return GAME_MODE_TITLE;

  const int vertical = input_.menu_up() || input_.menu_down();
  if (vertical && !vertical_down_) {
    if (input_.menu_up()) lane_ = 0;
    if (input_.menu_down()) lane_ = 1;
  }
  vertical_down_ = vertical;

  const int direction = input_.menu_left() || input_.menu_right();
  if (direction && !direction_down_) {
    const int count = lane_ == 0 ? bgm_track_count() :
                                   SoundEffect::label_count();
    int &selected = lane_ == 0 ? selected_bgm_ : selected_se_;
    if (count > 0) {
      if (input_.menu_left()) selected = (selected + count - 1) % count;
      else selected = (selected + 1) % count;
    }
  }
  direction_down_ = direction;

  const int confirm = input_.confirm();
  if (confirm && !confirm_down_) {
    if (lane_ == 0) {
      if (bgm_track_count() > 0)
        bgm_sound_test_play(bgm_track(selected_bgm_));
    } else {
      pending_label_ = SoundEffect::label_at(selected_se_);
    }
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
  screen_text(442, 8, "AUDIO", 1, COLOR_WHITE);
  screen_line(24, 22, 488, 22, COLOR_DIM);
  vector_centered("SOUND TEST", 50, 6, 2, 2, COLOR_WHITE);
  screen_line(156, 104, 356, 104, COLOR_CYAN);
  screen_centered_tracking("UP DOWN BGM SE", 366, 1, 3,
                           COLOR_WHITE);
  screen_centered_tracking("LEFT RIGHT SELECT", 396, 1, 3,
                           COLOR_WHITE);
  screen_centered_tracking("SPACE PLAY", 426, 1, 4, COLOR_CYAN);
  screen_centered_tracking("ESC TITLE", 456, 1, 3, COLOR_DIM);
}

void GameModeSoundTest::draw_selection(int lane, int selected, int color) const {
  const int bgm = lane == 0;
  const int available_bgm = bgm_track_count();
  const char *label = bgm ? (available_bgm > 0 ?
                             kBgmLabels[selected] : "UNAVAILABLE") :
                             SoundEffect::label_at(selected);
  if (!label) return;
  const int heading_y = bgm ? 128 : 240;
  const int label_y = bgm ? 158 : 270;
  const int count_y = bgm ? 204 : 316;
  screen_centered_tracking(bgm ? "BGM" : "SE", heading_y, 1, 4, color);
  vector_centered(label, label_y, 5, 2, 1, color);
  char count[] = "01 OF 09";
  count[0] = (char)('0' + (selected + 1) / 10);
  count[1] = (char)('0' + (selected + 1) % 10);
  const int total = bgm ? available_bgm : SoundEffect::label_count();
  count[6] = (char)('0' + total / 10);
  count[7] = (char)('0' + total % 10);
  screen_centered_tracking(count, count_y, 1, 3, color);
}

void GameModeSoundTest::render(int page) {
  const int changed = drawn_lane_[page] != lane_ ||
                      drawn_bgm_[page] != selected_bgm_ ||
                      drawn_se_[page] != selected_se_;
  if (drawn_lane_[page] < 0) draw_scene();
  if (changed) {
    if (drawn_bgm_[page] >= 0)
      draw_selection(0, drawn_bgm_[page], COLOR_BLACK);
    if (drawn_se_[page] >= 0)
      draw_selection(1, drawn_se_[page], COLOR_BLACK);
    draw_selection(0, selected_bgm_, lane_ == 0 ? COLOR_CYAN : COLOR_WHITE);
    draw_selection(1, selected_se_, lane_ == 1 ? COLOR_CYAN : COLOR_WHITE);
    drawn_lane_[page] = lane_;
    drawn_bgm_[page] = selected_bgm_;
    drawn_se_[page] = selected_se_;
  }
}

void GameModeSoundTest::finalize() {}

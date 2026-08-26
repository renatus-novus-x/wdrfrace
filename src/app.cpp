#include "app.h"

#include <stdint.h>
#include <x68k/iocs.h>

#include "screen.h"

namespace {

const int FIELD_W = 512;
const int FIELD_H = 480;
const int WAIT_VDISP_TIMEOUT_CS = 100;
const long CENTISEC_PER_DAY = 8640000L;
const uint8_t GPIP_VDISP = 0x10;
const uint16_t MAX_FRAME_CS = 10;
const uint16_t FIXED_FRAME_STEP_CS = 5;

#define CRTC_R04 ((void *)0x00E80008UL)
#define CRTC_R05 ((void *)0x00E8000AUL)
#define CRTC_R06 ((void *)0x00E8000CUL)
#define CRTC_R07 ((void *)0x00E8000EUL)
#define MFP_GPIP ((const void *)0x00E88001UL)

long ontime_diff_cs(struct iocs_time start, struct iocs_time end) {
  return ((long)end.day - (long)start.day) * CENTISEC_PER_DAY
       + (long)end.sec - (long)start.sec;
}

uint8_t read_gpip() { return (uint8_t)_iocs_b_bpeek(MFP_GPIP); }

void write_crtc(void *address, uint16_t value) {
  _iocs_b_wpoke(address, value);
}

int wait_vdisp() {
  struct iocs_time start = _iocs_ontime();
  int level = read_gpip() & GPIP_VDISP;
  for (;;) {
    int next = read_gpip() & GPIP_VDISP;
    if (ontime_diff_cs(start, _iocs_ontime()) > WAIT_VDISP_TIMEOUT_CS) return -1;
    if (next != level) {
      level = next;
      if (level != 0) return 0;
    }
  }
}

int set_60hz() {
  if (wait_vdisp() != 0) return -1;
  write_crtc(CRTC_R05, 0x0001);
  write_crtc(CRTC_R06, 0x0022);
  write_crtc(CRTC_R07, 0x0202);
  write_crtc(CRTC_R04, 0x020c);
  return 0;
}

int is_confirm_transition(GameModeId current, GameModeId next) {
  return (current == GAME_MODE_TITLE && next == GAME_MODE_COURSE_SELECT) ||
         (current == GAME_MODE_COURSE_SELECT &&
          next == GAME_MODE_HOW_TO_PLAY) ||
         (current == GAME_MODE_HOW_TO_PLAY && next == GAME_MODE_RACE);
}

int is_cancel_transition(GameModeId current, GameModeId next) {
  return (current == GAME_MODE_COURSE_SELECT && next == GAME_MODE_TITLE) ||
         (current == GAME_MODE_HOW_TO_PLAY &&
          next == GAME_MODE_COURSE_SELECT) ||
         (current == GAME_MODE_RACE && next == GAME_MODE_TITLE);
}

}  // namespace

GameMode *Application::mode_for(GameModeId id) {
  if (id == GAME_MODE_TITLE) return &title_mode_;
  if (id == GAME_MODE_DEMO) return &demo_mode_;
  if (id == GAME_MODE_COURSE_SELECT) return &course_mode_;
  if (id == GAME_MODE_HOW_TO_PLAY) return &controls_mode_;
  if (id == GAME_MODE_TEST) return &test_mode_;
  if (id == GAME_MODE_RACE) return &race_mode_;
  if (id == GAME_MODE_RESULT) return &result_mode_;
  return 0;
}

int Application::begin_current_mode() {
  if (!current_mode_ || !current_mode_->initialize()) return 0;
  mode_initializing_ = 1;
  render_due_ = 0;
  return 1;
}

int Application::initialize() {
  current_mode_ = 0;
  running_ = 0;
  paused_ = 0;
  frame_accumulator_cs_ = 0;
  render_due_ = 0;
  old_mode_ = _iocs_crtmod(-1);
  _iocs_crtmod(8);
  _iocs_g_clr_on();
  _iocs_window(0, 0, FIELD_W - 1, FIELD_H - 1);
  screen_palette_initialize();
  front_page_ = 0;
  back_page_ = 1;
  mode_initializing_ = 0;
  page_flip_pending_ = 0;
  _iocs_apage(front_page_);
  _iocs_vpage(1 << front_page_);
  _iocs_b_curoff();
  sound_.initialize();
  if (set_60hz() != 0) {
    finalize();
    return 0;
  }
  current_mode_id_ = GAME_MODE_TITLE;
  current_mode_ = mode_for(current_mode_id_);
  if (!begin_current_mode()) {
    finalize();
    return 0;
  }
  previous_time_ = _iocs_ontime();
  running_ = 1;
  return 1;
}

int Application::update() {
  if (!running_ || !current_mode_) return 0;
  if (wait_vdisp() != 0) {
    running_ = 0;
    return 0;
  }
  if (page_flip_pending_) {
    _iocs_vpage(1 << back_page_);
    const int old_front = front_page_;
    front_page_ = back_page_;
    back_page_ = old_front;
    page_flip_pending_ = 0;
  }

  struct iocs_time now = _iocs_ontime();
  long elapsed = ontime_diff_cs(previous_time_, now);
  previous_time_ = now;
  if (elapsed < 0) elapsed = 0;
  if (elapsed > MAX_FRAME_CS) elapsed = MAX_FRAME_CS;
  uint16_t frame_dt_cs = (uint16_t)elapsed;

  frame_accumulator_cs_ += frame_dt_cs;
  render_due_ = 0;
  const int preparing = mode_initializing_;
  while (frame_accumulator_cs_ >= FIXED_FRAME_STEP_CS) {
    frame_accumulator_cs_ -= FIXED_FRAME_STEP_CS;
    render_due_ = 1;
    sound_.update();
    if (preparing) continue;
    if (paused_) continue;

    GameModeId next = current_mode_->update();
    if (current_mode_->consume_select_sound()) sound_.play_select();
    if (next == current_mode_id_) continue;

    if (is_confirm_transition(current_mode_id_, next)) {
      sound_.play_confirm();
    } else if (is_cancel_transition(current_mode_id_, next)) {
      sound_.play_cancel();
    }

    if (current_mode_id_ == GAME_MODE_TITLE &&
        next == GAME_MODE_COURSE_SELECT) {
      const int players = title_mode_.player_count();
      controls_mode_.set_player_count(players);
      controls_mode_.set_cpu_level(title_mode_.cpu_level());
      race_mode_.set_player_count(players);
      race_mode_.set_cpu_level(title_mode_.cpu_level());
    }
    if (current_mode_id_ == GAME_MODE_COURSE_SELECT &&
        next == GAME_MODE_HOW_TO_PLAY) {
      race_mode_.set_course_id(course_mode_.course_id());
    }
    if (current_mode_id_ == GAME_MODE_RACE &&
        next == GAME_MODE_RESULT) {
      result_mode_.set_result(race_mode_.player_count(),
                              race_mode_.winner(),
                              race_mode_.lap(0), race_mode_.lap(1));
    }

    current_mode_->finalize();
    current_mode_ = 0;
    current_mode_id_ = next;
    frame_accumulator_cs_ = 0;
    if (next == GAME_MODE_EXIT) {
      running_ = 0;
      return 0;
    }
    current_mode_ = mode_for(next);
    running_ = current_mode_ && begin_current_mode();
    if (!running_) return 0;
  }

  if (preparing) {
    const int status = current_mode_->initialize_step();
    if (status < 0) {
      running_ = 0;
      return 0;
    }
    if (status > 0) mode_initializing_ = 0;
    render_due_ = 0;
  }

  return 1;
}

void Application::render() {
  if (current_mode_ && render_due_ && !page_flip_pending_ &&
      !mode_initializing_) {
    _iocs_apage(back_page_);
    current_mode_->render(back_page_);
    page_flip_pending_ = 1;
    render_due_ = 0;
  }
}

void Application::finalize() {
  sound_.finalize();
  if (current_mode_) {
    current_mode_->finalize();
    current_mode_ = 0;
  }
  _iocs_apage(0);
  screen_clear(0x0000);
  _iocs_apage(1);
  screen_clear(0x0000);
  _iocs_b_curon();
  int mode = old_mode_;
  if (mode < 0 || mode > 0x7f) mode = 12;
  _iocs_crtmod(mode);
  running_ = 0;
}

void Application::set_paused(int paused) {
  paused_ = paused != 0;
  frame_accumulator_cs_ = 0;
  previous_time_ = _iocs_ontime();
}

int Application::paused() const { return paused_; }

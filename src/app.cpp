#include "app.h"

#include <stdint.h>
#include <x68k/iocs.h>

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

void clear_screen() {
  struct iocs_fillptr rect;
  rect.x1 = 0;
  rect.y1 = 0;
  rect.x2 = FIELD_W - 1;
  rect.y2 = FIELD_H - 1;
  rect.color = 0x0000;
  _iocs_fill(&rect);
}

}  // namespace

GameMode *Application::mode_for(GameModeId id) {
  if (id == GAME_MODE_TITLE) return &title_mode_;
  if (id == GAME_MODE_DEMO) return &demo_mode_;
  if (id == GAME_MODE_HOW_TO_PLAY) return &controls_mode_;
  if (id == GAME_MODE_TEST) return &test_mode_;
  if (id == GAME_MODE_RACE) return &race_mode_;
  return 0;
}

int Application::initialize() {
  current_mode_ = 0;
  running_ = 0;
  paused_ = 0;
  frame_accumulator_cs_ = FIXED_FRAME_STEP_CS;
  render_due_ = 0;
  old_mode_ = _iocs_crtmod(-1);
  _iocs_crtmod(12);
  _iocs_g_clr_on();
  _iocs_b_curoff();
  if (set_60hz() != 0) {
    finalize();
    return 0;
  }
  current_mode_id_ = GAME_MODE_TITLE;
  current_mode_ = mode_for(current_mode_id_);
  if (!current_mode_ || !current_mode_->initialize()) {
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

  struct iocs_time now = _iocs_ontime();
  long elapsed = ontime_diff_cs(previous_time_, now);
  previous_time_ = now;
  if (elapsed < 0) elapsed = 0;
  if (elapsed > MAX_FRAME_CS) elapsed = MAX_FRAME_CS;
  uint16_t frame_dt_cs = (uint16_t)elapsed;

  frame_accumulator_cs_ += frame_dt_cs;
  render_due_ = 0;
  while (frame_accumulator_cs_ >= FIXED_FRAME_STEP_CS) {
    frame_accumulator_cs_ -= FIXED_FRAME_STEP_CS;
    render_due_ = 1;
    if (paused_) continue;

    GameModeId next = current_mode_->update();
    if (next == current_mode_id_) continue;

    current_mode_->finalize();
    current_mode_ = 0;
    current_mode_id_ = next;
    frame_accumulator_cs_ = 0;
    if (next == GAME_MODE_EXIT) {
      running_ = 0;
      return 0;
    }
    current_mode_ = mode_for(next);
    running_ = current_mode_ && current_mode_->initialize();
    if (!running_) return 0;
  }

  return 1;
}

void Application::render() {
  if (current_mode_ && render_due_) {
    current_mode_->render();
    render_due_ = 0;
  }
}

void Application::finalize() {
  if (current_mode_) {
    current_mode_->finalize();
    current_mode_ = 0;
  }
  clear_screen();
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

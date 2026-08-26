#include "sound.h"

#include <x68k/iocs.h>

namespace {

const int OPM_CHANNEL = 7;
const int CONFIRM_TICKS = 5;
const int CANCEL_TICKS = 3;
const int SELECT_TICKS = 2;
const int CONFIRM_FIRST = 0x5a;
const int CONFIRM_SECOND = 0x6e;
const int CANCEL_FIRST = 0x38;
const int CANCEL_SECOND = 0x2a;
const int SELECT_FIRST = 0x4e;
const int SELECT_SECOND = 0x58;

void write_operator(int slot, int multiple, int total_level, int decay) {
  const int offset = slot * 8 + OPM_CHANNEL;
  _iocs_opmset(0x40 + offset, multiple);
  _iocs_opmset(0x60 + offset, total_level);
  _iocs_opmset(0x80 + offset, 0x1f);
  _iocs_opmset(0xa0 + offset, decay);
  _iocs_opmset(0xc0 + offset, 0x00);
  _iocs_opmset(0xe0 + offset, 0xff);
}

}  // namespace

SoundEffect::SoundEffect()
    : initialized_(0), effect_(EFFECT_NONE), ticks_(0) {}

void SoundEffect::initialize() {
  key_off();
  _iocs_opmset(0x0f, 0x9f);
  _iocs_opmset(0x20 + OPM_CHANNEL, 0xc7);
  _iocs_opmset(0x38 + OPM_CHANNEL, 0x00);
  write_operator(0, 1, 0x14, 0x18);
  write_operator(1, 2, 0x7f, 0x00);
  write_operator(2, 1, 0x7f, 0x00);
  write_operator(3, 1, 0x28, 0x1f);
  initialized_ = 1;
  effect_ = EFFECT_NONE;
  ticks_ = 0;
}

void SoundEffect::key_off() {
  _iocs_opmset(0x08, OPM_CHANNEL);
}

void SoundEffect::play_note(int key_code) {
  key_off();
  _iocs_opmset(0x28 + OPM_CHANNEL, key_code);
  _iocs_opmset(0x30 + OPM_CHANNEL, 0x00);
  _iocs_opmset(0x08, 0x48 | OPM_CHANNEL);
}

void SoundEffect::start(Effect effect, int key_code) {
  if (!initialized_) return;
  effect_ = effect;
  if (effect == EFFECT_CONFIRM) {
    write_operator(0, 1, 0x0c, 0x10);
    write_operator(3, 1, 0x24, 0x18);
    ticks_ = CONFIRM_TICKS;
  } else if (effect == EFFECT_CANCEL) {
    write_operator(0, 1, 0x18, 0x18);
    write_operator(3, 1, 0x30, 0x1f);
    ticks_ = CANCEL_TICKS;
  } else {
    write_operator(0, 2, 0x12, 0x1a);
    write_operator(3, 1, 0x38, 0x1f);
    ticks_ = SELECT_TICKS;
  }
  play_note(key_code);
}

void SoundEffect::play_confirm() {
  start(EFFECT_CONFIRM, CONFIRM_FIRST);
}

void SoundEffect::play_cancel() {
  start(EFFECT_CANCEL, CANCEL_FIRST);
}

void SoundEffect::play_select() {
  start(EFFECT_SELECT, SELECT_FIRST);
}

void SoundEffect::update() {
  if (effect_ == EFFECT_NONE || ticks_ <= 0) return;
  --ticks_;
  const int effect_ticks = effect_ == EFFECT_CONFIRM ? CONFIRM_TICKS :
      (effect_ == EFFECT_CANCEL ? CANCEL_TICKS : SELECT_TICKS);
  const int second_note_tick = effect_ticks - 1;
  if (ticks_ == second_note_tick) {
    const int second_note = effect_ == EFFECT_CONFIRM ? CONFIRM_SECOND :
        (effect_ == EFFECT_CANCEL ? CANCEL_SECOND : SELECT_SECOND);
    play_note(second_note);
  }
  if (ticks_ == 0) {
    key_off();
    effect_ = EFFECT_NONE;
  }
}

void SoundEffect::finalize() {
  if (initialized_) {
    key_off();
    _iocs_opmset(0x0f, 0x00);
  }
  initialized_ = 0;
  effect_ = EFFECT_NONE;
  ticks_ = 0;
}

#include "sound.h"
#include "bgm.h"

#include <string.h>
#include <x68k/iocs.h>

namespace {

void sound_opm_write(int reg, int value)
{
  bgm_interrupt_lock();
  _iocs_opmset(reg, value);
  bgm_interrupt_unlock();
}

#define _iocs_opmset sound_opm_write

// NDP uses channels 0-2 and channel 7 for YM2151 noise emulation.
const int OPM_CHANNEL = 4;
const int kConfirmSequence[] = {0x5a, 1, 0x6e, 4};
const int kCancelSequence[] = {0x38, 1, 0x2a, 2};
const int kSelectSequence[] = {0x6e, 2};
const int kCountdownSequence[] = {0x6a, 2};
const int kStartSequence[] = {
  0x4e, 1, 0x5a, 1, 0x66, 2, 0x72, 4
};
const int kFinalLapSequence[] = {
  0x4a, 1, 0x52, 1, 0x5a, 2,
  0x4a, 1, 0x52, 1, 0x5a, 1, 0x6e, 3
};
const int kBoostSequence[] = {0x46, 1, 0x52, 1, 0x5e, 1, 0x6a, 1};
const int kDriftSequence[] = {
  0x42, 1, 0x4e, 1, 0x5a, 1, 0x66, 1, 0x72, 2
};
const int kTackleSequence[] = {0x36, 1, 0x2a, 3};
const int kWallSequence[] = {
  0x42, 1, 0x32, 1, 0x4a, 1, 0x36, 1, 0x52, 1
};
const int kGateSequence[] = {0x4a, 1, 0x5a, 1, 0x6e, 2};
const int kSlipstreamSequence[] = {0x3e, 1, 0x46, 1, 0x52, 2};

#define SEQUENCE_LENGTH(sequence) \
  ((int)(sizeof(sequence) / sizeof((sequence)[0]) / 2))

const char *kSoundLabels[] = {
  "CONFIRM", "CANCEL", "SELECT", "COUNTDOWN", "START",
  "FINAL LAP", "BOOST", "DRIFT", "TACKLE", "WALL", "GATE",
  "SLIPSTREAM"
};

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
    : initialized_(0),
      effect_(EFFECT_NONE),
      sequence_(0),
      sequence_length_(0),
      sequence_index_(0),
      note_ticks_(0) {}

void SoundEffect::initialize() {
  key_off();
  _iocs_opmset(0x20 + OPM_CHANNEL, 0xc7);
  _iocs_opmset(0x38 + OPM_CHANNEL, 0x00);
  write_operator(0, 1, 0x14, 0x18);
  write_operator(1, 2, 0x7f, 0x00);
  write_operator(2, 1, 0x7f, 0x00);
  write_operator(3, 1, 0x28, 0x1f);
  initialized_ = 1;
  effect_ = EFFECT_NONE;
  sequence_ = 0;
  sequence_length_ = 0;
  sequence_index_ = 0;
  note_ticks_ = 0;
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

void SoundEffect::start(Effect effect, const int *sequence, int length) {
  if (!initialized_ || !sequence || length <= 0) return;
  effect_ = effect;
  if (effect == EFFECT_CONFIRM) {
    write_operator(0, 1, 0x0c, 0x10);
    write_operator(3, 1, 0x24, 0x18);
  } else if (effect == EFFECT_CANCEL) {
    write_operator(0, 1, 0x18, 0x18);
    write_operator(3, 1, 0x30, 0x1f);
  } else if (effect == EFFECT_SELECT) {
    write_operator(0, 6, 0x18, 0x1f);
    write_operator(3, 1, 0x08, 0x1f);
  } else if (effect == EFFECT_COUNTDOWN) {
    write_operator(0, 1, 0x18, 0x1f);
    write_operator(3, 1, 0x02, 0x1f);
  } else if (effect == EFFECT_START) {
    write_operator(0, 3, 0x18, 0x10);
    write_operator(3, 1, 0x10, 0x10);
  } else if (effect == EFFECT_FINAL_LAP) {
    write_operator(0, 2, 0x18, 0x18);
    write_operator(3, 1, 0x14, 0x14);
  } else if (effect == EFFECT_BOOST) {
    write_operator(0, 3, 0x08, 0x10);
    write_operator(3, 1, 0x20, 0x18);
  } else if (effect == EFFECT_DRIFT) {
    write_operator(0, 5, 0x10, 0x1c);
    write_operator(3, 2, 0x18, 0x16);
  } else if (effect == EFFECT_TACKLE) {
    write_operator(0, 1, 0x04, 0x1f);
    write_operator(3, 1, 0x10, 0x1f);
  } else if (effect == EFFECT_WALL) {
    write_operator(0, 7, 0x04, 0x1f);
    write_operator(3, 6, 0x0c, 0x1f);
  } else if (effect == EFFECT_GATE) {
    write_operator(0, 2, 0x0c, 0x14);
    write_operator(3, 1, 0x26, 0x18);
  } else if (effect == EFFECT_SLIPSTREAM) {
    write_operator(0, 4, 0x18, 0x0c);
    write_operator(3, 1, 0x30, 0x14);
  } else {
    write_operator(0, 1, 0x08, 0x0c);
    write_operator(3, 1, 0x7f, 0x1f);
  }
  sequence_ = sequence;
  sequence_length_ = length;
  sequence_index_ = 0;
  note_ticks_ = sequence_[1];
  play_note(sequence_[0]);
}

void SoundEffect::stop() {
  if (initialized_) key_off();
  effect_ = EFFECT_NONE;
  sequence_ = 0;
  sequence_length_ = 0;
  sequence_index_ = 0;
  note_ticks_ = 0;
}

void SoundEffect::play_confirm() {
  start(EFFECT_CONFIRM, kConfirmSequence,
        SEQUENCE_LENGTH(kConfirmSequence));
}

void SoundEffect::play_cancel() {
  start(EFFECT_CANCEL, kCancelSequence,
        SEQUENCE_LENGTH(kCancelSequence));
}

void SoundEffect::play_select() {
  start(EFFECT_SELECT, kSelectSequence,
        SEQUENCE_LENGTH(kSelectSequence));
}

void SoundEffect::play_countdown() {
  start(EFFECT_COUNTDOWN, kCountdownSequence,
        SEQUENCE_LENGTH(kCountdownSequence));
}

void SoundEffect::play_start() {
  start(EFFECT_START, kStartSequence,
        SEQUENCE_LENGTH(kStartSequence));
}

void SoundEffect::play_final_lap() {
  start(EFFECT_FINAL_LAP, kFinalLapSequence,
        SEQUENCE_LENGTH(kFinalLapSequence));
}

void SoundEffect::play_boost() {
  start(EFFECT_BOOST, kBoostSequence, SEQUENCE_LENGTH(kBoostSequence));
}

void SoundEffect::play_drift() {
  start(EFFECT_DRIFT, kDriftSequence, SEQUENCE_LENGTH(kDriftSequence));
}

void SoundEffect::play_tackle() {
  start(EFFECT_TACKLE, kTackleSequence, SEQUENCE_LENGTH(kTackleSequence));
}

void SoundEffect::play_wall() {
  start(EFFECT_WALL, kWallSequence, SEQUENCE_LENGTH(kWallSequence));
}

void SoundEffect::play_gate() {
  start(EFFECT_GATE, kGateSequence, SEQUENCE_LENGTH(kGateSequence));
}

void SoundEffect::play_slipstream() {
  start(EFFECT_SLIPSTREAM, kSlipstreamSequence,
        SEQUENCE_LENGTH(kSlipstreamSequence));
}

int SoundEffect::play(const char *label) {
  if (!label) return 0;
  if (strcmp(label, "CONFIRM") == 0) play_confirm();
  else if (strcmp(label, "CANCEL") == 0) play_cancel();
  else if (strcmp(label, "SELECT") == 0) play_select();
  else if (strcmp(label, "COUNTDOWN") == 0) play_countdown();
  else if (strcmp(label, "START") == 0) play_start();
  else if (strcmp(label, "FINAL LAP") == 0) play_final_lap();
  else if (strcmp(label, "BOOST") == 0) play_boost();
  else if (strcmp(label, "DRIFT") == 0) play_drift();
  else if (strcmp(label, "TACKLE") == 0) play_tackle();
  else if (strcmp(label, "WALL") == 0) play_wall();
  else if (strcmp(label, "GATE") == 0) play_gate();
  else if (strcmp(label, "SLIPSTREAM") == 0) play_slipstream();
  else return 0;
  return 1;
}

int SoundEffect::label_count() {
  return sizeof(kSoundLabels) / sizeof(kSoundLabels[0]);
}

const char *SoundEffect::label_at(int index) {
  if (index < 0 || index >= label_count()) return 0;
  return kSoundLabels[index];
}

void SoundEffect::update() {
  if (effect_ == EFFECT_NONE || note_ticks_ <= 0) return;
  if (--note_ticks_ > 0) return;
  ++sequence_index_;
  if (sequence_index_ >= sequence_length_) {
    key_off();
    effect_ = EFFECT_NONE;
    sequence_ = 0;
    return;
  }
  const int offset = sequence_index_ * 2;
  note_ticks_ = sequence_[offset + 1];
  play_note(sequence_[offset]);
}

void SoundEffect::finalize() {
  if (initialized_) {
    key_off();
  }
  initialized_ = 0;
  effect_ = EFFECT_NONE;
  sequence_ = 0;
  sequence_length_ = 0;
  sequence_index_ = 0;
  note_ticks_ = 0;
}

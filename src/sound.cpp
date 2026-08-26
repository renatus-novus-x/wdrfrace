#include "sound.h"

#include <string.h>
#include <x68k/iocs.h>

namespace {

const int OPM_CHANNEL = 7;
const int kConfirmSequence[] = {0x5a, 1, 0x6e, 4};
const int kCancelSequence[] = {0x38, 1, 0x2a, 2};
const int kSelectSequence[] = {0x4e, 1, 0x58, 1};
const int kCountdownSequence[] = {0x4a, 2};
const int kStartSequence[] = {0x4a, 1, 0x6e, 3};
const int kFinalLapSequence[] = {0x5e, 1, 0x4a, 3};
const int kGoalP1Sequence[] = {
  0x4a, 2, 0x4e, 2, 0x5a, 2, 0x6e, 4
};
const int kGoalP2Sequence[] = {
  0x3a, 2, 0x3e, 2, 0x4a, 4
};
const int kGoalDrawSequence[] = {
  0x4a, 2, 0x4e, 2, 0x4a, 2, 0x4e, 4
};

#define SEQUENCE_LENGTH(sequence) \
  ((int)(sizeof(sequence) / sizeof((sequence)[0]) / 2))

const char *kSoundLabels[] = {
  "CONFIRM", "CANCEL", "SELECT", "COUNTDOWN", "START",
  "FINAL LAP", "GOAL P1", "GOAL P2", "GOAL DRAW"
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
  _iocs_opmset(0x0f, 0x9f);
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
    write_operator(0, 2, 0x12, 0x1a);
    write_operator(3, 1, 0x38, 0x1f);
  } else if (effect == EFFECT_COUNTDOWN) {
    write_operator(0, 2, 0x14, 0x1a);
    write_operator(3, 1, 0x3c, 0x1f);
  } else if (effect == EFFECT_START) {
    write_operator(0, 2, 0x08, 0x12);
    write_operator(3, 1, 0x24, 0x1a);
  } else if (effect == EFFECT_FINAL_LAP) {
    write_operator(0, 1, 0x10, 0x14);
    write_operator(3, 1, 0x28, 0x1c);
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

void SoundEffect::play_goal(int result) {
  if (result == 1) {
    start(EFFECT_GOAL_P1, kGoalP1Sequence,
          SEQUENCE_LENGTH(kGoalP1Sequence));
  } else if (result == 2) {
    start(EFFECT_GOAL_P2, kGoalP2Sequence,
          SEQUENCE_LENGTH(kGoalP2Sequence));
  } else {
    start(EFFECT_GOAL_DRAW, kGoalDrawSequence,
          SEQUENCE_LENGTH(kGoalDrawSequence));
  }
}

int SoundEffect::play(const char *label) {
  if (!label) return 0;
  if (strcmp(label, "CONFIRM") == 0) play_confirm();
  else if (strcmp(label, "CANCEL") == 0) play_cancel();
  else if (strcmp(label, "SELECT") == 0) play_select();
  else if (strcmp(label, "COUNTDOWN") == 0) play_countdown();
  else if (strcmp(label, "START") == 0) play_start();
  else if (strcmp(label, "FINAL LAP") == 0) play_final_lap();
  else if (strcmp(label, "GOAL P1") == 0) play_goal(1);
  else if (strcmp(label, "GOAL P2") == 0) play_goal(2);
  else if (strcmp(label, "GOAL DRAW") == 0) play_goal(0);
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
    _iocs_opmset(0x0f, 0x00);
  }
  initialized_ = 0;
  effect_ = EFFECT_NONE;
  sequence_ = 0;
  sequence_length_ = 0;
  sequence_index_ = 0;
  note_ticks_ = 0;
}

#include "sound.h"

#include <string.h>

#include "ndp.h"
#include "wdrse.h"

namespace {

NdpStreamSeBank stream_se_bank;

const char *kSoundLabels[] = {
  "CONFIRM", "CANCEL", "SELECT", "COUNTDOWN", "START",
  "BOOST", "DRIFT", "TACKLE", "WALL", "GATE",
  "SLIPSTREAM"
};

}  // namespace

SoundEffect::SoundEffect() : initialized_(0) {}

void SoundEffect::initialize() {
  initialized_ = ndp_stream_se_open(
      &stream_se_bank, wdr_stream_se_data, sizeof(wdr_stream_se_data));
  ndp_stream_se_set_master_volume(0);
}

void SoundEffect::start(Effect effect) {
  if (!initialized_ || effect < EFFECT_CONFIRM || effect >= EFFECT_COUNT) return;
  ndp_stream_se_play(&stream_se_bank, (unsigned int)effect);
}

void SoundEffect::stop() {
  if (initialized_) ndp_stream_se_stop();
}

void SoundEffect::play_confirm() { start(EFFECT_CONFIRM); }
void SoundEffect::play_cancel() { start(EFFECT_CANCEL); }
void SoundEffect::play_select() { start(EFFECT_SELECT); }
void SoundEffect::play_countdown() { start(EFFECT_COUNTDOWN); }
void SoundEffect::play_start() { start(EFFECT_START); }
void SoundEffect::play_boost() { start(EFFECT_BOOST); }
void SoundEffect::play_drift() { start(EFFECT_DRIFT); }
void SoundEffect::play_tackle() { start(EFFECT_TACKLE); }
void SoundEffect::play_wall() { start(EFFECT_WALL); }
void SoundEffect::play_gate() { start(EFFECT_GATE); }
void SoundEffect::play_slipstream() { start(EFFECT_SLIPSTREAM); }

int SoundEffect::play(const char *label) {
  if (!label) return 0;
  for (int index = 0; index < label_count(); ++index) {
    if (strcmp(label, kSoundLabels[index]) == 0) {
      start((Effect)index);
      return 1;
    }
  }
  return 0;
}

int SoundEffect::label_count() {
  return sizeof(kSoundLabels) / sizeof(kSoundLabels[0]);
}

const char *SoundEffect::label_at(int index) {
  if (index < 0 || index >= label_count()) return 0;
  return kSoundLabels[index];
}

void SoundEffect::update() {
  /* NDP advances the effect stream from the 60 Hz V-DISP interrupt. */
}

void SoundEffect::finalize() {
  stop();
  initialized_ = 0;
}

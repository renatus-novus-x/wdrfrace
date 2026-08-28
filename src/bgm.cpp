#include "bgm.h"

#include "bgmdata.h"

#include <x68k/iocs.h>

#define NDP_IMPLEMENTATION
#include "ndp.h"

namespace {

const unsigned int BGM_MASTER_ATTENUATION = 2;
volatile int bgm_interrupt_enabled = 0;
int bgm_interrupt_installed = 0;
BgmPlayer *active_bgm_player = 0;

void __attribute__((interrupt_handler)) bgm_vdisp_interrupt()
{
  if (bgm_interrupt_enabled) ndp_update();
}

int song_index(BgmTrack track) {
  if (track < BGM_TRACK_TITLE_DEMO || track > BGM_TRACK_RESULT) return -1;
  return (int)track - (int)BGM_TRACK_TITLE_DEMO;
}

}  // namespace

void bgm_interrupt_lock() { bgm_interrupt_enabled = 0; }

void bgm_interrupt_unlock()
{
  if (bgm_interrupt_installed) bgm_interrupt_enabled = 1;
}

void bgm_sound_test_play(BgmTrack track)
{
  if (!active_bgm_player) return;
  active_bgm_player->stop();
  active_bgm_player->play(track);
}

BgmPlayer::BgmPlayer()
    : initialized_(0), current_track_(BGM_TRACK_NONE) {}

int BgmPlayer::initialize() {
  if (!ndp_initialize()) return 0;
  ndp_set_master_volume(BGM_MASTER_ATTENUATION);
  current_track_ = BGM_TRACK_NONE;
  initialized_ = 1;
  bgm_interrupt_enabled = 1;
  if (_iocs_vdispst((const void *)bgm_vdisp_interrupt, 0, 1) != 0) {
    bgm_interrupt_enabled = 0;
    initialized_ = 0;
    ndp_finalize();
    return 0;
  }
  bgm_interrupt_installed = 1;
  active_bgm_player = this;
  return 1;
}

int BgmPlayer::start_current() {
  const int index = song_index(current_track_);
  if (!initialized_ || !WDR_BGM_AVAILABLE || index < 0 ||
      index >= WDR_BGM_SONG_COUNT) return 0;
  if (!ndp_start(wdr_bgm_songs[index].data, wdr_bgm_songs[index].size)) {
    current_track_ = BGM_TRACK_NONE;
    return 0;
  }
  ndp_set_master_volume(BGM_MASTER_ATTENUATION);
  return 1;
}

void BgmPlayer::update() {
  /* Playback is advanced by the V-DISP interrupt. */
}

void BgmPlayer::play(BgmTrack track) {
  if (!initialized_) return;
  bgm_interrupt_lock();
  if (track == BGM_TRACK_NONE) {
    ndp_stop();
    current_track_ = BGM_TRACK_NONE;
    bgm_interrupt_unlock();
    return;
  }
  if (track == current_track_ && ndp_is_playing()) {
    bgm_interrupt_unlock();
    return;
  }
  ndp_stop();
  current_track_ = track;
  start_current();
  bgm_interrupt_unlock();
}

void BgmPlayer::play_for_mode(GameModeId mode) {
  if (mode == GAME_MODE_TITLE || mode == GAME_MODE_DEMO) {
    play(BGM_TRACK_TITLE_DEMO);
  } else if (mode == GAME_MODE_COURSE_SELECT ||
             mode == GAME_MODE_HOW_TO_PLAY) {
    play(BGM_TRACK_COURSE_CONTROLS);
  } else if (mode == GAME_MODE_RACE) {
    play(BGM_TRACK_GAME);
  } else if (mode == GAME_MODE_RESULT) {
    play(BGM_TRACK_RESULT);
  } else {
    stop();
  }
}

void BgmPlayer::play_final_lap() { play(BGM_TRACK_FINAL_LAP); }

void BgmPlayer::stop() {
  bgm_interrupt_lock();
  if (initialized_) ndp_stop();
  current_track_ = BGM_TRACK_NONE;
  bgm_interrupt_unlock();
}

void BgmPlayer::finalize() {
  bgm_interrupt_lock();
  active_bgm_player = 0;
  if (bgm_interrupt_installed) {
    _iocs_vdispst((const void *)0, 0, 0);
    bgm_interrupt_installed = 0;
  }
  if (initialized_) ndp_finalize();
  initialized_ = 0;
  current_track_ = BGM_TRACK_NONE;
}

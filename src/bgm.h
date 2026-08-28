#ifndef WDR_BGM_H
#define WDR_BGM_H

#include "gmode.h"

enum BgmTrack {
  BGM_TRACK_NONE,
  BGM_TRACK_TITLE_DEMO,
  BGM_TRACK_COURSE_CONTROLS,
  BGM_TRACK_GAME,
  BGM_TRACK_FINAL_LAP,
  BGM_TRACK_RESULT,
};

void bgm_interrupt_lock();
void bgm_interrupt_unlock();
void bgm_sound_test_play(BgmTrack track);
int bgm_track_count();

class BgmPlayer {
 private:
  int initialized_;
  BgmTrack current_track_;

  int start_current();

 public:
  BgmPlayer();
  int initialize();
  void update();
  void play(BgmTrack track);
  void restart(BgmTrack track);
  void play_for_mode(GameModeId mode);
  void play_final_lap();
  void stop();
  void finalize();
};

#endif

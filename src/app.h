#ifndef WDR_APP_H
#define WDR_APP_H

#include <stdint.h>
#include <x68k/iocs.h>

#include "gmode.h"
#include "gmcourse.h"
#include "gmctrl.h"
#include "gmdemo.h"
#include "gmrace.h"
#include "gmresult.h"
#include "gmtest.h"
#include "gmtitle.h"
#include "sound.h"

class Application {
 private:
  int old_mode_;
  int running_;
  int paused_;
  struct iocs_time previous_time_;
  uint16_t frame_accumulator_cs_;
  int render_due_;
  int front_page_;
  int back_page_;
  int mode_initializing_;
  int page_flip_pending_;
  GameModeId current_mode_id_;
  GameMode *current_mode_;
  GameModeTitle title_mode_;
  GameModeDemo demo_mode_;
  GameModeCourse course_mode_;
  GameModeControls controls_mode_;
  GameModeTest test_mode_;
  GameModeRace race_mode_;
  GameModeResult result_mode_;
  SoundEffect sound_;

  GameMode *mode_for(GameModeId id);
  int begin_current_mode();

 public:
  int initialize();
  int update();
  void render();
  void finalize();
  void set_paused(int paused);
  int paused() const;
};

#endif

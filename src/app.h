#ifndef WDR_APP_H
#define WDR_APP_H

#include "gmode.h"
#include "gmtest.h"

class Application {
 private:
  int old_mode_;
  int running_;
  int paused_;
  GameModeId current_mode_id_;
  GameMode *current_mode_;
  GameModeTest test_mode_;

  GameMode *mode_for(GameModeId id);

 public:
  int initialize();
  int update();
  void render();
  void finalize();
  void set_paused(int paused);
  int paused() const;
};

#endif

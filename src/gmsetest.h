#ifndef WDR_GMSETEST_H
#define WDR_GMSETEST_H

#include "gmode.h"
#include "input.h"

class GameModeSoundTest : public GameMode {
 private:
  Input input_;
  int selected_;
  int direction_down_;
  int confirm_down_;
  int select_sound_pending_;
  const char *pending_label_;
  int drawn_selected_[2];

  void draw_scene() const;
  void draw_selection(int selected, int color) const;

 public:
  GameModeSoundTest();
  virtual int initialize();
  virtual GameModeId update();
  virtual int consume_select_sound();
  virtual const char *consume_sound_label();
  virtual void render(int page);
  virtual void finalize();
};

#endif

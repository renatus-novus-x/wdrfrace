#ifndef WDR_GMCTRL_H
#define WDR_GMCTRL_H

#include "gmode.h"
#include "input.h"

class GameModeControls : public GameMode {
 private:
  Input input_;
  int input_released_;
  int drawn_[2];

  void draw_screen() const;

 public:
  virtual int initialize();
  virtual GameModeId update();
  virtual void render(int page);
  virtual void finalize();
};

#endif

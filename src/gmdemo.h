#ifndef WDR_GMDEMO_H
#define WDR_GMDEMO_H

#include <x68k/iocs.h>

#include "gmode.h"
#include "input.h"
#include "math3d.h"

class GameModeDemo : public GameMode {
 private:
  Input input_;
  int input_released_;
  int frame_;
  int drawn_frame_;
  int frame_changed_;

  void draw_scene() const;
  void clear_replay() const;
  void draw_ring(const Vec2s *ring, iocs_color_t color) const;
  void draw_car(const Vec2s *car, iocs_color_t color) const;
  void draw_replay_frame(int previous_frame, int next_frame) const;

 public:
  virtual int initialize();
  virtual GameModeId update();
  virtual void render();
  virtual void finalize();
};

#endif

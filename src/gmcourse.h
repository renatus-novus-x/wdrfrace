#ifndef WDR_GMCOURSE_H
#define WDR_GMCOURSE_H

#include <x68k/iocs.h>

#include "gmode.h"
#include "input.h"
#include "math3d.h"

class GameModeCourse : public GameMode {
 private:
  Input input_;
  int selected_course_;
  int frame_;
  int input_released_;
  int direction_down_;
  int select_sound_pending_;
  int drawn_course_[2];
  int drawn_frame_[2];

  void draw_scene() const;
  void draw_ring(const Vec2s *ring, iocs_color_t color) const;
  void draw_marker(const Vec2s track[2][12], int segment,
                   int low, int high, iocs_color_t color) const;
  void draw_course(int course, int frame, iocs_color_t color) const;
  void draw_course_label(int course, iocs_color_t color) const;

 public:
  GameModeCourse();
  int course_id() const;
  virtual int initialize();
  virtual GameModeId update();
  virtual int consume_select_sound();
  virtual void render(int page);
  virtual void finalize();
};

#endif

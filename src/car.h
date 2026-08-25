#ifndef WDR_CAR_H
#define WDR_CAR_H

#include <stdint.h>
#include <x68k/iocs.h>

#include "camera.h"
#include "input.h"

struct ScreenRect {
  int min_x;
  int min_y;
  int max_x;
  int max_y;
  int valid;
};

class Car {
 private:
  enum { VERTEX_COUNT = 8, EDGE_COUNT = 12, TRIG_TABLE_SIZE = 256 };

  int angle_;
  int offset_;
  int speed_;
  int boost_;
  int lap_;
  Vec2s previous_[2][VERTEX_COUNT];
  Vec2s current_[VERTEX_COUNT];
  uint8_t previous_visible_[2][VERTEX_COUNT];
  uint8_t current_visible_[VERTEX_COUNT];
  int have_previous_[2];

  int project(const Camera &camera, const Vec3f &point, Vec2s &screen) const;
  void draw_wire(const Vec2s *points, const uint8_t *visible,
                 iocs_color_t color) const;

 public:
  void initialize(int angle, int offset);
  void update(const CarInput &input);
  void prepare_render(const Camera &camera, const float *sin_table);
  void prepare_screen(const Vec2s *points);
  ScreenRect previous_bounds(int page) const;
  void clear_previous(int page);
  void render(int page, iocs_color_t color);
  int speed() const;
  int angle() const;
  int offset() const;
  int boost() const;
  int lap() const;
};

#endif

#ifndef WDR_GMTEST_H
#define WDR_GMTEST_H

#include <stdint.h>
#include <x68k/iocs.h>

#include "camera.h"
#include "gmode.h"

class GameModeTest : public GameMode {
 private:
  enum {
    CAR_VERTEX_COUNT = 4,
    DEBUG_AXIS_POINT_COUNT = 4,
    TRIG_TABLE_SIZE = 256,
  };

  struct State {
    int frame;
    Vec3f base[CAR_VERTEX_COUNT];
    Vec2s prev[CAR_VERTEX_COUNT];
    Vec2s next[CAR_VERTEX_COUNT];
    uint8_t visible_prev[CAR_VERTEX_COUNT];
    uint8_t visible_next[CAR_VERTEX_COUNT];
    float camera_angle;
    int have_prev;
    struct iocs_time fps_start;
    int fps_count;
    int fps_x100;
    uint8_t fps_ready;
  };

  State state_;
  Camera camera_;
  float sin_table_[TRIG_TABLE_SIZE];
  Vec3f debug_axis_model_[DEBUG_AXIS_POINT_COUNT];
  Vec2s debug_axis_points_[DEBUG_AXIS_POINT_COUNT];
  Vec2s debug_axis_prev_[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible_[DEBUG_AXIS_POINT_COUNT];
  uint8_t debug_axis_visible_prev_[DEBUG_AXIS_POINT_COUNT];
  int debug_visible_;
  int debug_toggle_down_;
  int debug_visibility_changed_;
  int fps_updated_;

  void initialize_trig_table();
  int trig_index(float angle) const;
  long ontime_diff_cs(struct iocs_time start, struct iocs_time end) const;
  void initialize_car();
  void initialize_debug_axis();
  void update_camera();
  void project_debug_axis();
  int project_world(const Vec3f &in, Vec2s &out) const;
  int key_down(int scan) const;
  void draw_fill_block(int x1, int y1, int x2, int y2,
                       iocs_color_t color) const;
  void draw_pixel(int x, int y, iocs_color_t color) const;
  void draw_line(int x0, int y0, int x1, int y1,
                 iocs_color_t color) const;
  void draw_wire(const Vec2s *points, const uint8_t *visible,
                 iocs_color_t color) const;
  void draw_debug_axis() const;
  void erase_previous_frame();
  void save_previous_frame();
  const uint8_t *glyph_for_char(char c) const;
  void draw_glyph(int x, int y, char c, iocs_color_t color) const;
  void draw_text4x5(int x, int y, const char *text,
                    iocs_color_t color) const;
  void draw_fps_hud() const;
  int update_fps();

 public:
  virtual int initialize();
  virtual GameModeId update();
  virtual void render();
  virtual void finalize();
};

#endif

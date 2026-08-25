#ifndef WDR_GMRACE_H
#define WDR_GMRACE_H

#include <stdint.h>
#include <x68k/iocs.h>

#include "camera.h"
#include "car.h"
#include "gmode.h"
#include "gmresult.h"
#include "input.h"

class GameModeRace : public GameMode {
 private:
  enum {
    PLAYER_COUNT = 2,
    TRACK_SEGMENTS = 12,
    TRIG_TABLE_SIZE = 256,
  };

  Camera camera_;
  Car cars_[PLAYER_COUNT];
  Input input_;
  float sin_table_[TRIG_TABLE_SIZE];
  int intro_frame_;
  int intro_drawn_frame_[2];
  int player_count_;
  RaceWinner winner_;
  int lap_drawn_[2][PLAYER_COUNT];

  void initialize_trig_table();
  void prepare_intro_frame(int frame);
  void draw_ring(const Vec2s *ring, iocs_color_t color) const;
  void draw_track(const Vec2s track[2][TRACK_SEGMENTS],
                  iocs_color_t color) const;
  void draw_start_line(const Vec2s track[2][TRACK_SEGMENTS],
                       iocs_color_t color) const;
  int edge_intersects(const ScreenRect &bounds,
                      const Vec2s &a, const Vec2s &b) const;
  void repair_track(const Vec2s track[2][TRACK_SEGMENTS],
                    const ScreenRect *damage, int damage_count) const;
  void clear_screen() const;
  CarInput cpu_input() const;
  void draw_hud(int page);

 public:
  GameModeRace();
  void set_player_count(int players);
  int player_count() const;
  RaceWinner winner() const;
  int lap(int player) const;
  virtual int initialize();
  virtual GameModeId update();
  virtual void render(int page);
  virtual void finalize();
};

#endif

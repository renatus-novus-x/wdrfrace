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
    GATE_COUNT = 3,
  };

  struct ActiveGate {
    int angle;
    int lane;
    int active;
    int cooldown;
  };

  Camera camera_;
  Car cars_[PLAYER_COUNT];
  Input input_;
  float sin_table_[TRIG_TABLE_SIZE];
  int initialize_phase_;
  int trig_index_;
  float trig_s_;
  float trig_c_;
  int intro_frame_;
  int intro_drawn_frame_[2];
  int player_count_;
  int cpu_level_;
  int cpu_decision_timer_;
  int cpu_target_offset_;
  int cpu_boost_frames_;
  int cpu_boost_cooldown_;
  int course_id_;
  RaceWinner winner_;
  int lap_drawn_[2][PLAYER_COUNT];
  int boost_drawn_[2][PLAYER_COUNT];
  int slip_drawn_visible_[2][PLAYER_COUNT];
  ActiveGate gates_[GATE_COUNT];
  int gate_drawn_active_[2][GATE_COUNT];
  int gate_drawn_lane_[2][GATE_COUNT];
  int tackle_cooldown_;
  int countdown_frame_;
  int effect_frame_;
  int countdown_drawn_stage_[2];
  int boost_ready_[PLAYER_COUNT];
  int course_drawn_[2];
  int slipstream_frames_[PLAYER_COUNT];
  int slipstream_active_[PLAYER_COUNT];

  int initialize_trig_table_step();
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
  CarInput cpu_input();
  void draw_hud(int page);
  void draw_boost_gauge(int page, int player);
  void draw_slip_indicator(int page, int player);
  int slipstream_blink_on(int player) const;
  void update_gates(const int *previous_angles);
  void update_slipstream();
  void update_catchup_boost();
  void resolve_tackle();
  void draw_gate(const Vec2s track[2][TRACK_SEGMENTS],
                 int gate, int lane, iocs_color_t color) const;
  void draw_gates(int page, const Vec2s track[2][TRACK_SEGMENTS]);
  int countdown_stage() const;
  void draw_countdown(int page);

 public:
  GameModeRace();
  void set_player_count(int players);
  void set_cpu_level(int level);
  void set_course_id(int course_id);
  int player_count() const;
  RaceWinner winner() const;
  int lap(int player) const;
  virtual int initialize();
  virtual int initialize_step();
  virtual GameModeId update();
  virtual void render(int page);
  virtual void finalize();
};

#endif

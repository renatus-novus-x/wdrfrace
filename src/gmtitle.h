#ifndef WDRFRACE_GMTITLE_H
#define WDRFRACE_GMTITLE_H

#include "gmode.h"
#include "input.h"
#include "math3d.h"

struct HeroFrame;

class GameModeTitle : public GameMode {
 public:
  GameModeTitle();
  virtual int initialize();
  virtual GameModeId update();
  virtual int consume_select_sound();
  virtual void render(int page);
  virtual void finalize();
  int player_count() const;
  int cpu_level() const;

 private:
  void draw_scene();
  void draw_prompt(int color);
  void draw_player_menu(int players, int cpu_level,
                        int previous_players, int previous_cpu_level);
  void clear_garage();
  void draw_edges(const Vec2s (*edges)[2], int count, int color);
  void draw_one_edge(const Vec2s (*edges)[2], int index, int color);
  int car_color(int car, int pulse) const;
  int car_edge_index(int car, int phase) const;
  void draw_static_shot(const HeroFrame &frame);
  void draw_cut_markers(int color);
  void draw_garage_frame(int page, const HeroFrame &previous,
                         const HeroFrame &next);

  Input input_;
  int confirm_down_;
  int direction_down_;
  int level_direction_down_;
  int selected_players_;
  int selected_cpu_level_;
  int prompt_frame_;
  int prompt_visible_;
  int idle_frames_;
  int frame_;
  int drawn_frame_[2];
  int prompt_drawn_visible_[2];
  int menu_drawn_players_[2];
  int menu_drawn_cpu_level_[2];
  int cut_markers_visible_[2];
  int select_sound_pending_;
};

#endif

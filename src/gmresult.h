#ifndef WDR_GMRESULT_H
#define WDR_GMRESULT_H

#include "gmode.h"
#include "input.h"

enum RaceWinner {
  RACE_WINNER_NONE,
  RACE_WINNER_PLAYER_1,
  RACE_WINNER_PLAYER_2,
  RACE_WINNER_DRAW,
};

class GameModeResult : public GameMode {
 private:
  Input input_;
  int player_count_;
  RaceWinner winner_;
  int laps_[2];
  int elapsed_frames_;
  int input_released_;
  int drawn_[2];

  void draw_screen() const;

 public:
  GameModeResult();
  void set_result(int players, RaceWinner winner, int p1_lap, int p2_lap);
  virtual int initialize();
  virtual GameModeId update();
  virtual void render(int page);
  virtual void finalize();
};

#endif

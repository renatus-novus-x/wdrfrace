#ifndef WDR_GMODE_H
#define WDR_GMODE_H

#include <stdint.h>

enum GameModeId {
  GAME_MODE_TEST,
  GAME_MODE_SE_TEST,
  GAME_MODE_TITLE,
  GAME_MODE_DEMO,
  GAME_MODE_COURSE_SELECT,
  GAME_MODE_HOW_TO_PLAY,
  GAME_MODE_RACE,
  GAME_MODE_RESULT,
  GAME_MODE_EXIT,
};

enum GameSoundId {
  GAME_SOUND_NONE,
  GAME_SOUND_COUNTDOWN,
  GAME_SOUND_START,
  GAME_SOUND_FINAL_LAP,
  GAME_SOUND_GOAL_P1,
  GAME_SOUND_GOAL_P2,
  GAME_SOUND_GOAL_DRAW,
};

class GameMode {
 public:
  virtual ~GameMode();
  virtual int initialize();
  virtual int initialize_step();
  virtual void advance_time(int elapsed_cs);
  virtual GameModeId update();
  virtual int consume_select_sound();
  virtual GameSoundId consume_game_sound();
  virtual const char *consume_sound_label();
  virtual void render(int page);
  virtual void finalize();
};

#endif

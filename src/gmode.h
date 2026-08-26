#ifndef WDR_GMODE_H
#define WDR_GMODE_H

#include <stdint.h>

enum GameModeId {
  GAME_MODE_TEST,
  GAME_MODE_TITLE,
  GAME_MODE_DEMO,
  GAME_MODE_COURSE_SELECT,
  GAME_MODE_HOW_TO_PLAY,
  GAME_MODE_RACE,
  GAME_MODE_RESULT,
  GAME_MODE_EXIT,
};

class GameMode {
 public:
  virtual ~GameMode();
  virtual int initialize();
  virtual GameModeId update();
  virtual void render(int page);
  virtual void finalize();
};

#endif

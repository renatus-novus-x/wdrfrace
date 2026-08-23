#ifndef WDR_GMODE_H
#define WDR_GMODE_H

#include <stdint.h>

enum GameModeId {
  GAME_MODE_TEST,
  GAME_MODE_EXIT,
};

class GameMode {
 public:
  virtual ~GameMode();
  virtual int initialize();
  virtual GameModeId update();
  virtual void render();
  virtual void finalize();
};

#endif

#ifndef WDR_INPUT_H
#define WDR_INPUT_H

#include <stdint.h>

struct CarInput {
  int accelerate;
  int decelerate;
  int left;
  int right;
  int boost;
  int brake;
};

class Input {
 private:
  enum { KEY_GROUP_COUNT = 16 };
  uint8_t groups_[KEY_GROUP_COUNT];

  int key_down(int scan) const;

 public:
  void update();
  int quit() const;
  int confirm() const;
  int any_key() const;
  CarInput car_input(int player) const;
};

#endif

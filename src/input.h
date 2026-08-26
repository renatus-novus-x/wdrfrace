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
  enum { KEY_GROUP_COUNT = 16, PAD_COUNT = 2 };
  uint8_t groups_[KEY_GROUP_COUNT];
  uint8_t pads_[PAD_COUNT];

  int key_down(int scan) const;
  int pad_down(int port, int mask) const;
  int either_pad_down(int mask) const;

 public:
  void update();
  int quit() const;
  int cancel() const;
  int confirm() const;
  int any_key() const;
  int menu_up() const;
  int menu_down() const;
  int menu_left() const;
  int menu_right() const;
  CarInput car_input(int player) const;
};

#endif

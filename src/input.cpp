#include "input.h"

#include <x68k/iocs.h>

namespace {

const int KEY_ESC = 0x01;
const int KEY_Q = 0x11;
const int KEY_W = 0x12;
const int KEY_E = 0x13;
const int KEY_A = 0x1e;
const int KEY_S = 0x1f;
const int KEY_D = 0x20;
const int KEY_N = 0x31;
const int KEY_M = 0x32;
const int KEY_SPACE = 0x35;
const int KEY_LEFT = 0x3b;
const int KEY_UP = 0x3c;
const int KEY_RIGHT = 0x3d;
const int KEY_DOWN = 0x3e;

}  // namespace

void Input::update() {
  for (int i = 0; i < KEY_GROUP_COUNT; ++i) {
    groups_[i] = (uint8_t)_iocs_bitsns(i);
  }
}

int Input::key_down(int scan) const {
  return (groups_[scan >> 3] & (1 << (scan & 7))) != 0;
}

int Input::quit() const { return key_down(KEY_ESC); }

int Input::confirm() const { return key_down(KEY_SPACE); }

int Input::any_key() const {
  for (int i = 0; i < KEY_GROUP_COUNT; ++i) {
    if (groups_[i] != 0) return 1;
  }
  return 0;
}

CarInput Input::car_input(int player) const {
  CarInput result;
  if (player == 0) {
    result.accelerate = key_down(KEY_W);
    result.decelerate = key_down(KEY_S);
    result.left = key_down(KEY_A);
    result.right = key_down(KEY_D);
    result.boost = key_down(KEY_Q);
    result.brake = key_down(KEY_E);
  } else {
    result.accelerate = key_down(KEY_UP);
    result.decelerate = key_down(KEY_DOWN);
    result.left = key_down(KEY_LEFT);
    result.right = key_down(KEY_RIGHT);
    result.boost = key_down(KEY_N);
    result.brake = key_down(KEY_M);
  }
  return result;
}

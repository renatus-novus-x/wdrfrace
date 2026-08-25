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
const int JOY_UP = 0x01;
const int JOY_DOWN = 0x02;
const int JOY_LEFT = 0x04;
const int JOY_RIGHT = 0x08;
const int JOY_BUTTON_1 = 0x20;
const int JOY_BUTTON_2 = 0x40;
const int JOY_ACTIVITY_MASK = 0x6f;

}  // namespace

void Input::update() {
  for (int i = 0; i < KEY_GROUP_COUNT; ++i) {
    groups_[i] = (uint8_t)_iocs_bitsns(i);
  }
  for (int i = 0; i < PAD_COUNT; ++i) {
    pads_[i] = (uint8_t)_iocs_joyget(i);
  }
}

int Input::key_down(int scan) const {
  return (groups_[scan >> 3] & (1 << (scan & 7))) != 0;
}

int Input::pad_down(int port, int mask) const {
  return (pads_[port] & mask) == 0;
}

int Input::either_pad_down(int mask) const {
  return pad_down(0, mask) || pad_down(1, mask);
}

int Input::quit() const { return key_down(KEY_ESC); }

int Input::confirm() const {
  return key_down(KEY_SPACE) || either_pad_down(JOY_BUTTON_1);
}

int Input::menu_up() const {
  return key_down(KEY_UP) || key_down(KEY_W) ||
         either_pad_down(JOY_UP);
}

int Input::menu_down() const {
  return key_down(KEY_DOWN) || key_down(KEY_S) ||
         either_pad_down(JOY_DOWN);
}

int Input::any_key() const {
  for (int i = 0; i < KEY_GROUP_COUNT; ++i) {
    if (groups_[i] != 0) return 1;
  }
  for (int i = 0; i < PAD_COUNT; ++i) {
    if ((pads_[i] & JOY_ACTIVITY_MASK) != JOY_ACTIVITY_MASK) return 1;
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
  result.accelerate |= pad_down(player, JOY_UP);
  result.decelerate |= pad_down(player, JOY_DOWN);
  result.left |= pad_down(player, JOY_LEFT);
  result.right |= pad_down(player, JOY_RIGHT);
  result.boost |= pad_down(player, JOY_BUTTON_1);
  result.brake |= pad_down(player, JOY_BUTTON_2);
  return result;
}

#pragma once

#include <glm/ext/vector_float2.hpp>
struct InputData {
  // Instead of saving an enum of keys that GLFW already has, a
  // LUT to convert GLFW keycodes to specific bits: Potential downside is
  // requiring including GLFW in an engine class where as Input could be moved
  // to its own unique class, this could also handle input bindings, instead
  // of requiring if a key is pressed, there could be a "Move Forward" binding
  // that could be mapped to different keys / controller buttons
  enum KeyBinds : unsigned long {
    KEYBOARD_Q = 1 << 0,
    KEYBOARD_W = 1 << 1,
    KEYBOARD_E = 1 << 2,
    KEYBOARD_R = 1 << 3,
    KEYBOARD_T = 1 << 4,
    KEYBOARD_Y = 1 << 5,
    KEYBOARD_U = 1 << 6,
    KEYBOARD_I = 1 << 7,
    KEYBOARD_O = 1 << 8,
    KEYBOARD_P = 1 << 9,
    KEYBOARD_A = 1 << 10,
    KEYBOARD_S = 1 << 11,
    KEYBOARD_D = 1 << 12,
    KEYBOARD_F = 1 << 13,
    KEYBOARD_G = 1 << 14,
    KEYBOARD_H = 1 << 15,
    KEYBOARD_J = 1 << 16,
    KEYBOARD_K = 1 << 17,
    KEYBOARD_L = 1 << 18,
    KEYBOARD_Z = 1 << 19,
    KEYBOARD_X = 1 << 20,
    KEYBOARD_C = 1 << 21,
    KEYBOARD_V = 1 << 22,
    KEYBOARD_B = 1 << 23,
    KEYBOARD_N = 1 << 24,
    KEYBOARD_M = 1 << 25,
    KEYBOARD_LSHIFT = 1 << 26,
    KEYBOARD_LCTRL = 1 << 27,
    KEYBOARD_LALT = 1 << 28,
    KEYBOARD_TAB = 1 << 29,
    MOUSE_LEFT = 1 << 30,
    MOUSE_RIGHT = 1UL << 31,
  };

  glm::vec2 position;
  glm::vec2 deltaPos;

  unsigned long isPressed;
  unsigned long isFirstFrame;
  unsigned long isReleased;

  bool isKeyPressed(KeyBinds key, bool eatInput = false) {
    bool result = (isPressed & key) == 1;
    if (eatInput) {
      isPressed &= ~(1 << key);
    }
    return result;
  }
  bool isFirstFrameKeyPressed(KeyBinds key, bool eatInput = false) {
    bool result = (isFirstFrame & key) == 1;
    if (eatInput) {
      isFirstFrame &= ~(1 << key);
    }
    return result;
  }
  bool isKeyReleased(KeyBinds key, bool eatInput = false) {
    bool result = (isReleased & key) == 1;
    if (eatInput) {
      isReleased &= ~(1 << key);
    }
    return result;
  }
};
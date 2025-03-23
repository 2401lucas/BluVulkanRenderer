#ifndef INPUT_H
#define INPUT_H
#include <EASTL/deque.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>
#include <EASTL/vector_map.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>
#include "Window.h"

namespace blu::core {
struct InputEvent {
  int key;
  int action;
  double timestamp;
  int mods;
};

// TODO:
//  - Axis Input -1<->1
//  - USE CALLBACKS!
// Maybe base kay remaps OnApplySettings() so that the KeyMap isn't updated as
// often Aso fix keymap impl
// Was Pressed
// Is Pressed
// Was && Is == Held
// Was && !Is == Released
// !Was && Is == First Press
class KeybindManager {
 public:
  KeybindManager(blu::core::Window*);

  void RegisterKeyBind(const eastl::string& action, int default_key,
                       int alt_key = GLFW_KEY_UNKNOWN, bool allow_mods = false,
                       int req_mods = 0);
  glm::vec2 GetMousePos();
  bool IsActionPressed(const eastl::string& action, int mods = 0);
  void RemapAction(const eastl::string& action, int key, bool is_primary_key);
  void SaveKeybinds();
  bool LoadKeybinds();

 private:
  void UpdateKeyMap();

  struct Keybind_ {
    eastl::string action;
    int primary_key;
    int alternate_key;
    bool allows_modifiers;
    int required_mods;
  };

  blu::core::Window* window_;

  glm::vec2 prev_mouse_pos_;
  eastl::vector_map<eastl::string, Keybind_> keybinds_;
  eastl::vector_map<int, eastl::string> key_to_action_;
};
}  // namespace blu::core
#endif
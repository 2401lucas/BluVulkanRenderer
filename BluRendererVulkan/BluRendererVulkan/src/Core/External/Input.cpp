#include "Input.h"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

namespace blu::core {
KeybindManager::KeybindManager(blu::core::Window* window) { window_ = window; }

void KeybindManager::RegisterKeyBind(const eastl::string& action,
                                     int default_key, int alt_key,
                                     bool allow_mods, int req_mods) {
  keybinds_[action] = {action, default_key, alt_key, allow_mods, req_mods};
  UpdateKeyMap();
}

glm::vec2 KeybindManager::GetMousePos() {
  double xpos, ypos;
  glfwGetCursorPos(window_->Get(), &xpos, &ypos);

  return glm::vec2(xpos, ypos);
}

glm::vec2 KeybindManager::GetMousePosDiff() {
  auto mouse_pos = GetMousePos();
  auto mouse_pos_diff = prev_mouse_pos_ - mouse_pos;
  prev_mouse_pos_ = mouse_pos;
  return mouse_pos_diff * mouse_sense_;
}

bool KeybindManager::IsActionPressed(const eastl::string& action, int mods) {
  auto it = keybinds_.find(action);

  if (it == keybinds_.end()) {
    std::cout << "Action not bound: " << action.c_str() << std::endl;
    return false;
  }
  const auto& bind = it->second;

  if (bind.allows_modifiers &&
      (mods & bind.required_mods) != bind.required_mods) {
    return false;
  }

  // Less than 8 specifies mouse input
  if (bind.primary_key < 8) {
    return glfwGetMouseButton(window_->Get(), bind.primary_key) == GLFW_PRESS ||
           (bind.alternate_key != GLFW_KEY_UNKNOWN &&
            glfwGetMouseButton(window_->Get(), bind.alternate_key) ==
                GLFW_PRESS);
  } else {
    return glfwGetKey(window_->Get(), bind.primary_key) == GLFW_PRESS ||
           (bind.alternate_key != GLFW_KEY_UNKNOWN &&
            glfwGetKey(window_->Get(), bind.alternate_key) == GLFW_PRESS);
  }
}

void KeybindManager::RemapAction(const eastl::string& action, int key,
                                 bool is_primary_key) {
  if (is_primary_key) {
    keybinds_[action].primary_key = key;
  } else {
    keybinds_[action].alternate_key = key;
  }

  UpdateKeyMap();
}

void KeybindManager::SaveKeybinds() {
  nlohmann::json j;
  for (eastl::vector_map<eastl::string, Keybind_>::iterator
           it = keybinds_.begin(),
           it_end = keybinds_.end();
       it != it_end; ++it) {
    j[it->first.c_str()] = {{"primary", it->second.primary_key},
                            {"alternate", it->second.alternate_key},
                            {"allowMods", it->second.allows_modifiers},
                            {"reqMods", it->second.required_mods}};
  }

  std::ofstream file("inputs.imp");
  file << j.dump(4);
}

bool KeybindManager::LoadKeybinds() {
  std::ifstream file("inputs.imp");

  if (file.fail()) return false;

  nlohmann::json j;
  file >> j;

  for (auto& [action, data] : j.items()) {
    keybinds_[action.c_str()] = {action.c_str(), data["primary"],
                                 data["alternate"], data["allowMods"],
                                 data["reqMods"]};
  }

  UpdateKeyMap();

  return true;
}

void KeybindManager::UpdateKeyMap() {
  key_to_action_.clear();
  for (eastl::vector_map<eastl::string, Keybind_>::iterator
           it = keybinds_.begin(),
           it_end = keybinds_.end();
       it != it_end; ++it) {
    key_to_action_[it->second.primary_key] = it->first;
    if (it->second.alternate_key != GLFW_KEY_UNKNOWN) {
      key_to_action_[it->second.alternate_key] = it->first;
    }
  }
}
}  // namespace blu::core
#ifndef GAMEMANAGERCALLBACKHELPER_H
#define GAMEMANAGERCALLBACKHELPER_H

#include <EASTL/internal/function.h>
#include <eastl/string.h>

#include "Components/Model.h"
#include "Components/Transform.h"
#include "Components/Light.h"

struct GameManagerCallbackHelper {
  eastl::function<blu::game::components::Model*(
      const eastl::string& filepath,
      blu::game::components::Transform transform)>
      model_creation_callback;
  eastl::function<blu::game::components::Light*(
      blu::game::components::Transform transform)>
      light_creation_callback;
};

#endif
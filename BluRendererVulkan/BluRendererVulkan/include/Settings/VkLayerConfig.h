#pragma once

#include <vector>
namespace Renderer {
    namespace VkLayerConfig
    {
#ifdef NDEBUG
        inline const bool ARE_VALIDATION_LAYERS_ENABLED = false;
#else
        inline const bool ARE_VALIDATION_LAYERS_ENABLED = true;
#endif

        const char* VALIDATION_LAYERS[]  = {
            "VK_LAYER_KHRONOS_validation" // Contains all the standard validations.
        };
    };
};
#pragma once
#include <map>

#include "../../Render/Renderer/RenderSceneData.h"
#include "EntityChunk/EntityChunkManager.h"
#include "Systems/RendererSystem.h"

class EntityManager {
 public:
  RenderSceneData update();
  // ArchetypeSpecificUpdates
  void updateCamera(std::vector<BaseComponent*> components,
                    RenderSceneData& rendererData, glm::vec4* frustumPlanes,
                    glm::vec4* frustumCorners);
  void updateRenderedEntity(std::vector<BaseComponent*> components,
                            RenderSceneData& rendererData,
                            glm::vec4* frustumPlanes,
                            glm::vec4* frustumCorners);

  uint64_t createEntity(uint32_t components,
                        std::vector<BaseComponent*> componentData);
  std::vector<BaseComponent*> getEntityData(uint32_t components, uint64_t id);

 private:
  std::map<uint32_t, EntityChunkManager> registeredEntityArchetypes;
};
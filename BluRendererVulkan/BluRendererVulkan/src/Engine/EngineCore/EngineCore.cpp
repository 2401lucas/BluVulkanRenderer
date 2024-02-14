#include "EngineCore.h"

#include "../Entity/Components/CameraComponent.h"
#include "../Entity/Components/EntityComponent.h"
#include "../Entity/Components/LightComponent.h"
#include "../Entity/EntityArchetypes.h"
#include "../Mesh/MeshUtils.h"

EngineCore::EngineCore(GLFWwindow* window, const VkApplicationInfo& appInfo,
                       DeviceSettings deviceSettings) {
  sceneManager = new SceneManager();
  sceneManager->loadScene("temp");
  entityManager = new EntityManager();

  renderManager =
      new RenderManager(RendererType::Default, window, appInfo, deviceSettings,
                        sceneManager->getSceneDependancies());
  loadScene("temp");
}

RenderSceneData EngineCore::update(const float& time, InputData inputData,
                                   bool frameBufferResized) {
  RenderSceneData rendererData = entityManager->update();

  renderManager->draw(frameBufferResized, rendererData);
  return rendererData;
}

void EngineCore::fixedUpdate(const EngineTime& time) {
  // Update Physics based on component type
}
// TODO: Move?
void EngineCore::loadScene(const char* scenePath) {
  auto sceneInfo = sceneManager->getSceneInfo();
  for (auto& model : sceneInfo->dynamicModels) {
    std::vector<BaseComponent*> modelBase;
    EntityData* e = new EntityData();
    e->isActive = true;
    modelBase.push_back(e);

    Transform* t = new Transform();
    t->position = model.position;
    t->rotation = model.rotation;
    t->scale = model.scale;
    modelBase.push_back(t);

    MaterialData* m = new MaterialData();
    m->materialIndex =
        renderManager->getRenderer()->registerMaterial(model.materialPath);
    m->pipelineIndex = model.shaderSetID;
    modelBase.push_back(m);

    MeshData md;
    md.meshPath = model.modelPath;
    // TODO: Cache mesh data
    MeshUtils::getMeshDataFromPath(&md);
    renderManager->getRenderer()->registerMesh(md);
    MeshRenderer* mr = new MeshRenderer();
    mr->path = model.modelPath;
    mr->boundingBox = md.boundingBox;
    mr->indexCount = md.indices.size();
    modelBase.push_back(mr);

    entityManager->createEntity(
        TransformComponent + MaterialComponent + MeshRendererComponent,
        modelBase);
  }

  for (auto& light : sceneInfo->lights) {
    std::vector<BaseComponent*> lightBase;

    EntityData* e = new EntityData();
    e->isActive = true;
    lightBase.push_back(e);

    Transform* t = new Transform();
    t->position = light.lightPosition;
    t->rotation = light.lightRotation;
    t->scale = glm::vec3(0);
    lightBase.push_back(t);

    Light* l = new Light();
    l->constant = light.constant;
    l->innerCutoff = light.innerCutoff;
    l->lightType = light.lightType;
    l->linear = light.linear;
    l->outerCutoff = light.outerCutoff;
    l->quad = light.quad;
    lightBase.push_back(l);

    entityManager->createEntity(TransformComponent + LightComponent, lightBase);
  }

  std::vector<BaseComponent*> cameraBase;
  EntityData* e = new EntityData();
  e->isActive = true;
  cameraBase.push_back(e);

  Transform* cameraTransform = new Transform();
  cameraTransform->position = sceneInfo->cameras[0].position;
  cameraTransform->rotation = sceneInfo->cameras[0].rotation;
  cameraTransform->scale = glm::vec3(0);
  cameraBase.push_back(cameraTransform);

  Camera* camera = new Camera();
  camera->fov = sceneInfo->cameras[0].fov;
  camera->ratio = sceneInfo->cameras[0].ratio;
  camera->zNear = sceneInfo->cameras[0].zNear;
  camera->zFar = sceneInfo->cameras[0].zFar;
  cameraBase.push_back(camera);

  entityManager->createEntity(TransformComponent + CameraComponent, cameraBase);
}
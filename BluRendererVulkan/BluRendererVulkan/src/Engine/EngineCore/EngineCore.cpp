#include "EngineCore.h"
#include "../Entity/Components/EntityComponent.h"
#include "../Mesh/MeshUtils.h"
#include "../Entity/Components/CameraComponent.h"
#include "../Entity/Components/LightComponent.h"

EngineCore::EngineCore()
{

}

RenderSceneData EngineCore::update(const float& frameTime, InputData inputData)
{
	RenderSceneData rendererData = entityManager.update();

	return rendererData;
}

void EngineCore::fixedUpdate(const float& frameTime)
{
	//Update Physics based on component type
}

void EngineCore::loadScene(Scene* scene)
{
	auto sceneInfo = scene->getSceneInfo();
	for (auto& model : sceneInfo->dynamicModels) {
		std::vector<BaseComponent*> modelBase;
		EntityData* e = new EntityData();
		e->isActive = true;
		modelBase.push_back(e);

		Transform* t = new Transform();
		t->position	= model.position;
		t->rotation	= model.rotation;
		t->scale		= model.scale;
		modelBase.push_back(t);

		MaterialData* m = new MaterialData();
		m->materialIndex = model.materialIndex;
		m->pipelineIndex = model.shaderSetID;
		m->textureIndex = textureManager.getTextureIndex(model.texturePath);
		m->textureType = TextureType::Phong;
		modelBase.push_back(m);

		MeshRenderer* mr = new MeshRenderer();
		MeshUtils::getMeshDataFromPath(model.modelPath, mr);
		modelBase.push_back(mr);

		entityManager.createEntity(52, modelBase);
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

		entityManager.createEntity(12, lightBase);
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

	entityManager.createEntity(68, cameraBase);
}
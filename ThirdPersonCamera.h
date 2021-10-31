#pragma once

#include "Camera.h"
#include "Transform.h"
#include "GameEntity.h"

class ThirdPersonCamera
{
public:
	ThirdPersonCamera(GameEntity* entity, float aspectRatio);
	~ThirdPersonCamera();

	Camera* GetCamera();

	void Update(float dt);
private:
	Camera* camera;
	GameEntity* entity;
	Transform* pivot;
};